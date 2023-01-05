#include "pch.h"

#include "ActorProcessorTask.h"

#include "Controller.h"
#include "IConditionalVariableProcessor.h"
#include "IObjectManager.h"

#include "IED/AnimationUpdateManager.h"
#include "IED/EngineExtensions.h"
#include "IED/Inventory.h"
#include "IED/ReferenceLightController.h"
#include "IED/StringHolder.h"
#include "IED/Util/Common.h"

#include <ext/BSAnimationUpdateData.h>
#include <ext/Sky.h>

namespace IED
{
	ActorProcessorTask::ActorProcessorTask() :
		m_globalState{ IPerfCounter::Query() }
	{
	}

	SKMP_FORCEINLINE static constexpr void sync_ref_transform(
		ObjectEntryBase::State* a_state,
		const nodesRef_t&       a_nodes) noexcept
	{
		if (a_state->flags.test(ObjectEntryFlags::kSyncReferenceTransform) &&
		    a_nodes.rootNode->IsVisible())
		{
			if (a_state->transform.scale)
			{
				a_nodes.rootNode->m_localTransform.scale =
					a_nodes.ref->m_localTransform.scale * *a_state->transform.scale;
			}
			else
			{
				a_nodes.rootNode->m_localTransform.scale = a_nodes.ref->m_localTransform.scale;
			}

			if (a_state->transform.rotation)
			{
				a_nodes.rootNode->m_localTransform.rot =
					a_nodes.ref->m_localTransform.rot * *a_state->transform.rotation;
			}
			else
			{
				a_nodes.rootNode->m_localTransform.rot = a_nodes.ref->m_localTransform.rot;
			}

			if (a_state->transform.position)
			{
				a_nodes.rootNode->m_localTransform.pos =
					a_nodes.ref->m_localTransform * *a_state->transform.position;
			}
			else
			{
				a_nodes.rootNode->m_localTransform.pos = a_nodes.ref->m_localTransform.pos;
			}
		}
	}

	bool ActorProcessorTask::SyncRefParentNode(
		ActorObjectHolder& a_record,
		ObjectEntryBase&   a_entry) noexcept
	{
		bool result;

		auto& controller = GetController();

		if (const auto info = controller.LookupCachedActorInfo(a_record))
		{
			result = controller.AttachNodeImpl(
				info->npcRoot,
				a_entry.data.state->nodeDesc,
				a_entry.data.state->atmReference,
				a_entry);

			if (result)
			{
				INode::UpdateRootIfGamePaused(info->root);
			}
		}
		else
		{
			result = false;
		}

		return result;
	}

	void ActorProcessorTask::DoObjectRefSync(
		ActorObjectHolder& a_record,
		ObjectEntryBase&   a_entry) noexcept
	{
		const auto  state = a_entry.data.state.get();
		const auto& nodes = state->nodes;

		if (!nodes.ref)
		{
			return;
		}

		if (nodes.IsReferenceMovedOrOphaned())
		{
			if (state->flags.test(ObjectEntryFlags::kRefSyncDisableFailedOrphan))
			{
				return;
			}

			const bool result = SyncRefParentNode(a_record, a_entry);
			if (result)
			{
				a_record.RequestEval();
			}
			else
			{
				state->flags.set(ObjectEntryFlags::kRefSyncDisableFailedOrphan);
				return;
			}

			if (!nodes.ref)
			{
				return;
			}
		}

		sync_ref_transform(state, nodes);
	}

	void ActorProcessorTask::DoObjectRefSyncMTSafe(
		ActorObjectHolder& a_record,
		ObjectEntryBase&   a_entry) noexcept
	{
		const auto  state = a_entry.data.state.get();
		const auto& nodes = state->nodes;

		if (!nodes.ref)
		{
			return;
		}

		if (nodes.IsReferenceMovedOrOphaned())
		{
			if (!state->flags.test(ObjectEntryFlags::kRefSyncDisableFailedOrphan))
			{
				stl::lock_guard lock(m_syncRefParentQueueWRLock);

				m_syncRefParentQueue.emplace_back(
					std::addressof(a_record),
					std::addressof(a_entry));
			}
		}
		else
		{
			sync_ref_transform(state, nodes);
		}
	}

	void ActorProcessorTask::ProcessTransformUpdateRequest(
		ActorObjectHolder& a_data) noexcept
	{
		if (!a_data.m_flags.test(ActorObjectHolderFlags::kWantTransformUpdate))
		{
			return;
		}

		if (!a_data.m_flags.test(ActorObjectHolderFlags::kImmediateTransformUpdate) &&
		    a_data.m_flags.test(ActorObjectHolderFlags::kSkipNextTransformUpdate))
		{
			a_data.m_flags.clear(ActorObjectHolderFlags::kSkipNextTransformUpdate);
		}
		else
		{
			a_data.m_flags.clear(ActorObjectHolderFlags::kRequestTransformUpdateMask);

			GetController().EvaluateTransformsImpl(
				a_data,
				ControllerUpdateFlags::kUseCachedParams);
		}
	}

	void ActorProcessorTask::ProcessEvalRequest(ActorObjectHolder& a_data) noexcept
	{
		if (a_data.m_flags.consume(ActorObjectHolderFlags::kEvalThisFrame))
		{
			if (auto& params = a_data.GetCurrentProcessParams())
			{
				GetController().EvaluateImpl(
					*params,
					a_data,
					ControllerUpdateFlags::kPlaySound |
						ControllerUpdateFlags::kFromProcessorTask |
						ControllerUpdateFlags::kUseCachedParams);
			}
			else
			{
				GetController().EvaluateImpl(
					a_data,
					ControllerUpdateFlags::kPlaySound |
						ControllerUpdateFlags::kFromProcessorTask |
						ControllerUpdateFlags::kUseCachedParams);
			}
		}
	}

	static constexpr bool CheckMonitorNodes(
		ActorObjectHolder& a_data) noexcept
	{
		bool result = false;

		for (auto& f : a_data.GetMonitorNodes())
		{
			if (f.parent != f.node->m_parent)
			{
				f.parent = f.node->m_parent;

				result = true;
			}

			if (const auto size = f.node->m_children.m_size;
			    f.size != size)
			{
				f.size = size;

				result = true;
			}

			if (const bool visible = f.node->IsVisible();
			    visible != f.visible)
			{
				f.visible = visible;

				result = true;
			}
		}

		return result;
	}

	void ActorProcessorTask::UpdateGlobalState() noexcept
	{
		auto& controller = GetController();

		if (const auto lrhandle = (*g_thePlayer)->lastRiddenHorseHandle;
		    lrhandle != m_globalState.playerLastRidden)
		{
			m_globalState.playerLastRidden = lrhandle;

			controller.RequestLFEvaluateAll();
		}

		if (const auto fpstate = IsInFirstPerson();
		    fpstate != m_globalState.inFirstPerson)
		{
			m_globalState.inFirstPerson = fpstate;

			if (auto it = controller.m_objects.find(Data::IData::GetPlayerRefID());
			    it != controller.m_objects.end())
			{
				it->second.RequestEval();
			}
		}

		if (m_timer.GetStartTime() < m_globalState.nextRun)
		{
			return;
		}

		m_globalState.nextRun =
			m_timer.GetStartTime() +
			IPerfCounter::T(COMMON_STATE_CHECK_INTERVAL);

		bool changed = false;

		const auto* const sky = RE::Sky::GetSingleton();
		assert(sky);

		if (const auto current = (sky ? sky->GetCurrentWeatherHalfPct() : nullptr);
		    current != m_globalState.currentWeather)
		{
			m_globalState.currentWeather = current;
			changed                      = true;
		}

		if (const auto tod = Data::GetTimeOfDay(sky);
		    tod != m_globalState.timeOfDay)
		{
			m_globalState.timeOfDay = tod;
			changed                 = true;
		}

#if defined(IED_ENABLE_CONDITION_EN)

		auto player = *g_thePlayer;
		assert(player);

		if (player->loadedState)
		{
			auto pl = Game::ProcessLists::GetSingleton();
			assert(pl);

			if (bool n = pl->PlayerHasEnemiesNearby(0);
			    n != m_state.playerEnemiesNearby)
			{
				m_state.playerEnemiesNearby = n;

				if (auto it = m_controller.m_objects.find(Data::IData::GetPlayerRefID());
				    it != m_controller.m_objects.end())
				{
					it->second.RequestEval();
				}
			}
		}

#endif

		if (changed)
		{
			controller.RequestLFEvaluateAll();
		}
	}

	template <bool _Mt>
	void ActorProcessorTask::UpdateHolderMTSafe(
		const float                             a_interval,
		const Game::Unk2f6b948::Steps&          a_stepMuls,
		const std::optional<PhysicsUpdateData>& a_physUpdData,
		ActorObjectHolder&                      a_holder,
		bool                                    a_updateEffects) noexcept
	{
		auto& state = a_holder.m_state;

		NiPointer<TESObjectREFR> refr;
		if (!a_holder.GetHandle().Lookup(refr))
		{
			state.cellAttached = false;
			return;
		}

		const auto actor = refr->As<Actor>();
		if (!Util::Common::IsREFRValid(actor))
		{
			state.cellAttached = false;
			return;
		}

		const auto* const cell = actor->GetParentCell();
		if (cell && cell->IsAttached())
		{
			if (!state.cellAttached)
			{
				a_holder.RequestEvalDefer();
				state.cellAttached = true;
			}
		}
		else
		{
			state.cellAttached = false;
			return;
		}

		if (actor != a_holder.m_actor)  // ??
		{
			a_holder.m_actor = actor;
		}

		if (state.UpdateState(actor, cell))
		{
			a_holder.RequestEvalDefer();
		}

		a_holder.m_wantLFUpdate |= state.UpdateStateLF(actor);

		if (m_timer.GetStartTime() >= a_holder.m_nextLFStateCheck)
		{
			a_holder.m_nextLFStateCheck =
				m_timer.GetStartTime() +
				IPerfCounter::T(ActorObjectHolder::STATE_CHECK_INTERVAL_LOW);

			a_holder.m_wantLFUpdate |= state.UpdateFactions(actor);

			if (a_holder.m_wantLFUpdate)
			{
				a_holder.m_wantLFUpdate = false;
				a_holder.RequestEval();
			}
		}

		if (m_timer.GetStartTime() >= a_holder.m_nextMFStateCheck)
		{
			a_holder.m_nextMFStateCheck =
				m_timer.GetStartTime() +
				IPerfCounter::T(ActorObjectHolder::STATE_CHECK_INTERVAL_MED);

			if (state.UpdateEffects(actor))
			{
				a_holder.RequestEval();
			}
		}

		if (m_timer.GetStartTime() >= a_holder.m_nextHFStateCheck)
		{
			a_holder.m_nextHFStateCheck =
				m_timer.GetStartTime() +
				IPerfCounter::T(ActorObjectHolder::STATE_CHECK_INTERVAL_HIGH);

			if (a_holder.m_wantHFUpdate)
			{
				a_holder.m_wantHFUpdate = false;
				a_holder.RequestEval();
			}
		}

		if (a_holder.m_flags.test(ActorObjectHolderFlags::kWantEval))
		{
			if (a_holder.m_flagsbf.evalCountdown > 0)
			{
				a_holder.m_flagsbf.evalCountdown--;
			}

			if (a_holder.m_flags.test(ActorObjectHolderFlags::kImmediateEval) ||
			    a_holder.m_flagsbf.evalCountdown == 0)
			{
				a_holder.m_flags.clear(ActorObjectHolderFlags::kRequestEvalMask);
				a_holder.m_flags.set(ActorObjectHolderFlags::kEvalThisFrame);
			}
		}

		if (a_holder.UpdateNodeMonitorEntries())
		{
			a_holder.RequestEval();
		}

		if (CheckMonitorNodes(a_holder))
		{
			a_holder.RequestTransformUpdate();
		}

		bool update = false;

		a_holder.visit([&](auto& a_v) noexcept [[msvc::forceinline]] {
			auto& state = a_v.data.state;

			if (!state)
			{
				return;
			}

			if constexpr (_Mt)
			{
				DoObjectRefSyncMTSafe(a_holder, a_v);
			}
			else
			{
				DoObjectRefSync(a_holder, a_v);
			}

			if constexpr (
				std::is_same_v<
					std::remove_cvref_t<decltype(a_v)>,
					ObjectEntrySlot>)
			{
				if (state->hideCountdown)
				{
					if (--state->hideCountdown == 0)
					{
						if (state->flags.test(ObjectEntryFlags::kInvisible))
						{
							/*if (state->flags.test(ObjectEntryFlags::kWantUnloadAfterHide))
							{
								if constexpr (_Mt)
								{
									{
										stl::lock_guard lock(m_syncRefParentQueueWRLock);
										std::erase_if(
											m_syncRefParentQueue,
											[&](auto& a_e) {
												return a_e.second == std::addressof(a_v);
											});
									}

									stl::lock_guard lock(m_unloadQueueWRLock);
									m_unloadQueue.emplace_back(std::addressof(a_holder), std::addressof(a_v));
								}
								else
								{
									GetController().RemoveObject(
										nullptr,
										a_holder.GetHandle(),
										a_v,
										a_holder,
										ControllerUpdateFlags::kNone,
										false);
								}
							}
							else
							{
								state->SetVisible(false);

								if (state->nodes.HasPhysicsNode())
								{
									if (auto& simComponent = state->simComponent)
									{
										a_holder.RemoveAndDestroySimComponent(simComponent);
									}
								}
							}*/

							state->SetVisible(false);

							if (state->nodes.HasPhysicsNode())
							{
								if (auto& simComponent = state->simComponent)
								{
									a_holder.RemoveAndDestroySimComponent(simComponent);
								}
							}

							update = true;
						}
					}
				}
			}
		});

		if (update)
		{
			a_holder.RequestTransformUpdateDefer();
		}

		if (a_updateEffects)
		{
			RunEffectUpdates(a_interval, a_stepMuls, a_physUpdData, a_holder);
		}
	}

	void ActorProcessorTask::RunPreUpdates(
		const Game::Unk2f6b948::Steps& a_stepMuls) noexcept
	{
		const auto interval = *Game::g_frameTimerSlow;

		std::optional<PhysicsUpdateData> physUpdateData;

		if (PhysicsProcessingEnabled())
		{
			PreparePhysicsUpdateData(interval, physUpdateData);
		}

		const auto paused = Game::IsPaused();

		auto& data = GetController().GetObjects();

		if (ParallelProcessingEnabled())
		{
			std::for_each(
				std::execution::par,
				data.begin(),
				data.end(),
				[&](auto& a_e) noexcept {
					UpdateHolderMTSafe<true>(
						interval,
						a_stepMuls,
						physUpdateData,
						a_e.second,
						!paused);
				});

			if (!m_syncRefParentQueue.empty())
			{
				for (const auto& e : m_syncRefParentQueue)
				{
					const bool result = SyncRefParentNode(*e.first, *e.second);

					if (result)
					{
						e.first->RequestEval();
					}
					else
					{
						e.second->data.state->flags.set(ObjectEntryFlags::kRefSyncDisableFailedOrphan);
					}
				}

				m_syncRefParentQueue.clear();
			}

			/*if (!m_unloadQueue.empty())
			{
				for (const auto& e : m_unloadQueue)
				{
					GetController().RemoveObject(
						nullptr,
						e.first->GetHandle(),
						*e.second,
						*e.first,
						ControllerUpdateFlags::kNone,
						false);
				}

				m_unloadQueue.clear();
			}*/
		}
		else
		{
			for (auto& e : data)
			{
				UpdateHolderMTSafe<false>(
					interval,
					a_stepMuls,
					physUpdateData,
					e.second,
					!paused);
			}
		}
	}

	static void UpdateActorGearAnimations(
		TESObjectREFR*           a_actor,
		const ActorObjectHolder& a_holder,
		float                    a_step) noexcept
	{
		struct TLSData
		{
			std::uint8_t  unk000[0x768];  // 000
			std::uint32_t unk768;         // 768
		};

		auto tlsData = reinterpret_cast<TLSData**>(__readgsqword(0x58));

		auto& tlsUnk768 = tlsData[*g_TlsIndexPtr]->unk768;

		const auto oldUnk768 = tlsUnk768;
		tlsUnk768            = 0x3A;

		BSAnimationUpdateData data{ a_step };
		data.reference    = a_actor;
		data.shouldUpdate = a_actor->GetMustUpdate();

		a_actor->ModifyAnimationUpdateData(data);

		a_holder.UpdateAllAnimationGraphs(data);

		tlsUnk768 = oldUnk768;
	}

	void ActorProcessorTask::SetProcessorTaskRunAUState(bool a_state) noexcept
	{
		m_runAnimationUpdates = !AnimationUpdateController::GetSingleton().GetEnabled() && a_state;
	}

	void ActorProcessorTask::Run() noexcept
	{
		auto& controller = GetController();

		const stl::lock_guard lock(controller.m_lock);

		if (!m_run)
		{
			return;
		}

		m_timer.Begin();

		UpdateGlobalState();

		const auto stepMuls = Game::Unk2f6b948::GetStepMultipliers();

		std::optional<animUpdateData_t> animUpdateData;

		if (m_runAnimationUpdates &&
		    !Game::IsPaused())
		{
			animUpdateData.emplace(stepMuls * *Game::g_frameTimerSlow);
		}

		RunPreUpdates(stepMuls);

		const auto& cvdata = controller.m_config.active.condvars;

		if (!cvdata.empty() || animUpdateData)
		{
			for (auto& [i, e] : controller.m_objects)
			{
				if (!e.IsCellAttached())
				{
					continue;
				}

				if (animUpdateData)
				{
					const auto step =
						e.IsPlayer() ?
							animUpdateData->steps.player :
							animUpdateData->steps.npc;

					UpdateActorGearAnimations(e.m_actor, e, step);
				}

				const bool wantVarUpdate = e.m_flags.consume(ActorObjectHolderFlags::kWantVarUpdate);

				if ((wantVarUpdate ||
				     e.m_flags.test(ActorObjectHolderFlags::kEvalThisFrame)) &&
				    !cvdata.empty())
				{
					if (auto info = controller.LookupCachedActorInfo(e.m_actor, e))
					{
						auto& params = e.CreateProcessParams(
							info->sex,
							ControllerUpdateFlags::kPlaySound,
							info->actor.get(),
							info->handle,
							controller.m_temp.sr,
							e.m_temp.idt,
							e.m_temp.eqt,
							controller.m_temp.uc,
							e.m_actor,
							info->npc,
							info->npcOrTemplate,
							info->race,
							info->root,
							info->npcRoot,
							e,
							controller);

						if (IConditionalVariableProcessor::UpdateVariableMap(
								params,
								cvdata,
								e.GetVariables()))
						{
							controller.RequestHFEvaluateAll(i);
							e.m_flags.set(ActorObjectHolderFlags::kEvalThisFrame);
						}
					}
				}
			}
		}

		for (auto& e : controller.m_objects)
		{
			if (e.second.IsCellAttached())
			{
				ProcessEvalRequest(e.second);
				ProcessTransformUpdateRequest(e.second);

				e.second.ClearCurrentProcessParams();
			}
		}

		controller.RunObjectCleanup();

		m_timer.End(m_currentTime);
	}

	Controller& ActorProcessorTask::GetController() noexcept
	{
		return static_cast<Controller&>(*this);
	}

}
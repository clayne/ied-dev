#include "pch.h"

#include "ReferenceLightController.h"

#include "Controller/INode.h"
#include "StringHolder.h"

namespace IED
{
	ReferenceLightController ReferenceLightController::m_Instance;

	namespace detail
	{
		static constexpr RE::LIGHT_CREATE_PARAMS make_params(
			TESObjectREFR*              a_refr,
			const TESObjectLIGH*        a_lightForm,
			const Data::ExtraLightData* a_config) noexcept
		{
			RE::LIGHT_CREATE_PARAMS params;

			params.portalStrict = a_lightForm->data.flags.test_any(TES_LIGHT_FLAGS::kPortalStrict);
			params.affectLand   = a_config ? !a_config->flags.test(Data::ExtraLightFlags::kDontLightLandscape) : true;  // EngineFixes patch (always true for chars), default is (a_actor->flags & kTESObjectREFRFlag_DoesntLightLandscape) == 0
			params.lightsWater  = a_config ? !a_config->flags.test(Data::ExtraLightFlags::kDontLightWater) : true;      // (a_actor->flags & TESObjectREFR::kFlag_DoesnLightWater) == 0;
			params.neverFades   = true;                                                                                 // !a_refr->IsHeadingMarker();
			params.unk00        = true;                                                                                 // arg4 in the original func

			params.data3D.lensFlareRenderData = static_cast<RE::BSLensFlareRenderData*>(a_lightForm->lensFlare);

			if (a_lightForm->data.flags.test_any(TES_LIGHT_FLAGS::kShadowFlags))
			{
				params.shadow          = true;
				params.falloffExponent = a_lightForm->data.fallofExponent;

				constexpr auto pi = std::numbers::pi_v<float>;

				if (a_lightForm->data.flags.test_any(TES_LIGHT_FLAGS::kSpotShadow))
				{
					params.fov = a_config ? a_config->fieldOfView * pi / 180.0f : 0.0f;  // 0 if base != TESObjectLIGH (TESObjectREFR::GetLightExtraFOV(a_refr) * 0.017453292);
				}
				else if (a_lightForm->data.flags.test_any(TES_LIGHT_FLAGS::kHemiShadow))
				{
					params.fov = pi;
				}
				else
				{
					params.fov = pi * 2.0f;
				}

				params.nearDistance    = a_lightForm->data.nearDistance;
				params.shadowDepthBias = a_config ? a_config->shadowDepthBias : 0.0f;  // 0 if base != TESObjectLIGH (TESObjectREFR::GetExtraLightShadowDepthBias(a_refr))
			}
			else
			{
				if (a_config && a_config->flags.test(Data::ExtraLightFlags::kTargetSelf))
				{
					params.data3D.lightingTarget = a_refr->Get3D1(false);
				}

				params.shadow = false;
			}

			return params;
		}

		static constexpr RE::LIGHT_CREATE_PARAMS make_params() noexcept
		{
			return {
				true,
				false,
				false,
				true,
				true,
				true,
				1.0f,
				1.0f,
				5.0f,
				0.0f,
				0,
				{ nullptr, nullptr }
			};
		}

		static constexpr NiColor make_diffuse(
			const TESObjectLIGH* a_lightForm) noexcept
		{
			constexpr auto mul = 1.0f / 255.0f;

			if (!a_lightForm->data.flags.test_any(TES_LIGHT_FLAGS::kNegative))
			{
				return {
					static_cast<float>(a_lightForm->data.color.red) * mul,
					static_cast<float>(a_lightForm->data.color.green) * mul,
					static_cast<float>(a_lightForm->data.color.blue) * mul
				};
			}
			else
			{
				return {
					-(static_cast<float>(a_lightForm->data.color.red) * mul),
					-(static_cast<float>(a_lightForm->data.color.green) * mul),
					-(static_cast<float>(a_lightForm->data.color.blue) * mul)
				};
			}
		}

		static constexpr NiPoint3 make_radius(
			const TESObjectLIGH* a_lightForm) noexcept
		{
			auto radius = static_cast<float>(a_lightForm->data.radius);

			if (radius <= 0.0f)
			{
				radius = 0.1f;
			}

			return { radius, radius, radius };
		}
	}

	static REFR_LIGHT* GetExtraLight(Actor* a_actor) noexcept
	{
		if (auto extraLight = a_actor->extraData.Get<ExtraLight>())
		{
			return extraLight->lightData;
		}
		else
		{
			return nullptr;
		}
	}

	static constexpr TESObjectLIGH* GetEquippedLHLight(
		const Actor* const a_actor) noexcept
	{
		const auto* const pm = a_actor->processManager;
		if (!pm)
		{
			return nullptr;
		}

		const auto* const mid = pm->middleProcess;
		if (!mid)
		{
			return nullptr;
		}

		const auto* const lh = mid->leftHand;
		if (!lh)
		{
			return nullptr;
		}

		const auto obj = lh->type;
		if (!obj)
		{
			return nullptr;
		}

		return obj->As<TESObjectLIGH>();
	}

	void ReferenceLightController::Initialize()
	{
		const auto edl = ScriptEventSourceHolder::GetSingleton();
		ASSERT(edl != nullptr);
		edl->AddEventSink<TESCellAttachDetachEvent>(this);

		m_initialized = true;
	}

	void ReferenceLightController::OnUpdatePlayerLight(
		PlayerCharacter* a_actor) const noexcept
	{
		const read_lock_guard lock(m_lock);

		visit_lights(
			a_actor,
			[&](auto& a_entry) noexcept [[msvc::forceinline]] {
				UpdateRefrLight(a_entry.form, a_entry.data.niObject, a_actor, -1.0f);
			});
	}

	void ReferenceLightController::OnActorCrossCellBoundary(
		Actor* a_actor) const noexcept
	{
		const read_lock_guard lock(m_lock);

		visit_lights(a_actor, [&](auto& a_entry) [[msvc::forceinline]] {
			const auto params = detail::make_params(
				a_actor,
				a_entry.form,
				std::addressof(a_entry.data.extraLightData));

			RE::ShadowSceneNode::GetSingleton()->CreateAndAddLight(a_entry.data.niObject.get(), params);
		});
	}

	void ReferenceLightController::OnActorCellAttached(
		Actor* a_actor) const noexcept
	{
		if (a_actor == *g_thePlayer)
		{
			return;
		}

		if (!a_actor->Get3D2())
		{
			return;
		}

		OnActorCrossCellBoundary(a_actor);

		if (m_fixVanillaLightOnCellAttach.load(std::memory_order_relaxed))
		{
			ReAddActorExtraLight(a_actor);
		}
	}

	void ReferenceLightController::OnRefreshLightOnSceneMove(
		Actor* a_actor) const noexcept
	{
		const read_lock_guard lock(m_lock);

		visit_lights(
			a_actor,
			[](auto& a_entry) noexcept [[msvc::forceinline]] {
				if (auto& bsl = a_entry.data.bsObject)
				{
					RE::ShadowSceneNode::GetSingleton()->QueueAddLight(
						bsl.get());
				}
			});
	}

	void ReferenceLightController::OnActorUpdate(
		Actor*      a_actor,
		REFR_LIGHT* a_extraLight) const noexcept
	{
		if (a_actor == *g_thePlayer)
		{
			const read_lock_guard lock(m_lock);

			visit_lights(
				a_actor,
				[](auto& a_entry) noexcept [[msvc::forceinline]] {
					RE::ShadowSceneNode::GetSingleton()->UnkQueueBSLight(
						a_entry.data.niObject.get());
				});
		}
		else
		{
			if (m_fixVanillaNPCLightUpdates.load(std::memory_order_relaxed) &&
			    a_extraLight &&
			    a_extraLight->light)
			{
				if (const auto equipped = GetEquippedLHLight(a_actor))
				{
					if (::NRTTI<NiPointLight>::IsType(a_extraLight->light->GetRTTI()))
					{
						UpdateRefrLight(
							equipped,
							reinterpret_cast<const NiPointer<NiPointLight>&>(a_extraLight->light),
							a_actor,
							-1.0f);
					}
				}
			}

			if (m_npcLightUpdates.load(std::memory_order_relaxed))
			{
				const read_lock_guard lock(m_lock);

				visit_lights(
					a_actor,
					[&](auto& a_entry) noexcept [[msvc::forceinline]] {
						UpdateRefrLight(
							a_entry.form,
							a_entry.data.niObject,
							a_actor,
							-1.0f);

						RE::ShadowSceneNode::GetSingleton()->UnkQueueBSLight(
							a_entry.data.niObject.get());
					});
			}
			else
			{
				const read_lock_guard lock(m_lock);

				visit_lights(
					a_actor,
					[](auto& a_entry) noexcept [[msvc::forceinline]] {
						RE::ShadowSceneNode::GetSingleton()->UnkQueueBSLight(
							a_entry.data.niObject.get());
					});
			}
		}
	}

	void ReferenceLightController::AddLight(
		Game::FormID       a_actor,
		TESObjectLIGH*     a_form,
		const ObjectLight& a_light) noexcept
	{
		if (m_initialized)
		{
			const write_lock_guard lock(m_lock);

			auto& e = m_data.try_emplace(a_actor).first->second;

			e.emplace_front(a_form, a_light);
		}
	}

	void ReferenceLightController::RemoveLight(
		Game::FormID                   a_actor,
		const NiPointer<NiPointLight>& a_light) noexcept
	{
		if (m_initialized)
		{
			const write_lock_guard lock(m_lock);

			auto it = m_data.find(a_actor);
			if (it != m_data.end())
			{
				std::erase_if(it->second, [&](auto& a_v) {
					return a_v.data.niObject == a_light;
				});
			}
		}
	}

	void ReferenceLightController::RemoveActor(
		Game::FormID a_actor) noexcept
	{
		if (m_initialized)
		{
			const write_lock_guard lock(m_lock);

			m_data.erase(a_actor);
		}
	}

	/*void ReferenceLightController::ReAttachLight(
		Actor*        a_actor,
		ObjectLight&  a_object) noexcept
	{
		if (m_initialized)
		{
			const write_lock_guard lock(m_lock);

			auto it = m_data.find(a_actor->formID);
			if (it != m_data.end())
			{
				auto it2 = std::find_if(
					it->second.begin(),
					it->second.end(),
					[&](auto& a_e) {
						return a_e.niLight == a_object.niObject;
					});

				if (it2 != it->second.end())
				{
					ReAttachLightImpl(a_actor, *it2, a_object);
				}
			}
		}
	}*/

	std::unique_ptr<ObjectLight> ReferenceLightController::CreateAndAttachPointLight(
		const TESObjectLIGH*        a_lightForm,
		Actor*                      a_actor,
		NiNode*                     a_object,
		const Data::ExtraLightData& a_config) noexcept
	{
		const auto sh = BSStringHolder::GetSingleton();

		auto attachmentNode = ::Util::Node::GetNodeByName(a_object, sh->m_attachLight);

		if (!attachmentNode)
		{
			attachmentNode = a_object;
		}

		char name[32];
		stl::snprintf(name, "%.8X PtLight", a_lightForm->formID.get());

		const auto pointLight = NiPointLight::Create();

		pointLight->m_name = name;

		attachmentNode->AttachChild(pointLight, true);

		pointLight->ambient = {};
		pointLight->radius  = detail::make_radius(a_lightForm);
		pointLight->diffuse = detail::make_diffuse(a_lightForm);

		const auto params = detail::make_params(
			a_actor,
			a_lightForm,
			std::addressof(a_config));

		const auto bsLight = RE::ShadowSceneNode::GetSingleton()->CreateAndAddLight(pointLight, params);

		NiPointLightSetAttenuation(pointLight, static_cast<std::int32_t>(pointLight->radius.x));

		pointLight->fade = a_lightForm->fade;

		return std::make_unique<ObjectLight>(
			pointLight,
			a_actor == *g_thePlayer ? bsLight : nullptr,
			a_config);
	}

	void ReferenceLightController::QueueRemoveAllLightsFromNode(NiNode* a_node) noexcept
	{
		RE::ShadowSceneNode::GetSingleton()->QueueRemoveAllLightsFromNode(a_node, true, true);
	}

	std::size_t ReferenceLightController::GetNumLights() const noexcept
	{
		const read_lock_guard lock(m_lock);

		std::size_t i = 0;

		for (auto& e : m_data)
		{
			std::for_each(
				e.second.begin(),
				e.second.end(),
				[&](auto&) [[msvc::forceinline]] {
					i++;
				});
		}

		return i;
	}

	bool ReferenceLightController::HasBSLight(
		Game::FormID a_actor,
		const RE::BSLight* a_light) noexcept
	{
		const read_lock_guard lock(m_lock);

		auto it = m_data.find(a_actor);
		if (it != m_data.end())
		{
			for (auto& e : it->second)
			{
				if (e.data.bsObject.get() == a_light)
				{
					return true;
				}
			}
		}

		return false;
	}

	void ReferenceLightController::ReAddActorExtraLight(Actor* a_actor) noexcept
	{
		const auto* const refrLight = GetExtraLight(a_actor);
		if (!refrLight)
		{
			return;
		}

		const auto& light = refrLight->light;
		if (!light)
		{
			return;
		}

		const auto* const torch = GetEquippedLHLight(a_actor);

		const auto params =
			torch ?
				detail::make_params(a_actor, torch, nullptr) :
				detail::make_params();

		RE::ShadowSceneNode::GetSingleton()->CreateAndAddLight(light.get(), params);
	}

	EventResult ReferenceLightController::ReceiveEvent(
		const TESCellAttachDetachEvent*           a_evn,
		BSTEventSource<TESCellAttachDetachEvent>* a_dispatcher)
	{
		if (a_evn && a_evn->attached && a_evn->reference)
		{
			if (auto actor = a_evn->reference->As<Actor>())
			{
				OnActorCellAttached(actor);
			}
		}

		return EventResult::kContinue;
	}
}
#include "pch.h"

#include "Controller/Controller.h"
#include "EngineExtensions.h"
#include "Util/Logging.h"

#include <ext/GarbageCollector.h>
#include <ext/IHook.h>
#include <ext/JITASM.h>
#include <ext/Node.h>
#include <ext/VFT.h>

constexpr static auto mv_failstr = "Memory validation failed";

#define UNWRAP(...) __VA_ARGS__
#define VALIDATE_MEMORY(addr, bytes_se, bytes_ae)                                  \
	{                                                                              \
		if (IAL::IsAE())                                                           \
		{                                                                          \
			ASSERT_STR(Patching::validate_mem(addr, UNWRAP bytes_ae), mv_failstr); \
		}                                                                          \
		else                                                                       \
		{                                                                          \
			ASSERT_STR(Patching::validate_mem(addr, UNWRAP bytes_se), mv_failstr); \
		}                                                                          \
	}

namespace IED
{
	EngineExtensions EngineExtensions::m_Instance;

	void EngineExtensions::Install(
		Controller*                       a_controller,
		const std::shared_ptr<ConfigINI>& a_config)
	{
		m_Instance.InstallImpl(a_controller, a_config);
	}

	void EngineExtensions::InstallImpl(
		Controller*                       a_controller,
		const std::shared_ptr<ConfigINI>& a_config)
	{
		m_controller = a_controller;

		Patch_RemoveAllBipedParts();
		Hook_REFR_GarbageCollector();
		Hook_Actor_Resurrect();
		Hook_Actor_3DEvents();

		if (a_config->m_nodeOverrideEnabled)
		{
			m_conf.weaponAdjustDisable       = a_config->m_weaponAdjustDisable;
			m_conf.nodeOverridePlayerEnabled = a_config->m_nodeOverridePlayerEnabled;
			m_conf.disableNPCProcessing      = a_config->m_disableNPCProcessing;

			Hook_Armor_Update();
			Patch_CreateWeaponNodes();
		}

		if (a_config->m_weaponAdjustFix)
		{
			m_conf.weaponAdjustFix = a_config->m_weaponAdjustFix;

			Patch_SetWeapAdjAnimVar();
		}

		if (m_conf.weaponAdjustDisable)
		{
			Patch_WeaponAdjustDisable();
		}

		if (a_config->m_immediateFavUpdate)
		{
			Hook_ToggleFav();
		}

		if (a_config->m_effectShaders)
		{
			Hook_ProcessEffectShaders();
		}

		/*if (a_config->m_enableCorpseScatter)
		{
			Patch_CorpseScatter();
		}*/
	}

	void EngineExtensions::Patch_RemoveAllBipedParts()
	{
		VALIDATE_MEMORY(
			m_removeAllBipedParts_a.get(),
			({ 0x40, 0x57, 0x48, 0x83, 0xEC, 0x30 }),
			({ 0x40, 0x56, 0x57, 0x41, 0x56, 0x48, 0x83, 0xEC, 0x30 }));

		struct Assembly : JITASM::JITASM
		{
			Assembly(std::uintptr_t a_targetAddr) :
				JITASM(ISKSE::GetLocalTrampoline())
			{
				Xbyak::Label retnLabel;

				std::size_t size = IAL::IsAE() ? 0x9 : 0x6;

				db(reinterpret_cast<Xbyak::uint8*>(a_targetAddr), size);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_targetAddr + size);
			}
		};

		LogPatchBegin();
		{
			Assembly code(m_removeAllBipedParts_a.get());
			m_removeAllBipedParts_o = code.get<decltype(m_removeAllBipedParts_o)>();

			ISKSE::GetBranchTrampoline().Write6Branch(
				m_removeAllBipedParts_a.get(),
				std::uintptr_t(RemoveAllBipedParts_Hook));
		}
		LogPatchEnd();
	}

	void EngineExtensions::Hook_REFR_GarbageCollector()
	{
		if (hook::call5(
				ISKSE::GetBranchTrampoline(),
				m_garbageCollectorREFR_a.get(),
				std::uintptr_t(GarbageCollectorReference_Hook),
				m_garbageCollectorReference_o))
		{
			Debug("[%s] Installed", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install garbage collector hook");
		}
	}

	void EngineExtensions::Hook_Actor_Resurrect()
	{
		if (VTable::Detour2(
				m_vtblCharacter_a.get(),
				0xAB,
				Character_Resurrect_Hook,
				&m_characterResurrect_o))
		{
			Debug("[%s] Detoured Character::Resurrect @0xAB", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install Character::Resurrect vtbl hook");
		}

		if (hook::call5(
				ISKSE::GetBranchTrampoline(),
				m_reanimActorStateUpdate_a.get(),
				std::uintptr_t(ReanimateActorStateUpdate_Hook),
				m_ReanimActorStateUpd_o))
		{
			Debug("[%s] Installed reanimate hook", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install state update hook");
		}
	}

	void EngineExtensions::Hook_Actor_3DEvents()
	{
		if (VTable::Detour2(
				m_vtblCharacter_a.get(),
				0x6B,
				Character_Release3D_Hook,
				&m_characterRelease3D_o))
		{
			Debug("[%s] Detoured Character::Release3D @0x6B", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install Character::Release3D vtbl hook");
		}

		if (VTable::Detour2(
				m_vtblActor_a.get(),
				0x6B,
				Actor_Release3D_Hook,
				&m_actorRelease3D_o))
		{
			Debug("[%s] Detoured Actor::Release3D @0x6B", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install Actor::Release3D vtbl hook");
		}
	}

	void EngineExtensions::Hook_Armor_Update()
	{
		if (hook::call5(
				ISKSE::GetBranchTrampoline(),
				m_armorUpdate_a.get(),
				std::uintptr_t(ArmorUpdate_Hook),
				m_ArmorChange_o))
		{
			Debug("[%s] Installed", __FUNCTION__);
		}
		else
		{
			HALT("Failed to install armor update hook");
		}
	}

	void EngineExtensions::Patch_SetWeapAdjAnimVar()
	{
		VALIDATE_MEMORY(
			m_weapAdj_a.get(),
			({ 0xE8 }),
			({ 0xE8 }));

		struct Assembly : JITASM::JITASM
		{
			Assembly(std::uintptr_t targetAddr) :
				JITASM(ISKSE::GetLocalTrampoline())
			{
				Xbyak::Label callLabel;
				Xbyak::Label retnLabel;

				mov(rcx, rsi);
				mov(r9, r13);  // Biped
				call(ptr[rip + callLabel]);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(targetAddr + 0x5);

				L(callLabel);
				dq(std::uintptr_t(SetWeapAdjAnimVar_Hook));
			}
		};

		LogPatchBegin();
		{
			Assembly code(m_weapAdj_a.get());
			ISKSE::GetBranchTrampoline().Write5Branch(
				m_weapAdj_a.get(),
				code.get());
		}
		LogPatchEnd();
	}

	void EngineExtensions::Patch_CreateWeaponNodes()
	{
		VALIDATE_MEMORY(
			m_createWeaponNodes_a.get(),
			({ 0x40, 0x56, 0x57, 0x41, 0x54, 0x41, 0x56 }),
			({ 0x40, 0x56, 0x57, 0x41, 0x54, 0x41, 0x56 }));

		struct Assembly : JITASM::JITASM
		{
			Assembly(std::uintptr_t a_targetAddr) :
				JITASM(ISKSE::GetLocalTrampoline())
			{
				Xbyak::Label retnLabel;

				db(reinterpret_cast<Xbyak::uint8*>(a_targetAddr), 0x7);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_targetAddr + 0x7);
			}
		};

		LogPatchBegin();
		{
			Assembly code(m_createWeaponNodes_a.get());
			m_createWeaponNodes_o = code.get<decltype(m_createWeaponNodes_o)>();
			ISKSE::GetBranchTrampoline().Write6Branch(
				m_createWeaponNodes_a.get(),
				std::uintptr_t(CreateWeaponNodes_Hook));
		}
		LogPatchEnd();
	}

	// actually blocks the node from havok entirely
	void EngineExtensions::Patch_WeaponAdjustDisable()
	{
		ASSERT_STR(
			Patching::validate_mem(
				m_hkaLookupSkeletonBones_a.get(),
				{ 0xE8 }),
			mv_failstr);

		struct Assembly : JITASM::JITASM
		{
			Assembly(std::uintptr_t a_targetAddr) :
				JITASM(ISKSE::GetLocalTrampoline())
			{
				Xbyak::Label callLabel;
				Xbyak::Label retnLabel;

				mov(r9, IAL::IsAE() ? r13 : r15);
				call(ptr[rip + callLabel]);
				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_targetAddr + 0x5);

				L(callLabel);
				dq(std::uintptr_t(hkaLookupSkeletonNode_Hook));
			}
		};

		LogPatchBegin();
		{
			Assembly code(m_hkaLookupSkeletonBones_a.get());

			ISKSE::GetBranchTrampoline().Write5Branch(
				m_hkaLookupSkeletonBones_a.get(),
				code.get());
		}
		LogPatchEnd();
	}

	void EngineExtensions::Hook_ToggleFav()
	{
		if (hook::call5(
				ISKSE::GetBranchTrampoline(),
				m_toggleFav1_a.get(),
				std::uintptr_t(ToggleFavGetExtraList_Hook),
				m_toggleFavGetExtraList_o))
		{
			Debug("[%s] Installed", __FUNCTION__);
		}
		else
		{
			Error("[%s] Failed", __FUNCTION__);
		}
	}

	void EngineExtensions::Hook_ProcessEffectShaders()
	{
		if (hook::call5(
				ISKSE::GetBranchTrampoline(),
				m_processEffectShaders_a.get(),
				std::uintptr_t(ProcessEffectShaders_Hook),
				m_processEffectShaders_o))
		{
			Debug("[%s] Installed", __FUNCTION__);
		}
		else
		{
			Error("[%s] Failed", __FUNCTION__);
		}
	}

	/*void EngineExtensions::Patch_CorpseScatter()
	{
		ASSERT_STR(
			Patching::validate_mem(
				m_bipedAttachHavok_a,
				{ 0x41, 0xB1, 0x01, 0x41, 0xB8, 0x04, 0x00, 0x00, 0x00 }),
			mv_failstr);

		struct Assembly : JITASM::JITASM
		{
			Assembly(
				std::uintptr_t a_targetAddr) :
				JITASM(ISKSE::GetLocalTrampoline())
			{
				Xbyak::Label callLabel;
				Xbyak::Label retnLabel;

				mov(rcx, rbx);  // actor

				if (IAL::IsAE())
				{
					mov(edx, r15d);  // slot
					sub(edx, 0xFFFFFFE0);
				}
				else
				{
					mov(edx, r14d);  // slot
				}

				call(ptr[rip + callLabel]);

				mov(r8d, eax);
				mov(r9b, 1);

				jmp(ptr[rip + retnLabel]);

				L(retnLabel);
				dq(a_targetAddr + 0x9);

				L(callLabel);
				dq(std::uintptr_t(Biped_QueueAttachHavok_Hook));
			}
		};

		LogPatchBegin();
		{
			Assembly code(m_bipedAttachHavok_a);
			ISKSE::GetBranchTrampoline().Write6Branch(
				m_bipedAttachHavok_a,
				code.get());
		}
		LogPatchEnd();
	}*/

	void EngineExtensions::RemoveAllBipedParts_Hook(Biped* a_biped)
	{
		{
			NiPointer<TESObjectREFR> ref;

			if (a_biped->handle.Lookup(ref) && ref->formID)
			{
				if (auto actor = ref->As<Actor>())
				{
					if (ITaskPool::IsRunningOnCurrentThread())
					{
						m_Instance.FailsafeCleanupAndEval(actor);
					}
					else
					{
						m_Instance.m_controller->RemoveActor(actor, a_biped->handle, ControllerUpdateFlags::kNone);
						m_Instance.m_controller->QueueEvaluate(actor, ControllerUpdateFlags::kNone);
					}
				}
			}
		}

		m_Instance.m_removeAllBipedParts_o(a_biped);
	}

	void EngineExtensions::Character_Resurrect_Hook(
		Character* a_actor,
		bool       a_resetInventory,
		bool       a_attach3D)
	{
		if (a_attach3D)
		{
			m_Instance.m_controller->QueueReset(a_actor, ControllerUpdateFlags::kNone);

			//_DMESSAGE("resurrect %X", a_actor->formID.get());
		}

		m_Instance.m_characterResurrect_o(a_actor, a_resetInventory, a_attach3D);
	}

	void EngineExtensions::Actor_Release3D_Hook(
		Actor* a_actor)
	{
		bool eval = false;

		if (a_actor->formID)
		{
			if (ITaskPool::IsRunningOnCurrentThread())
			{
				m_Instance.FailsafeCleanupAndEval(a_actor);
			}
			else
			{
				m_Instance.m_controller->RemoveActor(a_actor, ControllerUpdateFlags::kNone);

				eval = true;
			}
		}

		m_Instance.m_actorRelease3D_o(a_actor);

		if (eval)
		{
			m_Instance.m_controller->QueueEvaluate(a_actor, ControllerUpdateFlags::kNone);
		}
	}

	void EngineExtensions::Character_Release3D_Hook(
		Character* a_actor)
	{
		bool eval = false;

		if (a_actor->formID)
		{
			if (ITaskPool::IsRunningOnCurrentThread())
			{
				m_Instance.FailsafeCleanupAndEval(a_actor);
			}
			else
			{
				m_Instance.m_controller->RemoveActor(a_actor, ControllerUpdateFlags::kNone);

				eval = true;
			}
		}

		m_Instance.m_characterRelease3D_o(a_actor);

		if (eval)
		{
			m_Instance.m_controller->QueueEvaluate(a_actor, ControllerUpdateFlags::kNone);
		}
	}

	void EngineExtensions::FailsafeCleanupAndEval(
		Actor*                     a_actor,
		const std::source_location a_loc)
	{
		ITaskPool::AddTask([this, fid = a_actor->formID, handle = a_actor->GetHandle()]() {
			m_controller->RemoveActor(fid, ControllerUpdateFlags::kNone);
			m_controller->QueueEvaluate(handle, ControllerUpdateFlags::kNone);
		});

		Debug("[%.8X] [%s]: called from ITaskPool", a_actor->formID.get(), a_loc.function_name());
	}

	void EngineExtensions::ReanimateActorStateUpdate_Hook(
		Actor* a_actor,
		bool   a_unk1)
	{
		m_Instance.m_ReanimActorStateUpd_o(a_actor, a_unk1);

		if (a_actor->actorState1.lifeState ==
		    ActorState::ACTOR_LIFE_STATE::kReanimate)
		{
			m_Instance.m_controller->QueueReset(a_actor, ControllerUpdateFlags::kNone);

			//_DMESSAGE("reanimate %X", a_actor->formID.get());
		}
	}

	void EngineExtensions::ArmorUpdate_Hook(
		Game::InventoryChanges* a_ic,
		Game::InitWornVisitor&  a_visitor)
	{
		auto formid = a_visitor.actor ?
		                  a_visitor.actor->formID :
                          0;

		m_Instance.m_ArmorChange_o(a_ic, a_visitor);

		if (formid)
		{
			m_Instance.m_controller->QueueRequestEvaluate(formid, false, true);
		}
	}

	bool EngineExtensions::GarbageCollectorReference_Hook(TESObjectREFR* a_refr)
	{
		if (auto actor = a_refr->As<Actor>())
		{
			m_Instance.m_controller->RemoveActor(actor, ControllerUpdateFlags::kNone);
		}

		return m_Instance.m_garbageCollectorReference_o(a_refr);
	}

	bool EngineExtensions::SetWeapAdjAnimVar_Hook(
		TESObjectREFR*       a_refr,
		const BSFixedString& a_animVarName,
		float                a_val,
		Biped*               a_biped)
	{
		if (m_Instance.m_conf.weaponAdjustFix)
		{
			auto& biped3p = a_refr->GetBiped1(false);
			if (!biped3p || biped3p.get() != a_biped)
			{
				return false;
			}
		}

		return a_refr->SetVariableOnGraphsFloat(a_animVarName, a_val);
	}

	BaseExtraList* EngineExtensions::ToggleFavGetExtraList_Hook(TESObjectREFR* a_actor)
	{
		m_Instance.m_controller->QueueRequestEvaluate(a_actor->formID, true, false);

		return m_Instance.m_toggleFavGetExtraList_o(a_actor);
	}

	void EngineExtensions::ProcessEffectShaders_Hook(
		Game::ProcessLists* a_pl,
		float               a_frameTimerSlow)
	{
		m_Instance.m_processEffectShaders_o(a_pl, a_frameTimerSlow);

		m_Instance.m_controller->ProcessEffectShaders();
	}

	/*std::uint32_t EngineExtensions::Biped_QueueAttachHavok_Hook(
		TESObjectREFR* a_actor,
		BIPED_OBJECT   a_slot)
	{
		std::uint32_t result = 4;

		if (a_actor && a_slot != BIPED_OBJECT::kNone)
		{
			if (auto actor = a_actor->As<Actor>())
			{
				if ((a_slot >= BIPED_OBJECT::kOneHandSword &&
				     a_slot <= BIPED_OBJECT::kCrossbow) ||
				    a_slot == actor->GetShieldBipedObject())
				{
					if (actor->IsDead() && actor->GetNiNode())
					{
						result = 1;
					}
				}
			}
		}

		return result;
	}*/

	bool EngineExtensions::hkaLookupSkeletonNode_Hook(
		NiNode*                   a_root,
		const BSFixedString&      a_name,
		hkaGetSkeletonNodeResult& a_result,
		const RE::hkaSkeleton&    a_hkaSkeleton)
	{
		if (a_hkaSkeleton.name &&
		    _stricmp(a_hkaSkeleton.name, StringHolder::HK_NPC_ROOT) == 0)
		{
			if (auto sh = m_Instance.m_controller->GetBSStringHolder())
			{
				if (a_name == sh->m_weaponAxe ||
				    a_name == sh->m_weaponMace ||
				    a_name == sh->m_weaponSword ||
				    a_name == sh->m_weaponDagger ||
				    a_name == sh->m_weaponBack ||
				    a_name == sh->m_weaponBow ||
				    a_name == sh->m_quiver)
				{
					a_result.root  = nullptr;
					a_result.unk08 = std::numeric_limits<std::uint32_t>::max();

					return false;
				}
			}
		}

		return hkaGetSkeletonNode(a_root, a_name, a_result);
	}

	void EngineExtensions::CreateWeaponNodes_Hook(
		TESObjectREFR* a_actor,
		TESForm*       a_object,
		bool           a_left)
	{
		m_Instance.m_createWeaponNodes_o(a_actor, a_object, a_left);

		if (a_actor)
		{
			m_Instance.m_controller->QueueRequestEvaluate(a_actor->formID, false, true, true);
		}
	}

	bool EngineExtensions::RemoveAllChildren(
		NiNode*              a_object,
		const BSFixedString& a_name)
	{
		bool result = false;

		// some massive paranoia
		std::uint32_t maxiter = 1000;

		while (auto object = GetObjectByName(a_object, a_name, true))
		{
			object->m_parent->RemoveChild(object);
			result = true;

			if (!--maxiter)
			{
				break;
			}
		}

		return result;
	}

	auto EngineExtensions::AttachObject(
		Actor*    a_actor,
		NiNode*   a_root,
		NiNode*   a_targetNode,
		NiNode*   a_object,
		ModelType a_modelType,
		bool      a_leftWeapon,
		bool      a_shield,
		bool      a_dropOnDeath,
		bool      a_removeScabbards,
		bool      a_keepTorchFlame,
		bool      a_disableHavok)
		-> stl::flag<AttachResultFlags>
	{
		stl::flag<AttachResultFlags> result{
			AttachResultFlags::kNone
		};

		if (a_disableHavok)
		{
			::Util::Node::Traverse(
				a_object,
				[&](NiAVObject* a_object) {
					a_object->collisionObject.reset();
					return ::Util::Node::VisitorControl::kContinue;
				});

			a_dropOnDeath = false;
		}

		if (auto bsxFlags = GetBSXFlags(a_object))
		{
			stl::flag<BSXFlags::Flag> flags(bsxFlags->m_data);

			if (a_disableHavok &&
			    flags.test(BSXFlags::kHavok))
			{
				// recreate since this isn't cloned

				NiPointer newbsx = BSXFlags::Create(
					flags.value & ~BSXFlags::Flag::kHavok);

				if (auto index = a_object->GetIndexOf(newbsx->m_pcName); index >= 0)
				{
					if (auto& entry = a_object->m_extraData[index]; entry == bsxFlags)
					{
						newbsx->IncRef();
						entry->DecRef();
						entry = newbsx;

						flags    = newbsx->m_data;
						bsxFlags = newbsx;
					}
				}
			}

			if (flags.test(BSXFlags::Flag::kAddon))
			{
				fUnk28BAD0(a_object);
			}
		}

		AttachAddonNodes(a_object);

		SetRootOnShaderProperties(a_object, a_root);

		a_targetNode->AttachChild(a_object, true);

		NiAVObject::ControllerUpdateContext ctx{
			static_cast<float>(*m_unkglob0),
			0
		};

		a_object->UpdateDownwardPass(ctx, nullptr);

		a_object->IncRef();

		fUnk12ba3e0(*m_shadowSceneNode, a_object);
		fUnk12b99f0(*m_shadowSceneNode, a_object);

		a_object->DecRef();

		auto sh = m_Instance.m_controller->GetBSStringHolder();

		a_object->m_name = sh->m_object;

		std::uint32_t collisionFilterInfo = 0;

		if (a_modelType == ModelType::kWeapon)
		{
			auto scbNode     = GetObjectByName(a_object, sh->m_scb, true);
			auto scbLeftNode = GetObjectByName(a_object, sh->m_scbLeft, true);

			if (a_removeScabbards)
			{
				if (scbNode || scbLeftNode)
				{
					if (scbNode)
					{
						scbNode->m_parent->RemoveChild(scbNode);
					}

					if (scbLeftNode)
					{
						scbLeftNode->m_parent->RemoveChild(scbLeftNode);
					}

					ShrinkToSize(a_object);
				}
			}
			else
			{
				NiAVObject* scbAttach;
				NiAVObject* scbRemove;

				if (a_leftWeapon && scbLeftNode)
				{
					scbAttach = scbLeftNode;
					scbRemove = scbNode;

					scbLeftNode->ClearHidden();

					result.set(AttachResultFlags::kScbLeft);
				}
				else
				{
					scbAttach = scbNode;
					scbRemove = scbLeftNode;
				}

				if (scbAttach || scbRemove)
				{
					if (scbAttach)
					{
						a_targetNode->AttachChild(scbAttach, true);

						fUnkDC6140(a_targetNode, true);
					}

					if (scbRemove)
					{
						scbRemove->m_parent->RemoveChild(scbRemove);
					}

					ShrinkToSize(a_object);
				}
			}

			collisionFilterInfo = a_leftWeapon ? 0x12 : 0x14;
		}
		else if (a_modelType == ModelType::kLight)
		{
			if (!a_keepTorchFlame)
			{
				bool shrink = false;

				if (auto node = GetObjectByName(a_object, sh->m_torchFire, true))
				{
					node->m_parent->RemoveChild(node);
					shrink = true;

					result.set(AttachResultFlags::kTorchFlameRemoved);
				}

				bool custRemoved = false;

				custRemoved |= RemoveAllChildren(a_object, sh->m_mxTorchSmoke);
				custRemoved |= RemoveAllChildren(a_object, sh->m_mxTorchSparks);
				custRemoved |= RemoveAllChildren(a_object, sh->m_mxAttachSmoke);
				custRemoved |= RemoveAllChildren(a_object, sh->m_mxAttachSparks);
				custRemoved |= RemoveAllChildren(a_object, sh->m_attachENBLight);
				custRemoved |= RemoveAllChildren(a_object, sh->m_enbFireLightEmitter);
				custRemoved |= RemoveAllChildren(a_object, sh->m_enbTorchLightEmitter);

				if (custRemoved)
				{
					result.set(AttachResultFlags::kTorchCustomRemoved);
				}

				shrink |= custRemoved;

				if (shrink)
				{
					ShrinkToSize(a_object);
				}
			}

			collisionFilterInfo = 0x12;
		}
		else if (a_modelType == ModelType::kArmor)
		{
			if (a_shield)
			{
				collisionFilterInfo = 0x12;
			}
		}

		fUnk1CD130(a_object, collisionFilterInfo);

		QueueAttachHavok(
			BSTaskPool::GetSingleton(),
			a_object,
			a_dropOnDeath ? 4 : 0,
			true);

		if (auto cell = a_actor->GetParentCell())
		{
			if (auto world = cell->GetHavokWorld())
			{
				NiPointer<Actor> mountedActor;

				bool isMounted = a_actor->GetMountedActor(mountedActor);

				unks_01 tmp;

				auto& r = fUnk5EBD90(isMounted ? mountedActor.get() : a_actor, tmp);
				fUnk5C39F0(BSTaskPool::GetSingleton(), a_object, world, r.p2);
			}
		}

		SetRootOnShaderProperties(a_targetNode, a_root);

		a_actor->UpdateAlpha();

		return result;
	}

	bool EngineExtensions::CreateWeaponBehaviorGraph(
		NiAVObject*                               a_object,
		RE::WeaponAnimationGraphManagerHolderPtr& a_out)
	{
		auto sh = m_Instance.m_controller->GetBSStringHolder();

		auto bged = a_object->GetExtraData<BSBehaviorGraphExtraData>(sh->m_bged);
		if (!bged)
		{
			return false;
		}

		if (bged->controlsBaseSkeleton)
		{
			return false;
		}

		auto result = RE::WeaponAnimationGraphManagerHolder::Create();

		if (!LoadWeaponGraph(
				*result,
				bged->behaviorGraphFile.c_str()))
		{
			return false;
		}

		a_out = std::move(result);

		if (!BindAnimationObject(*a_out, a_object))
		{
			return false;
		}

		/*if (Game::IsPaused())
		{
			BSAnimationUpdateData data{ std::numeric_limits<float>::epsilon() };
			data.forceUpdate = true;
			a_out->Update(data);
		}*/

		return true;
	}

	void EngineExtensions::CleanupWeaponBehaviorGraph(
		RE::WeaponAnimationGraphManagerHolderPtr& a_graph)
	{
		RE::BSAnimationGraphManagerPtr manager;
		if (a_graph->GetAnimationGraphManagerImpl(manager))
		{
			auto gc = RE::GarbageCollector::GetSingleton();
			assert(gc);
			gc->QueueBehaviorGraph(manager);
		}
	}

	void EngineExtensions::UpdateRoot(NiNode* a_root)
	{
		a_root->UpdateWorldBound();

		NiAVObject::ControllerUpdateContext ctx{ 0, 0x2000 };
		a_root->Update(ctx);

		fUnk12BAFB0(*m_shadowSceneNode, a_root, false);
	}

	void EngineExtensions::SetDropOnDeath(
		Actor*      a_actor,
		NiAVObject* a_object,
		bool        a_switch)
	{
	}

	void EngineExtensions::CleanupObject(
		Game::ObjectRefHandle a_handle,
		NiAVObject*           a_object,
		NiNode*               a_root)
	{
		if (!SceneRendering() &&
		    ITaskPool::IsRunningOnCurrentThread())
		{
			CleanupObjectImpl(a_handle, a_object);
		}
		else
		{
			//BSTaskPool::GetSingleton()->QueueCleanupNode(a_handle, a_object);

			class NodeCleanupTask :
				public TaskDelegate
			{
			public:
				inline NodeCleanupTask(
					Game::ObjectRefHandle a_handle,
					NiAVObject*           a_object,
					NiNode*               a_root) :
					m_handle(a_handle),
					m_object(a_object),
					m_root(a_root)
				{
				}

				virtual void Run() override
				{
					CleanupObjectImpl(m_handle, m_object);

					m_object.reset();
					m_root.reset();
				}

				virtual void Dispose() override
				{
					delete this;
				}

			private:
				Game::ObjectRefHandle m_handle;
				NiPointer<NiAVObject> m_object;
				NiPointer<NiNode>     m_root;
			};

			ITaskPool::AddPriorityTask<NodeCleanupTask>(
				a_handle,
				a_object,
				a_root);
		}
	}

	BSXFlags* EngineExtensions::GetBSXFlags(NiObjectNET* a_object)
	{
		auto sh = m_Instance.m_controller->GetBSStringHolder();

		return a_object->GetExtraData<BSXFlags>(sh->m_bsx);
	}

}
#include "pch.h"

#include "UISlotEditorRace.h"

#include "IED/Controller/Controller.h"

namespace IED
{
	namespace UI
	{
		UISlotEditorRace::UISlotEditorRace(Controller& a_controller) :
			UISlotEditorCommon<Game::FormID>(a_controller),
			UIRaceList<entrySlotData_t>(), m_controller(a_controller),
			UITipsInterface(a_controller),
			UILocalizationInterface(a_controller)
		{}

		void UISlotEditorRace::Initialize()
		{
			InitializeProfileBase();

			auto& store = m_controller.GetConfigStore();

			SetSex(store.settings.data.ui.slotEditor.raceConfig.sex, false);
		}

		void UISlotEditorRace::Draw()
		{
			if (ImGui::BeginChild("slot_editor_race", { -1.0f, 0.0f }))
			{
				ListTick();

				auto entry = ListGetSelected();
				const char* curSelName{ nullptr };

				ImGui::Spacing();
				ListDraw(entry, curSelName);
				ImGui::Separator();
				ImGui::Spacing();

				if (entry)
				{
					DrawSlotEditor(entry->handle, entry->data);
				}
			}

			ImGui::EndChild();
		}

		constexpr Data::ConfigClass UISlotEditorRace::GetConfigClass() const
		{
			return Data::ConfigClass::Race;
		}

		Data::SettingHolder::EditorPanelRaceSettings& UISlotEditorRace::GetRaceSettings() const
		{
			return m_controller.GetConfigStore().settings.data.ui.slotEditor.raceConfig;
		}

		auto UISlotEditorRace::GetCurrentData() -> SlotEditorCurrentData
		{
			auto entry = ListGetSelected();
			if (entry)
			{
				return { entry->handle, std::addressof(entry->data) };
			}
			else
			{
				return { Game::FormID(), nullptr };
			}
		}

		Data::SettingHolder::EditorPanelCommon& UISlotEditorRace::GetEditorPanelSettings()
		{
			return m_controller.GetConfigStore().settings.data.ui.slotEditor;
		}

		void UISlotEditorRace::OnEditorPanelSettingsChange()
		{
			auto& store = m_controller.GetConfigStore();
			store.settings.MarkDirty();
		}

		void UISlotEditorRace::ListResetAllValues(
			Game::FormID a_handle)
		{}

		auto UISlotEditorRace::GetData(Game::FormID a_handle)
			-> const entrySlotData_t&
		{
			auto& store = m_controller.GetConfigStore().active;

			m_tmpData = store.slot.GetRaceCopy(a_handle);

			return m_tmpData;
		}

		auto UISlotEditorRace::GetOrCreateConfigSlotHolder(
			Game::FormID a_handle) const
			-> Data::configSlotHolder_t&
		{
			auto& store = m_controller.GetConfigStore().active;
			auto& data = store.slot.GetRaceData();

			return data.try_emplace(static_cast<Data::configForm_t>(a_handle)).first->second;
		}

		void UISlotEditorRace::MergeProfile(
			profileSelectorParamsSlot_t<Game::FormID>& a_data,
			const SlotProfile& a_profile)
		{
			UpdateConfigFromProfile(a_data.handle, a_profile.Data(), true);

			a_data.data = GetData(a_data.handle);

			ResetFormSelectorWidgets();

			m_controller.QueueResetRace(
				a_data.handle,
				ControllerUpdateFlags::kNone);
		}

		void UISlotEditorRace::OnBaseConfigChange(
			Game::FormID a_handle,
			const void* a_params,
			PostChangeAction a_action)
		{
			auto params = static_cast<const SingleSlotConfigUpdateParams*>(a_params);

			auto& store = m_controller.GetConfigStore();

			UpdateConfigSingleSlot(
				a_handle,
				params,
				store.settings.data.ui.slotEditor.sexSync);

			switch (a_action)
			{
			case PostChangeAction::Evaluate:
				m_controller.QueueEvaluateRace(
					a_handle,
					ControllerUpdateFlags::kNone);
				break;
			case PostChangeAction::Reset:
				m_controller.QueueResetRace(
					a_handle,
					ControllerUpdateFlags::kNone,
					params->slot);
				break;
			case PostChangeAction::UpdateTransform:
				m_controller.QueueUpdateTransformSlotRace(
					a_handle,
					params->slot);
				break;
			case PostChangeAction::AttachNode:
				m_controller.QueueAttachSlotNodeRace(
					a_handle,
					params->slot,
					true);
				break;
			}
		}

		void UISlotEditorRace::OnFullConfigChange(
			Game::FormID a_handle,
			const SlotConfigUpdateParams& a_params)
		{
			UpdateConfig(a_handle, a_params.data);

			ResetFormSelectorWidgets();

			m_controller.QueueResetRace(a_handle, ControllerUpdateFlags::kNone);
		}

		void UISlotEditorRace::OnSingleSlotClear(
			Game::FormID a_handle,
			const void* a_params)
		{
			auto params = static_cast<const SingleSlotConfigUpdateParams*>(a_params);

			auto& store = m_controller.GetConfigStore().active;

			ResetConfigSlot(a_handle, params->slot, store.slot.GetRaceData());
			QueueListUpdateCurrent();

			m_controller.QueueResetRace(a_handle, ControllerUpdateFlags::kNone, params->slot);
		}

		void UISlotEditorRace::OnFullConfigClear(
			Game::FormID a_handle)
		{
			auto& store = m_controller.GetConfigStore().active;

			ResetConfig(a_handle, store.slot.GetRaceData());
			QueueListUpdateCurrent();

			m_controller.QueueResetRace(a_handle, ControllerUpdateFlags::kNone);
		}

		void UISlotEditorRace::OnSexChanged(
			Data::ConfigSex a_newSex)
		{
			auto& store = m_controller.GetConfigStore();

			if (store.settings.data.ui.slotEditor.raceConfig.sex != a_newSex)
			{
				ResetFormSelectorWidgets();
				store.settings.Set(
					store.settings.data.ui.slotEditor.raceConfig.sex,
					a_newSex);
			}
		}

		void UISlotEditorRace::OnListOptionsChange()
		{
			auto& store = m_controller.GetConfigStore();
			store.settings.MarkDirty();
		}

		const ActorInfoHolder& UISlotEditorRace::GetActorInfoHolder() const
		{
			return m_controller.GetActorInfo();
		}

		const SetObjectWrapper<Game::FormID>& UISlotEditorRace::GetCrosshairRef()
		{
			return m_controller.GetCrosshairRef();
		}

		UIPopupQueue& UISlotEditorRace::GetPopupQueue()
		{
			return m_controller.UIGetPopupQueue();
		}

		UIPopupQueue& UISlotEditorRace::GetPopupQueue_ProfileBase() const
		{
			return m_controller.UIGetPopupQueue();
		}

		UIData::UICollapsibleStates& UISlotEditorRace::GetCollapsibleStatesData()
		{
			auto& config = m_controller.GetConfigStore().settings;

			return config.data.ui.slotEditor
			    .colStates[stl::underlying(Data::ConfigClass::Race)];
		}

		void UISlotEditorRace::OnCollapsibleStatesUpdate()
		{
			m_controller.GetConfigStore().settings.MarkDirty();
		}

		void UISlotEditorRace::OnOpen()
		{
			Reset();
		}

		void UISlotEditorRace::OnClose()
		{
			Reset();
		}

		void UISlotEditorRace::Reset()
		{
			ListReset();
			ResetFormSelectorWidgets();
		}

		void UISlotEditorRace::QueueUpdateCurrent()
		{
			QueueListUpdateCurrent();
		}
	}
}
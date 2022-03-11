#pragma once

#include "IED/UI/UITips.h"

#include "UIDescriptionPopup.h"
#include "UIPopupToggleButtonWidget.h"

#include "IED/ConfigOverrideEffectShader.h"

#include "UIEffectShaderEditorWidgetStrings.h"

#include "IED/UI/UILocalizationInterface.h"

namespace IED
{
	class Controller;

	namespace UI
	{
		enum class EffectShaderContextAction
		{
			None,
			Delete,
			PasteOver,
			Add
		};

		struct EffectShaderEntryResult
		{
			EffectShaderContextAction action{ EffectShaderContextAction::None };
		};

		template <class T>
		class UIEffectShaderEditorWidget :
			public virtual UIDescriptionPopupWidget,
			public virtual UITipsInterface,
			public virtual UILocalizationInterface
		{
		public:
			UIEffectShaderEditorWidget(Controller& a_controller);

			void DrawEffectShaderEditor(
				const T&                          a_params,
				Data::configEffectShaderHolder_t& a_data);

		private:
			EffectShaderContextAction DrawEffectShaderEntryContextMenu(
				const T&                        a_params,
				Data::configEffectShaderData_t& a_data);

			void DrawEffectShaderEntries(
				const T&                          a_params,
				Data::configEffectShaderHolder_t& a_data);

			void DrawEffectShaderEntry(
				const T&                        a_params,
				Data::configEffectShaderData_t& a_data);

			void DrawTextureTree(
				const char*                        a_id,
				const char*                        a_title,
				const T&                           a_params,
				Data::configEffectShaderTexture_t& a_data,
				const bool                         a_mustBeSet);

			void DrawEffectShaderFlags(
				const T&                        a_params,
				Data::configEffectShaderData_t& a_data);

			EffectShaderContextAction DrawTargetNodesContextMenu(
				const T&                      a_params,
				Data::configFixedStringSet_t& a_data);

			void DrawTargetNodes(
				const T&                      a_params,
				Data::configFixedStringSet_t& a_data);

			void DrawTargetNodeTable(
				const T&                      a_params,
				Data::configFixedStringSet_t& a_data);

			virtual void OnEffectShaderUpdate(const T& a_params) = 0;

			char m_inputBuffer1[MAX_PATH]{ 0 };
		};

		template <class T>
		UIEffectShaderEditorWidget<T>::UIEffectShaderEditorWidget(
			Controller& a_controller)
		{
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawEffectShaderEditor(
			const T&                          a_params,
			Data::configEffectShaderHolder_t& a_data)
		{
			ImGui::PushID("es_editor");

			ImGui::PushID("header");

			if (ImGui::CheckboxFlagsT(
					LS(CommonStrings::Enabled, "1"),
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderHolderFlags::kEnabled)))
			{
				OnEffectShaderUpdate(a_params);
			}

			ImGui::PopID();

			ImGui::Spacing();

			bool disabled = !a_data.enabled();

			UICommon::PushDisabled(disabled);

			DrawEffectShaderEntries(a_params, a_data);

			UICommon::PopDisabled(disabled);

			ImGui::PopID();
		}

		template <class T>
		EffectShaderContextAction UIEffectShaderEditorWidget<T>::DrawEffectShaderEntryContextMenu(
			const T&                        a_params,
			Data::configEffectShaderData_t& a_data)
		{
			EffectShaderContextAction result{ EffectShaderContextAction::None };

			ImGui::PushID("tn_context_area");

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 1.0f });

			UIPopupToggleButtonWidget::DrawPopupToggleButton("open", "context_menu");

			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

			ImGui::PopStyleVar();

			if (ImGui::BeginPopup("context_menu"))
			{
				if (ImGui::MenuItem(LS(CommonStrings::Delete, "1")))
				{
					result = EffectShaderContextAction::Delete;
				}

				ImGui::Separator();

				if (ImGui::MenuItem(LS(CommonStrings::Copy, "2")))
				{
					UIClipboard::Set(a_data);
				}

				auto clipData = UIClipboard::Get<Data::configEffectShaderData_t>();

				if (ImGui::MenuItem(
						LS(CommonStrings::PasteOver, "3"),
						nullptr,
						false,
						clipData != nullptr))
				{
					if (clipData)
					{
						a_data = *clipData;

						OnEffectShaderUpdate(a_params);

						result = EffectShaderContextAction::PasteOver;
					}
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();

			return result;
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawEffectShaderEntries(
			const T&                          a_params,
			Data::configEffectShaderHolder_t& a_data)
		{
			if (a_data.data.empty())
			{
				return;
			}

			ImGui::PushID("es_entries");

			auto it = a_data.data.begin();

			while (it != a_data.data.end())
			{
				ImGui::PushID(it->first.c_str());

				const auto result = DrawEffectShaderEntryContextMenu(
					a_params,
					it->second);

				switch (result)
				{
				case EffectShaderContextAction::Delete:

					it = a_data.data.erase(it);
					OnEffectShaderUpdate(a_params);
					break;

				case EffectShaderContextAction::PasteOver:

					OnEffectShaderUpdate(a_params);
					ImGui::SetNextItemOpen(true);
					break;
				}

				if (it != a_data.data.end())
				{
					if (ImGui::TreeNodeEx(
							"entry",
							ImGuiTreeNodeFlags_DefaultOpen |
								ImGuiTreeNodeFlags_SpanAvailWidth,
							"%s",
							it->first.c_str()))
					{
						ImGui::Spacing();

						DrawEffectShaderEntry(a_params, it->second);

						ImGui::Spacing();

						ImGui::TreePop();
					}

					++it;
				}

				ImGui::PopID();
			}

			ImGui::PopID();
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawEffectShaderEntry(
			const T&                        a_params,
			Data::configEffectShaderData_t& a_data)
		{
			DrawTextureTree(
				"1",
				LS(UIEffectShaderEditorWidgetStrings::BaseTexture),
				a_params,
				a_data.baseTexture,
				true);

			DrawTextureTree(
				"2",
				LS(UIEffectShaderEditorWidgetStrings::PalleteTexture),
				a_params,
				a_data.paletteTexture,
				false);

			DrawTextureTree(
				"3",
				LS(UIEffectShaderEditorWidgetStrings::BlockOutTexture),
				a_params,
				a_data.blockOutTexture,
				false);

			ImGui::Spacing();

			if (ImGui::ColorEdit4(
					LS(UIEffectShaderEditorWidgetStrings::FillColor, "4"),
					a_data.fillColor,
					ImGuiColorEditFlags_AlphaBar))
			{
				a_data.fillColor.clamp();

				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::ColorEdit4(
					LS(UIEffectShaderEditorWidgetStrings::RimColor, "5"),
					a_data.rimColor,
					ImGuiColorEditFlags_AlphaBar))
			{
				a_data.rimColor.clamp();

				OnEffectShaderUpdate(a_params);
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			DrawEffectShaderFlags(a_params, a_data);

			ImGui::Spacing();

			DrawTargetNodes(a_params, a_data.targetNodes);
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawTextureTree(
			const char*                        a_id,
			const char*                        a_title,
			const T&                           a_params,
			Data::configEffectShaderTexture_t& a_data,
			const bool                         a_mustBeSet)
		{
			if (ImGui::TreeNodeEx(
					a_id,
					ImGuiTreeNodeFlags_DefaultOpen |
						ImGuiTreeNodeFlags_SpanAvailWidth,
					"%s",
					a_title))
			{
				ImGui::Spacing();

				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s:", LS(CommonStrings::Texture));
				ImGui::SameLine();

				UICommon::PushDisabled(a_mustBeSet);

				if (ImGui::RadioButton(
						LS(CommonStrings::None, "0"),
						a_data.fbf.selected == Data::EffectShaderSelectedTexture::None))
				{
					a_data.fbf.selected = Data::EffectShaderSelectedTexture::None;
					OnEffectShaderUpdate(a_params);
				}

				UICommon::PopDisabled(a_mustBeSet);

				ImGui::SameLine();

				if (ImGui::RadioButton(
						LS(CommonStrings::Black, "1"),
						a_data.fbf.selected == Data::EffectShaderSelectedTexture::Black))
				{
					a_data.fbf.selected = Data::EffectShaderSelectedTexture::Black;
					OnEffectShaderUpdate(a_params);
				}

				ImGui::SameLine();

				if (ImGui::RadioButton(
						LS(CommonStrings::White, "2"),
						a_data.fbf.selected == Data::EffectShaderSelectedTexture::White))
				{
					a_data.fbf.selected = Data::EffectShaderSelectedTexture::White;
					OnEffectShaderUpdate(a_params);
				}

				ImGui::SameLine();

				if (ImGui::RadioButton(
						LS(CommonStrings::Grey, "3"),
						a_data.fbf.selected == Data::EffectShaderSelectedTexture::Grey))
				{
					a_data.fbf.selected = Data::EffectShaderSelectedTexture::Grey;
					OnEffectShaderUpdate(a_params);
				}

				ImGui::SameLine();

				if (ImGui::RadioButton(
						LS(CommonStrings::Custom, "4"),
						a_data.fbf.selected == Data::EffectShaderSelectedTexture::Custom))
				{
					a_data.fbf.selected = Data::EffectShaderSelectedTexture::Custom;
					OnEffectShaderUpdate(a_params);
				}

				ImGui::Spacing();

				if (a_data.fbf.selected == Data::EffectShaderSelectedTexture::Custom)
				{
					auto l = a_data.path.get().copy(
						m_inputBuffer1,
						sizeof(m_inputBuffer1) - 1);

					m_inputBuffer1[l] = 0;

					if (ImGui::InputText(
							LS(CommonStrings::Texture, "5"),
							m_inputBuffer1,
							sizeof(m_inputBuffer1),
							ImGuiInputTextFlags_EnterReturnsTrue))
					{
						a_data.path = m_inputBuffer1;
						OnEffectShaderUpdate(a_params);
					}
				}

				ImGui::Spacing();

				ImGui::TreePop();
			}
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawEffectShaderFlags(
			const T&                        a_params,
			Data::configEffectShaderData_t& a_data)
		{
			ImGui::PushID("flags");

			ImGui::Columns(2, nullptr, false);

			if (ImGui::CheckboxFlagsT(
					LS(CommonStrings::Yield, "1"),
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kYield)))
			{
				OnEffectShaderUpdate(a_params);
			}

			ImGui::Columns();

			ImGui::Spacing();

			ImGui::Columns(2, nullptr, false);

			if (ImGui::CheckboxFlagsT(
					"kGrayscaleToColor",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kGrayscaleToColor)))
			{
				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::CheckboxFlagsT(
					"kGrayscaleToAlpha",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kGrayscaleToAlpha)))
			{
				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::CheckboxFlagsT(
					"kIgnoreTextureAlpha",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kIgnoreTextureAlpha)))
			{
				OnEffectShaderUpdate(a_params);
			}

			ImGui::NextColumn();

			if (ImGui::CheckboxFlagsT(
					"kBaseTextureProjectedUVs",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kBaseTextureProjectedUVs)))
			{
				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::CheckboxFlagsT(
					"kIgnoreBaseGeomTexAlpha",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kIgnoreBaseGeomTexAlpha)))
			{
				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::CheckboxFlagsT(
					"kLighting",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kLighting)))
			{
				OnEffectShaderUpdate(a_params);
			}

			if (ImGui::CheckboxFlagsT(
					"kAlpha",
					stl::underlying(std::addressof(a_data.flags.value)),
					stl::underlying(Data::EffectShaderDataFlags::kAlpha)))
			{
				OnEffectShaderUpdate(a_params);
			}

			ImGui::Columns();

			ImGui::PopID();
		}

		template <class T>
		EffectShaderContextAction UIEffectShaderEditorWidget<T>::DrawTargetNodesContextMenu(
			const T&                      a_params,
			Data::configFixedStringSet_t& a_data)
		{
			EffectShaderContextAction result{ EffectShaderContextAction::None };

			ImGui::PushID("tn_context_area");

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 1.0f });

			if (UIPopupToggleButtonWidget::DrawPopupToggleButton("open", "context_menu"))
			{
				ClearDescriptionPopupBuffer();
			}

			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

			ImGui::PopStyleVar();

			if (ImGui::BeginPopup("context_menu"))
			{
				if (ImGui::BeginMenu(LS(CommonStrings::Add, "1")))
				{
					if (DrawDescriptionPopup())
					{
						auto r = a_data.emplace(GetDescriptionPopupBuffer());
						if (r.second)
						{
							result = EffectShaderContextAction::Add;
						}
					}

					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::MenuItem(LS(CommonStrings::Copy, "2")))
				{
					UIClipboard::Set(a_data);
				}

				auto clipData = UIClipboard::Get<Data::configFixedStringSet_t>();

				if (ImGui::MenuItem(
						LS(CommonStrings::PasteOver, "3"),
						nullptr,
						false,
						clipData != nullptr))
				{
					if (clipData)
					{
						a_data = *clipData;

						OnEffectShaderUpdate(a_params);

						result = EffectShaderContextAction::PasteOver;
					}
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();

			return result;
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawTargetNodes(
			const T&                      a_params,
			Data::configFixedStringSet_t& a_data)
		{
			ImGui::PushID("tn_area");

			const auto result = DrawTargetNodesContextMenu(
				a_params,
				a_data);

			switch (result)
			{
			case EffectShaderContextAction::Add:
			case EffectShaderContextAction::PasteOver:
				OnEffectShaderUpdate(a_params);
				ImGui::SetNextItemOpen(true);
				break;
			}

			if (ImGui::TreeNodeEx(
					"tn_tree",
					ImGuiTreeNodeFlags_DefaultOpen |
						ImGuiTreeNodeFlags_SpanAvailWidth,
					"%s",
					LS(UIEffectShaderEditorWidgetStrings::TargetNodes)))
			{
				if (!a_data.empty())
				{
					ImGui::Spacing();

					DrawTargetNodeTable(a_params, a_data);

					ImGui::Spacing();
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		template <class T>
		void UIEffectShaderEditorWidget<T>::DrawTargetNodeTable(
			const T&                      a_params,
			Data::configFixedStringSet_t& a_data)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 4.f, 4.f });

			constexpr int NUM_COLUMNS = 2;

			if (ImGui::BeginTable(
					"table",
					NUM_COLUMNS,
					ImGuiTableFlags_Borders |
						ImGuiTableFlags_Resizable |
						ImGuiTableFlags_NoSavedSettings |
						ImGuiTableFlags_SizingStretchProp,
					{ -1.0f, 0.f }))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 1.1f);
				ImGui::TableSetupColumn(LS(CommonStrings::Node), ImGuiTableColumnFlags_None, 75.0f);

				ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

				for (int column = 0; column < NUM_COLUMNS; column++)
				{
					ImGui::TableSetColumnIndex(column);
					ImGui::TableHeader(ImGui::TableGetColumnName(column));
				}

				auto it = a_data.begin();

				int i = 0;

				while (it != a_data.end())
				{
					ImGui::PushID(i);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 1.0f });

					if (ImGui::Button("X"))
					{
						it = a_data.erase(it);

						OnEffectShaderUpdate(a_params);
					}

					ImGui::PopStyleVar();

					if (it != a_data.end())
					{
						ImGui::TableSetColumnIndex(1);

						ImGui::Text("%s", it->c_str());

						++it;
						i++;
					}

					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			ImGui::PopStyleVar();
		}

	}
}

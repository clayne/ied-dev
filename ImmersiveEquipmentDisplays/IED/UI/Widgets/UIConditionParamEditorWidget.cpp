#include "pch.h"

#include "IED/Data.h"
#include "UIConditionParamEditorWidget.h"

#include "IED/Controller/Controller.h"

#include "IED/UI/UICommon.h"

namespace IED
{
	namespace UI
	{
		UIConditionParamEditorWidget::UIConditionParamEditorWidget(Controller& a_controller) :
			UIFormLookupInterface(a_controller),
			UILocalizationInterface(a_controller),
			UIConditionExtraSelectorWidget(a_controller),
			m_formPickerForm(a_controller, FormInfoFlags::kNone, true),
			m_formPickerKeyword(a_controller, FormInfoFlags::kNone, true)
		{
			GetKeywordPicker().SetAllowedTypes(
				{ BGSKeyword::kTypeID });
		}

		void UIConditionParamEditorWidget::OpenConditionParamEditorPopup()
		{
			ImGui::OpenPopup("match_param_editor");
		}

		bool UIConditionParamEditorWidget::DrawConditionParamEditorPopup()
		{
			bool result = false;

			auto fontSize = ImGui::GetFontSize();

			ImGui::SetNextWindowSizeConstraints(
				{ fontSize * 34.0f, 0.0f },
				{ 800.0f, 800.0f });

			if (ImGui::BeginPopup("match_param_editor"))
			{
				ImGui::PushItemWidth(fontSize * -5.5f);

				if (const auto& e = get(ConditionParamItem::CMENode); e.p1 && e.p2)
				{
					result |= DrawCMNodeSelector(
						LS(CommonStrings::Node, "ns"),
						e.As1<stl::fixed_string>(),
						NodeOverrideData::GetCMENodeData(),
						static_cast<const stl::fixed_string*>(e.p2));

					ImGui::Spacing();
				}

				if (const auto& e = get(ConditionParamItem::BipedSlot); e.p1)
				{
					result |= DrawBipedObjectSelector(
						LS(CommonStrings::Node, "bp"),
						e.As1<Biped::BIPED_OBJECT>());

					ImGui::Spacing();
				}

				if (const auto& e = get(ConditionParamItem::EquipmentSlot); e.p1)
				{
					result |= DrawObjectSlotSelector(
						LS(CommonStrings::Slot, "ss"),
						e.As1<Data::ObjectSlot>());

					ImGui::Spacing();
				}

				if (const auto& e = get(ConditionParamItem::EquipmentSlotExtra); e.p1)
				{
					result |= DrawObjectSlotSelector(
						LS(CommonStrings::Slot, "ss"),
						e.As1<Data::ObjectSlotExtra>());

					ImGui::Spacing();
				}

				if (const auto& e = get(ConditionParamItem::Form); e.p1)
				{
					ConditionParamItemExtraArgs args;

					if (m_extraInterface)
					{
						if (const auto& f = get(ConditionParamItem::Extra); f.p1)
						{
							args.p1 = e.p1;
							args.p2 = e.p2;
							args.p3 = f.p1;

							result |= m_extraInterface->DrawConditionItemExtra(
								ConditionParamItem::Form,
								args);
						}
					}

					UICommon::PushDisabled(args.disable);

					result |= m_formPickerForm.DrawFormPicker(
						"fp_1",
						LS(CommonStrings::Form),
						e.As1<Game::FormID>());

					UICommon::PopDisabled(args.disable);
				}

				if (const auto& e = get(ConditionParamItem::Keyword); e.p1)
				{
					ConditionParamItemExtraArgs args;

					if (m_extraInterface)
					{
						if (const auto& f = get(ConditionParamItem::Extra); f.p1)
						{
							args.p1 = e.p1;
							args.p2 = e.p2;
							args.p3 = f.p1;

							result |= m_extraInterface->DrawConditionItemExtra(
								ConditionParamItem::Keyword,
								args);
						}
					}

					UICommon::PushDisabled(args.disable);

					result |= m_formPickerKeyword.DrawFormPicker(
						"fp_2",
						LS(CommonStrings::Keyword),
						e.As1<Game::FormID>());

					UICommon::PopDisabled(args.disable);
				}

				if (const auto& e = get(ConditionParamItem::QuestCondType); e.p1)
				{
					if (ImGui::RadioButton(
							LS(CommonStrings::Complete, "qts"),
							e.As1<Data::QuestConditionType>() == Data::QuestConditionType::kComplete))
					{
						e.As1<Data::QuestConditionType>() = Data::QuestConditionType::kComplete;
						result = true;
					}

					ImGui::Spacing();
				}

				if (const auto& e = get(ConditionParamItem::CondExtra); e.p1)
				{
					result |= DrawExtraConditionSelector(
						e.As1<Data::ExtraConditionType>());
				}

				if (m_extraInterface)
				{
					if (const auto& e = get(ConditionParamItem::Extra); e.p1)
					{
						result |= m_extraInterface->DrawConditionParamExtra(e.p1, e.p2);
					}
				}

				ImGui::PopItemWidth();

				ImGui::EndPopup();
			}

			return result;
		}

		const char* UIConditionParamEditorWidget::GetItemDesc(ConditionParamItem a_item)
		{
			switch (a_item)
			{
			case ConditionParamItem::Form:
			case ConditionParamItem::Keyword:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						GetFormDesc(*static_cast<const Game::FormID*>(e.p1));
					}
					else
					{
						m_descBuffer[0] = 0x0;
					}
				}
				break;
			case ConditionParamItem::CMENode:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						auto& data = NodeOverrideData::GetCMENodeData();

						auto it = data.find(*static_cast<const stl::fixed_string*>(e.p1));
						if (it != data.end())
						{
							return it->second.desc;
						}
					}

					m_descBuffer[0] = 0x0;
				}
				break;
			case ConditionParamItem::BipedSlot:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						return GetFormKeywordExtraDesc(GetBipedSlotDesc(
							*static_cast<Biped::BIPED_OBJECT*>(e.p1)));
					}
					else
					{
						m_descBuffer[0] = 0x0;
					}
				}
				break;
			case ConditionParamItem::EquipmentSlot:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						return GetFormKeywordExtraDesc(
							Data::GetSlotName(*static_cast<Data::ObjectSlot*>(e.p1)));
					}
					else
					{
						m_descBuffer[0] = 0x0;
					}
				}
				break;
			case ConditionParamItem::EquipmentSlotExtra:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						return GetFormKeywordExtraDesc(
							Data::GetSlotName(*static_cast<Data::ObjectSlotExtra*>(e.p1)));
					}
					else
					{
						m_descBuffer[0] = 0x0;
					}
				}
				break;

			case ConditionParamItem::Furniture:
				{
					if (const auto& e = get(ConditionParamItem::Extra); e.p1)
					{
						auto r = GetFormKeywordExtraDesc(nullptr);

						auto match = static_cast<Data::configNodeOverrideCondition_t*>(e.p1);

						if (r[0] == 0)
						{
							stl::snprintf(
								m_descBuffer2,
								"LD: %s",
								match->flags.test(Data::NodeOverrideConditionFlags::kLayingDown) ?
                                    LS(CommonStrings::True) :
                                    LS(CommonStrings::False));
						}
						else
						{
							stl::snprintf(
								m_descBuffer2,
								"%s, LD: %s",
								r,
								match->flags.test(Data::NodeOverrideConditionFlags::kLayingDown) ?
                                    LS(CommonStrings::True) :
                                    LS(CommonStrings::False));
						}

						return m_descBuffer2;
					}
					else
					{
						m_descBuffer[0] = 0x0;
					}
				}
				break;

			case ConditionParamItem::CondExtra:
				{
					if (const auto& e = get(a_item); e.p1)
					{
						if (auto r = condition_type_to_desc(e.As1<Data::ExtraConditionType>()))
						{
							return r;
						}
					}
					m_descBuffer[0] = 0x0;
				}
				break;
			default:
				m_descBuffer[0] = 0x0;
				break;
			}

			return m_descBuffer;
		}

		void UIConditionParamEditorWidget::GetFormDesc(Game::FormID a_form)
		{
			if (auto info = LookupForm(a_form))
			{
				stl::snprintf(
					m_descBuffer,
					"[%.8X] %s",
					a_form.get(),
					info->form.name.c_str());
			}
			else
			{
				stl::snprintf(
					m_descBuffer,
					"%.8X",
					a_form.get());
			}
		}

		const char* UIConditionParamEditorWidget::GetFormKeywordExtraDesc(
			const char* a_idesc) const noexcept
		{
			Game::FormID a_iform;
			Game::FormID a_ikw;

			if (const auto& e = get(ConditionParamItem::Form); e.p1)
			{
				a_iform = *static_cast<const Game::FormID*>(e.p1);
			}

			if (const auto& e = get(ConditionParamItem::Keyword); e.p1)
			{
				a_ikw = *static_cast<const Game::FormID*>(e.p1);
			}

			if (a_iform && a_ikw)
			{
				if (a_idesc)
				{
					stl::snprintf(
						m_descBuffer,
						"%s, F: %.8X, KW: %.8X",
						a_idesc,
						a_iform.get(),
						a_ikw.get());
				}
				else
				{
					stl::snprintf(
						m_descBuffer,
						"F: %.8X, KW: %.8X",
						a_iform.get(),
						a_ikw.get());
				}
			}
			else if (a_iform)
			{
				if (a_idesc)
				{
					stl::snprintf(
						m_descBuffer,
						"%s, F: %.8X",
						a_idesc,
						a_iform.get());
				}
				else
				{
					stl::snprintf(
						m_descBuffer,
						"F: %.8X",
						a_iform.get());
				}
			}
			else if (a_ikw)
			{
				if (a_idesc)
				{
					stl::snprintf(
						m_descBuffer,
						"%s, KW: %.8X",
						a_idesc,
						a_ikw.get());
				}
				else
				{
					stl::snprintf(
						m_descBuffer,
						"KW: %.8X",
						a_ikw.get());
				}
			}
			else
			{
				if (a_idesc)
				{
					stl::snprintf(
						m_descBuffer,
						"%s",
						a_idesc);
				}
				else
				{
					m_descBuffer[0] = 0;
				}
			}

			return m_descBuffer;
		}

		void UIConditionParamEditorWidget::Reset()
		{
			for (auto& e : m_entries)
			{
				e = {};
			}
		}

		bool UIConditionParamExtraInterface::DrawConditionItemExtra(
			ConditionParamItem a_item,
			ConditionParamItemExtraArgs& a_args)
		{
			return false;
		}

	}
}
#pragma once

#include "ConfigOverride.h"
#include "ConfigOverrideDefault.h"
#include "NodeMap.h"

#include "Controller/ObjectDatabaseLevel.h"

#include "UI/UIData.h"
#include "UI/UIMainCommon.h"
#include <Fonts/FontInfo.h>

namespace IED
{
	namespace Data
	{
		class SettingHolder
		{
		public:
			struct Controls :
				public ConfigKeyPair
			{
			};

			using EditorPanelCommonFlagsType = std::uint32_t;

			struct EditorPanelCommon
			{
				bool sexSync{ false };
				bool eoPropagation{ false };
				EditorPanelCommonFlagsType flags{ 0 };

				template <
					class T,
					class = std::enable_if_t<
						std::is_enum_v<T> && std::is_same_v<std::underlying_type_t<T>, EditorPanelCommonFlagsType>,
						void>>
				inline constexpr stl::flag<T>& get_flags() noexcept
				{
					return reinterpret_cast<stl::flag<T>&>(flags);
				}

				/*template <class T, class = std::enable_if_t<std::is_enum_v<T>, void>>
				inline constexpr const T& get_flags() const noexcept
				{
					return static_cast<const T&>(flags);
				}*/
			};

			/*struct ObjectDatabase
			{
				std::size_t limit{ 10 };
			};*/

			struct ProfileEditor :
				public EditorPanelCommon
			{
				ConfigSex sex{ ConfigSex::Male };

				UI::UIData::UICollapsibleStates colStates;
			};

			struct EditorPanelActorSettings
			{
				bool selectCrosshairActor{ true };
				bool autoSelectSex{ true };
				bool showAll{ true };
				ConfigSex sex{ Data::ConfigSex::Male };

				Game::FormID lastActor;
			};

			struct EditorPanelRaceSettings
			{
				bool selectCrosshairActor{ true };
				bool playableOnly{ false };
				bool showEditorIDs{ true };
				ConfigSex sex{ ConfigSex::Male };
			};

			struct EditorPanel :
				public EditorPanelCommon
			{
				EditorPanelActorSettings actorConfig;
				EditorPanelActorSettings npcConfig;
				EditorPanelRaceSettings raceConfig;

				GlobalConfigType globalType{ GlobalConfigType::Player };
				ConfigSex globalSex{ ConfigSex::Male };

				ConfigClass lastConfigClass{ ConfigClass::Global };

				UI::UIData::UICollapsibleStates colStates[CONFIG_CLASS_MAX];
			};

			struct ImportExport
			{
				bool eraseTemporary{ true };
				stl::flag<Data::ConfigStoreSerializationFlags> exportFlags{
					Data::ConfigStoreSerializationFlags::kAll
				};
			};

			struct UserInterface
			{
				UserInterface() noexcept
				{
					for (auto& e : logLevels)
					{
						e = true;
					}
				}

				EditorPanel slotEditor;
				EditorPanel customEditor;
				EditorPanel transformEditor;
				ProfileEditor slotProfileEditor;
				ProfileEditor customProfileEditor;
				ProfileEditor transformProfileEditor;
				ImportExport importExport;

				UI::UIData::UICollapsibleStates settingsColStates;
				UI::UIData::UICollapsibleStates statsColStates;

				UI::UIEditorPanel lastPanel;

				Controls toggleKeys;

				bool enableControlLock{ true };
				bool enableRestrictions{ false };
				float scale{ 1.0f };

				std::uint32_t logLimit{ 200 };
				bool logLevels[stl::underlying(LogLevel::Max) + 1];

				bool closeOnESC{ false };

				stl::flag<Data::ConfigStoreSerializationFlags> defaultExportFlags{
					Data::ConfigStoreSerializationFlags::kAll
				};

				stl::fixed_string font;
				SetObjectWrapper<float> fontSize;
				stl::flag<GlyphPresetFlags> extraGlyphs{ GlyphPresetFlags::kNone };
			};

			struct Settings
			{
				UserInterface ui;

				Controls playerBlockKeys;

				bool playSound{ true };
				bool playSoundNPC{ false };
				bool hideEquipped{ false };
				bool toggleKeepLoaded{ false };

				SetObjectWrapper<LogLevel> logLevel;

				ObjectDatabaseLevel odbLevel{ ObjectDatabaseLevel::kDisabled };

				stl::fixed_string laIEDage;
			};

			Settings data;

			inline void SetPath(const fs::path& a_path)
			{
				m_path = a_path;
			}

			inline void SetPath(fs::path&& a_path)
			{
				m_path = std::move(a_path);
			}

			inline constexpr const auto& GetPath() const noexcept
			{
				return m_path;
			}

			bool Load();
			bool Save();
			bool SaveIfDirty();

			template <
				class Tm,
				class Tv,
				class = std::enable_if_t<std::is_convertible_v<Tv, Tm>>>
			inline void Set(Tm& a_member, Tv&& a_value)
			{
				a_member = std::forward<Tv>(a_value);
				m_dirty = true;
			}

			inline void MarkDirty()
			{
				m_dirty = true;
			}

			inline bool MarkIf(bool a_isTrue)
			{
				if (a_isTrue)
				{
					m_dirty = true;
				}
				return a_isTrue;
			}

			inline constexpr const auto& GetLastException() const noexcept
			{
				return m_lastException;
			}

		private:
			mutable except::descriptor m_lastException;

			fs::path m_path;
			bool m_dirty{ false };
		};

	}  // namespace Data
}  // namespace IED
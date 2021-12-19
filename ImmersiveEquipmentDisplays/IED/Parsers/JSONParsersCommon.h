#pragma once

#include "IED/ConfigCommon.h"

#include "JSONFormParser.h"

#include "Serialization/Serialization.h"

namespace IED
{
	namespace Serialization
	{
		class SlotKeyParser
		{
		public:
			SlotKeyParser();

			const char* SlotToKey(Data::ObjectSlot a_slot) const noexcept;
			Data::ObjectSlot KeyToSlot(const std::string& a_key) const;

		private:
			std::unordered_map<std::string, Data::ObjectSlot> m_keyToSlot;
		};

		template <class Th, class Tm>
		constexpr void ParseConfigMap(
			const Json::Value& a_in,
			Tm& a_out,
			std::uint32_t a_version)
		{
			Parser<Game::FormID> pform;
			Parser<Th> pholder;

			auto& data = a_in["data"];

			for (auto& e : data)
			{
				Game::FormID form;

				if (!pform.Parse(e["form"], form, a_version))
				{
					gLog.Warning("%s: missing form entry", __FUNCTION__);
					continue;
				}

				Th tmp;

				if (!pholder.Parse(e["data"], tmp))
				{
					gLog.Warning("%s: failed to parse record data", __FUNCTION__);
					continue;
				}

				auto r = a_out.try_emplace(form, std::move(tmp));

				if (!r.second)
				{
					gLog.Warning("%s: duplicate form %.8X in list", __FUNCTION__, form);
				}
			}
		}

		template <class Th, class Tm>
		constexpr void CreateConfigMap(
			const Tm& a_data,
			Json::Value& a_out,
			std::uint32_t a_version)
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::arrayValue));

			Parser<Game::FormID> pform;
			Parser<Th> pholder;

			for (auto& e : a_data)
			{
				auto v = Json::Value();

				pform.Create(e.first, v["form"]);
				pholder.Create(e.second, v["data"]);

				data.append(std::move(v));
			}

			a_out["version"] = a_version;
		}

		
		template <class Th, class Tm, class Ts>
		[[nodiscard]] constexpr bool ParseConfigStore(
			const Json::Value& a_in,
			Ts& a_out,
			std::uint32_t a_version)
		{
			Parser<Tm> pmap;
			Parser<Th> pholder;

			auto& data = a_in["data"];

			auto& g = a_out.GetFormMaps();

			if (!pmap.Parse(data["actor"], g[stl::underlying(ConfigClass::Actor)]))
			{
				return false;
			}

			if (!pmap.Parse(data["npc"], g[stl::underlying(ConfigClass::NPC)]))
			{
				return false;
			}

			if (!pmap.Parse(data["race"], g[stl::underlying(ConfigClass::Race)]))
			{
				return false;
			}

			auto& f = a_out.GetGlobalData();

			if (!pholder.Parse(
					data["default_player"],
					f[stl::underlying(GlobalConfigType::Player)]))
			{
				return false;
			}

			if (!pholder.Parse(
					data["default_npc"],
					f[stl::underlying(GlobalConfigType::NPC)]))
			{
				return false;
			}

			return true;
		}

		
		template <class Th, class Tm, class Ts>
		constexpr void CreateConfigStore(
			const Ts& a_data,
			Json::Value& a_out,
			std::uint32_t a_version)
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::objectValue));

			Parser<Tm> pmap;
			Parser<Th> pholder;

			auto& g = a_data.GetFormMaps();

			pmap.Create(g[stl::underlying(ConfigClass::Actor)], data["actor"]);
			pmap.Create(g[stl::underlying(ConfigClass::NPC)], data["npc"]);
			pmap.Create(g[stl::underlying(ConfigClass::Race)], data["race"]);

			auto& f = a_data.GetGlobalData();

			pholder.Create(
				f[stl::underlying(GlobalConfigType::Player)],
				data["default_player"]);

			pholder.Create(
				f[stl::underlying(GlobalConfigType::NPC)],
				data["default_npc"]);

			a_out["version"] = a_version;
		}

	}  // namespace Serialization
}  // namespace IED
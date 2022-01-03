#include "pch.h"

#include "JSONConfigNodeOverridePlacementOverrideListParser.h"
#include "JSONConfigNodeOverridePlacementOverrideParser.h"

namespace IED
{
	namespace Serialization
	{
		static constexpr std::uint32_t CURRENT_VERSION = 1;

		template <>
		bool Parser<Data::configNodeOverridePlacementOverrideList_t>::Parse(
			const Json::Value& a_in,
			Data::configNodeOverridePlacementOverrideList_t& a_out) const
		{
			JSON_PARSE_VERSION()

			auto& data = a_in["data"];

			Parser<Data::configNodeOverridePlacementOverride_t> parser(m_state);

			for (auto& e : data)
			{
				if (!parser.Parse(e, a_out.emplace_back(), version))
				{
					return false;
				}
			}

			return true;
		}

		template <>
		void Parser<Data::configNodeOverridePlacementOverrideList_t>::Create(
			const Data::configNodeOverridePlacementOverrideList_t& a_data,
			Json::Value& a_out) const
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::arrayValue));

			Parser<Data::configNodeOverridePlacementOverride_t> parser(m_state);

			for (auto& e : a_data)
			{
				parser.Create(e, data.append(Json::Value()));
			}

			a_out["version"] = CURRENT_VERSION;
		}

		template <>
		void Parser<Data::configNodeOverridePlacementOverrideList_t>::GetDefault(
			Data::configNodeOverridePlacementOverrideList_t& a_out) const
		{
		}
	}
}
#include "pch.h"

#include "JSONConfigBaseValuesParser.h"
#include "JSONEquipmentOverrideConditionListParser.h"
#include "JSONEquipmentOverrideParser.h"

namespace IED
{
	namespace Serialization
	{
		template <>
		bool Parser<Data::equipmentOverride_t>::Parse(
			const Json::Value& a_in,
			Data::equipmentOverride_t& a_out,
			const std::uint32_t a_version) const
		{
			Parser<Data::configBaseValues_t> bvParser;
			Parser<Data::equipmentOverrideConditionList_t> mlParser;

			if (!mlParser.Parse(a_in["matches"], a_out.matches))
			{
				return false;
			}

			if (!bvParser.Parse(a_in["config"], a_out, a_version))
			{
				return false;
			}

			a_out.description = a_in.get("desc", "").asString();
			a_out.eoFlags = static_cast<Data::EquipmentOverrideFlags>(a_in.get("flags", 0).asUInt());

			return true;
		}

		template <>
		void Parser<Data::equipmentOverride_t>::Create(
			const Data::equipmentOverride_t& a_data,
			Json::Value& a_out) const
		{
			Parser<Data::configBaseValues_t> bvParser;
			Parser<Data::equipmentOverrideConditionList_t> mlParser;

			mlParser.Create(a_data.matches, a_out["matches"]);
			bvParser.Create(a_data, a_out["config"]);

			a_out["desc"] = a_data.description;
			a_out["flags"] = stl::underlying(a_data.eoFlags.value);
		}

		template <>
		void Parser<Data::equipmentOverride_t>::GetDefault(Data::equipmentOverride_t& a_out) const
		{}
	}
}
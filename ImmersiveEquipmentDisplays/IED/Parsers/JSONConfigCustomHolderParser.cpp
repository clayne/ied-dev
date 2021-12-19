#include "pch.h"

#include "JSONConfigCustomHolderParser.h"
#include "JSONConfigCustomParser.h"
#include "JSONParsersCommon.h"

namespace IED
{
	namespace Serialization
	{
		using namespace Data;

		template <>
		bool Parser<Data::configCustomHolder_t>::Parse(
			const Json::Value& a_in,
			Data::configCustomHolder_t& a_out) const
		{
			std::uint32_t version;

			if (!ParseVersion(a_in, "version", version))
			{
				Error("%s: bad version data", __FUNCTION__);
				return false;
			}

			Parser<Data::configCustom_t> pslot;

			auto& data = a_in["data"];

			for (auto it = data.begin(); it != data.end(); ++it)
			{
				auto key = it.key().asString();

				auto& v = a_out.data.try_emplace(key).first->second;

				parserDesc_t<Data::configCustom_t> desc[]{
					{ "m", v(ConfigSex::Male) },
					{ "f", v(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					pslot.Parse((*it)[e.member], e.data, version);
				}
			}

			return true;
		}

		template <>
		void Parser<Data::configCustomHolder_t>::Create(
			const Data::configCustomHolder_t& a_data,
			Json::Value& a_out) const
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::objectValue));

			using enum_type = std::underlying_type_t<ObjectSlot>;

			Parser<Data::configCustom_t> pslot;

			for (auto& e : a_data.data)
			{
				parserDescConst_t<Data::configCustom_t> desc[]{
					{ "m", e.second(ConfigSex::Male) },
					{ "f", e.second(ConfigSex::Female) }
				};

				auto& v = data[e.first];

				for (auto& f : desc)
				{
					pslot.Create(f.data, v[f.member]);
				}
			}

			a_out["version"] = 1u;
		}

		template <>
		void Parser<Data::configCustomHolder_t>::GetDefault(
			Data::configCustomHolder_t& a_out) const
		{
		}

	}
}
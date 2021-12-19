#pragma once

#include "IED/ConfigOverride.h"

#include "Serialization/Serialization.h"

namespace IED
{
	namespace Serialization
	{
		template <>
		bool Parser<Data::equipmentOverride_t>::Parse(
			const Json::Value& a_in,
			Data::equipmentOverride_t& a_outData,
			const std::uint32_t a_version) const;

		template <>
		void Parser<Data::equipmentOverride_t>::Create(
			const Data::equipmentOverride_t& a_data,
			Json::Value& a_out) const;

		template <>
		void Parser<Data::equipmentOverride_t>::GetDefault(Data::equipmentOverride_t& a_out) const;

	}  // namespace Serialization
}  // namespace IED
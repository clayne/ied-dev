#pragma once

#include "IED/OM/ConfigOutfitForm.h"

#include "Serialization/Serialization.h"

namespace IED
{
	namespace Serialization
	{
		template <>
		bool Parser<Data::OM::configOutfitForm_t>::Parse(
			const Json::Value&            a_in,
			Data::OM::configOutfitForm_t& a_out,
			const std::uint32_t           a_version) const;

		template <>
		void Parser<Data::OM::configOutfitForm_t>::Create(
			const Data::OM::configOutfitForm_t& a_data,
			Json::Value&                        a_out) const;

	}
}
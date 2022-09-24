#pragma once

#include "ObjectEntryBase.h"

namespace IED
{

	enum class CustomObjectEntryFlags : std::uint32_t
	{
		kNone = 0,

		kProcessedChance = 1u << 0,
		kBlockedByChance = 1u << 1,
		kUseGroup        = 1u << 2,

		kChanceMask = kProcessedChance | kBlockedByChance
	};

	DEFINE_ENUM_CLASS_BITWISE(CustomObjectEntryFlags);

	struct ObjectEntryCustom :
		ObjectEntryBase
	{
		inline constexpr void clear_chance_flags() noexcept
		{
			cflags.clear(CustomObjectEntryFlags::kChanceMask);
		}

		Game::FormID                      modelForm;
		stl::flag<CustomObjectEntryFlags> cflags{ CustomObjectEntryFlags::kNone };
	};

}
#pragma once

#include "IED/ConfigTransform.h"

namespace IED
{
	namespace Data
	{
		struct cacheTransform_t
		{
			constexpr void Update(const Data::configTransform_t& a_in) noexcept
			{
				scale    = a_in.scale;
				position = a_in.position;
				rotation = a_in.rotationMatrix;

				tag = a_in;
			}

			constexpr void clear() noexcept
			{
				scale.clear();
				position.clear();
				rotation.reset();

				tag.reset();
			}

			[[nodiscard]] inline constexpr bool operator==(
				const Data::configTransform_t& a_rhs) const noexcept
			{
				return tag == a_rhs;
			}

			stl::optional<NiPoint3>   position;
			std::optional<NiMatrix33> rotation;
			stl::optional<float>      scale;
			std::optional<luid_tag>   tag;
		};
	}
}
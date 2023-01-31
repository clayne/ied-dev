#pragma once

#include "IED/AnimationWeaponSlot.h"
#include "IED/GearNodeID.h"

namespace IED
{
	struct WeaponNodeEntry
	{
		friend class INodeOverride;

	public:
		WeaponNodeEntry(
			const stl::fixed_string&          a_nodeName,
			NiNode*                           a_node,
			NiNode*                           a_defaultNode,
			NiNode*                           a_node1p,
			AnimationWeaponSlot               a_animID,
			GearNodeID                        a_gearNodeID,
			const std::optional<NiTransform>& a_xfrm) noexcept :
			nodeName(a_nodeName),
			node(a_node),
			defaultNode(a_defaultNode),
			node1p(a_node1p),
			animSlot(a_animID),
			gearNodeID(a_gearNodeID),
			originalTransform(a_xfrm)
		{
		}

		bool has_visible_geometry() const noexcept;

		const stl::fixed_string          nodeName;
		const NiPointer<NiNode>          node;
		const NiPointer<NiNode>          defaultNode;
		const NiPointer<NiNode>          node1p;
		const AnimationWeaponSlot        animSlot;
		const GearNodeID                 gearNodeID;
		const std::optional<NiTransform> originalTransform;

	private:
		mutable NiPointer<NiNode> target;
	};

}
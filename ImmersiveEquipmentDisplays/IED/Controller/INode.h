#pragma once

#include "../ConfigOverride.h"
#include "../Data.h"

#include "ObjectManagerCommon.h"

namespace IED
{
	struct nodesRef_t
	{
		NiPointer<NiNode> obj;
		NiPointer<NiNode> ref;

		SKMP_FORCEINLINE bool IsReferenceMovedOrOphaned() const noexcept
		{
			if (auto objParent = obj->m_parent)
			{
				if (auto objParentParent = objParent->m_parent)
				{
					if (auto refParent = ref->m_parent)
					{
						return refParent != objParentParent;
					}
					else
					{
						return true;
					}
				}
			}

			return false;
		}
	};

	class INode
	{
	private:
		struct findResult_t
		{
			NiNode* parent{ nullptr };
			NiNode* ref{ nullptr };
		};

	public:
		inline static constexpr std::size_t NODE_NAME_BUFFER_SIZE = MAX_PATH;
		inline static constexpr auto ATTACHMENT_NODE_FLAGS = NiAVObject::kFlag_SelectiveUpdate |
		                                                     NiAVObject::kFlag_SelectiveUpdateTransforms |
		                                                     NiAVObject::kFlag_kSelectiveUpdateController |
		                                                     NiAVObject::kFlag_kNoAnimSyncS;

		static void UpdateDownwardPass(NiAVObject* a_object);

		static bool CreateTargetNode(
			const Data::configBaseValues_t& a_entry,
			const Data::NodeDescriptor& a_node,
			NiNode* a_root,
			nodesRef_t& a_out);

		static bool AttachObjectToTargetNode(
			const Data::NodeDescriptor& a_node,
			bool a_atmReference,
			NiNode* a_root,
			NiAVObject* a_object,
			NiPointer<NiNode>& a_newRef);

		static void UpdateObjectTransform(
			const Data::cacheTransform_t& a_trnsf,
			NiAVObject* a_node,
			NiNode* a_refNode,
			bool a_updateWorldData = true);

		static void GetArmorNodeName(
			Game::FormID a_formid,
			Game::FormID a_arma,
			char (&a_out)[NODE_NAME_BUFFER_SIZE]);

		static void GetWeaponNodeName(
			Game::FormID a_formid,
			char (&a_out)[NODE_NAME_BUFFER_SIZE]);

		static void GetMiscNodeName(
			Game::FormID a_formid,
			char (&a_out)[NODE_NAME_BUFFER_SIZE]);

	protected:
		static NiNode* CreateNode(const char *a_name);

	private:
		static NiNode* CreateAttachmentNode(
			const BSFixedString& a_nodeName);
		
		/*static NiNode* CreateAttachmentNode(
			BSFixedString&& a_nodeName);*/

		static findResult_t FindNodes(
			const Data::NodeDescriptor& a_node,
			bool a_atmReference,
			NiNode* a_root);

		static BSFixedString GetTargetNodeName(
			const Data::NodeDescriptor& a_node,
			bool a_atmReference);
	};
}  // namespace IED
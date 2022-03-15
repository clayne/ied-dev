#pragma once

#include "IED/ConfigModelGroup.h"
#include "IED/ConfigStore.h"
#include "INode.h"
#include "NodeOverrideData.h"
#include "ObjectDatabase.h"
#include "ObjectManagerCommon.h"

#include "IED/ActorState.h"

namespace IED
{
	struct effectShaderData_t
	{
		enum class EntryFlags : std::uint32_t
		{
			kNone = 0,

			kForce = 1u << 0
		};

		struct Entry
		{
			Entry()  = default;
			~Entry() = default;

			Entry(const Entry&) = delete;
			Entry(Entry&&)      = default;
			Entry& operator=(const Entry&) = delete;
			Entry& operator=(Entry&&) = default;

			stl::flag<EntryFlags>                                                      flags{ EntryFlags::kNone };
			RE::BSTSmartPointer<BSEffectShaderData>                                    shaderData;
			std::vector<std::pair<NiPointer<BSShaderProperty>, NiPointer<BSGeometry>>> nodes;
		};

		using data_type = std::unordered_map<stl::fixed_string, Entry>;

		[[nodiscard]] inline constexpr bool operator==(
			const Data::effectShaderList_t& a_rhs) const
		{
			return tag == a_rhs;
		}

		[[nodiscard]] inline constexpr explicit operator bool() const noexcept
		{
			return tag.has_value();
		}

		void clear();

		void Update(
			NiNode*                                 a_object,
			const uuid_tag&                         a_tag,
			const Data::configEffectShaderHolder_t& a_data);

		template <class Tf>
		constexpr void visit_nodes(Tf a_func)
		{
			for (auto& e : data)
			{
				for (auto& f : e.second.nodes)
				{
					a_func(e, f);
				}
			}
		}

		data_type               data;
		std::optional<uuid_tag> tag;
	};

	DEFINE_ENUM_CLASS_BITWISE(effectShaderData_t::EntryFlags);

	enum class ObjectEntryFlags : std::uint32_t
	{
		kNone                       = 0,
		kRefSyncDisableFailedOrphan = 1u << 1,
		kScbLeft                    = 1u << 2,
		kSyncReferenceTransform     = 1u << 6,
		kPlaySound                  = 1u << 8,
		kIsGroup                    = 1u << 9,
	};

	DEFINE_ENUM_CLASS_BITWISE(ObjectEntryFlags);

	struct objectEntryBase_t
	{
		objectEntryBase_t()  = default;
		~objectEntryBase_t() = default;

		objectEntryBase_t(const objectEntryBase_t&) = delete;
		objectEntryBase_t(objectEntryBase_t&&)      = delete;
		objectEntryBase_t& operator=(const objectEntryBase_t&) = delete;
		objectEntryBase_t& operator=(objectEntryBase_t&&) = delete;

		void reset(Game::ObjectRefHandle a_handle, NiNode* a_root);

		inline void SetNodeVisible(bool a_switch) noexcept
		{
			if (state)
			{
				state->nodes.obj->SetVisible(a_switch);
			}
		}

		SKMP_FORCEINLINE auto IsActive() const noexcept
		{
			return state && state->nodes.obj->IsVisible();
		}

		SKMP_FORCEINLINE auto GetFormIfActive() const noexcept
		{
			return IsActive() ? state->form : nullptr;
		}

		SKMP_FORCEINLINE auto IsNodeVisible() const noexcept
		{
			return state && state->nodes.obj->IsVisible();
		}

		struct State
		{
			State()  = default;
			~State() = default;

			State(const State&) = delete;
			State(State&&)      = delete;
			State& operator=(const State&) = delete;
			State& operator=(State&&) = delete;

			struct GroupObject
			{
				NiPointer<NiNode>      object;
				NiPointer<NiNode>      main;
				Data::cacheTransform_t transform;

				void PlayAnimation(Actor* a_actor, const stl::fixed_string& a_sequence);
			};

			void UpdateData(
				const Data::configBaseValues_t& a_in) noexcept
			{
				UpdateFlags(a_in);
				transform.Update(a_in);

				resetTriggerFlags = a_in.flags & Data::BaseFlags::kResetTriggerFlags;
			}

			void UpdateFlags(
				const Data::configBaseValues_t& a_in) noexcept
			{
				// gross but efficient

				static_assert(
					std::is_same_v<std::underlying_type_t<ObjectEntryFlags>, std::underlying_type_t<Data::BaseFlags>> &&
					stl::underlying((ObjectEntryFlags::kPlaySound)) == stl::underlying((Data::BaseFlags::kPlaySound)) &&
					stl::underlying((ObjectEntryFlags::kSyncReferenceTransform)) == stl::underlying((Data::BaseFlags::kSyncReferenceTransform)));

				flags =
					(flags & ~(ObjectEntryFlags::kPlaySound | ObjectEntryFlags::kSyncReferenceTransform | ObjectEntryFlags::kRefSyncDisableFailedOrphan)) |
					static_cast<ObjectEntryFlags>((a_in.flags & (Data::BaseFlags::kPlaySound | Data::BaseFlags::kSyncReferenceTransform)));
			}

			void UpdateGroupTransforms(const Data::configModelGroup_t& a_group)
			{
				for (auto& e : a_group.entries)
				{
					if (auto it = groupObjects.find(e.first);
					    it != groupObjects.end())
					{
						it->second.transform.Update(e.second.transform);
					}
				}
			}

			void CleanupObjects(Game::ObjectRefHandle a_handle);

			void UpdateAndPlayAnimation(
				Actor*                   a_actor,
				const stl::fixed_string& a_sequence);

			TESForm*                                           form{ nullptr };
			Game::FormID                                       formid;
			stl::flag<ObjectEntryFlags>                        flags{ ObjectEntryFlags::kNone };
			Data::NodeDescriptor                               nodeDesc;
			nodesRef_t                                         nodes;
			Data::cacheTransform_t                             transform;
			std::list<ObjectDatabase::ObjectDatabaseEntry>     dbEntries;
			std::unordered_map<stl::fixed_string, GroupObject> groupObjects;
			effectShaderData_t                                 effectShaders;
			stl::fixed_string                                  currentSequence;
			stl::flag<Data::BaseFlags>                         resetTriggerFlags{ Data::BaseFlags::kNone };
			bool                                               atmReference{ true };
		};

		std::unique_ptr<State> state;
	};

	struct objectEntrySlot_t :
		objectEntryBase_t
	{
		Data::actorStateSlotEntry_t slotState;
		Data::ObjectSlot            slotid{ Data::ObjectSlot::kMax };
		std::uint8_t                hideCountdown{ 0 };

		inline constexpr void ResetDeferredHide() noexcept
		{
			hideCountdown = 0;
		}
	};

	enum class CustomObjectEntryFlags : std::uint32_t
	{
		kNone = 0,

		kProcessedChance = 1u << 0,
		kBlockedByChance = 1u << 1,
		kUseGroup        = 1u << 2,

		kChanceMask = kProcessedChance | kBlockedByChance
	};

	DEFINE_ENUM_CLASS_BITWISE(CustomObjectEntryFlags);

	struct objectEntryCustom_t :
		objectEntryBase_t
	{
		inline constexpr void clear_chance_flags() noexcept
		{
			cflags.clear(CustomObjectEntryFlags::kChanceMask);
		}

		Game::FormID                      modelForm;
		stl::flag<CustomObjectEntryFlags> cflags{ CustomObjectEntryFlags::kNone };
	};

	class IObjectManager;
	class Controller;
	class ActorProcessorTask;
	class INodeOverride;

	enum class ActorObjectHolderFlags : std::uint32_t
	{
		kNone = 0,

		kWantTransformUpdate      = 1u << 0,
		kImmediateTransformUpdate = 1u << 1,
		kSkipNextTransformUpdate  = 1u << 2,

		kRequestTransformUpdateDefer     = kWantTransformUpdate | kSkipNextTransformUpdate,
		kRequestTransformUpdateImmediate = kWantTransformUpdate | kImmediateTransformUpdate,
		kRequestTransformUpdateMask      = kWantTransformUpdate | kImmediateTransformUpdate | kSkipNextTransformUpdate,

		kWantEval       = 1u << 3,
		kImmediateEval  = 1u << 4,
		kEvalCountdown1 = 1u << 5,
		kEvalCountdown2 = 1u << 6,

		kRequestEval          = kWantEval,
		kRequestEvalImmediate = kWantEval | kImmediateEval,
		kRequestEvalMask      = kWantEval | kImmediateEval | kEvalCountdown1 | kEvalCountdown2
	};

	DEFINE_ENUM_CLASS_BITWISE(ActorObjectHolderFlags);

	struct ActorObjectHolderFlagsBitfield
	{
		std::uint32_t wantTransformUpdate     : 1;
		std::uint32_t immediateTransformUpdate: 1;
		std::uint32_t skipNextTransformUpdate : 1;
		std::uint32_t wantEval                : 1;
		std::uint32_t immediateEval           : 1;
		std::uint32_t evalCountdown           : 2;
		std::uint32_t unused                  : 25;
	};

	static_assert(sizeof(ActorObjectHolderFlagsBitfield) == sizeof(ActorObjectHolderFlags));

	struct weapNodeEntry_t
	{
		friend class INodeOverride;

	public:
		weapNodeEntry_t(
			const stl::fixed_string& a_nodeName,
			NiNode*                  a_node,
			NiNode*                  a_defaultNode) :
			nodeName(a_nodeName),
			node(a_node),
			defaultNode(a_defaultNode)
		{
		}

		const stl::fixed_string nodeName;
		NiPointer<NiNode>       node;
		NiPointer<NiNode>       defaultNode;

	private:
		mutable NiPointer<NiNode> target;
	};

	class BSStringHolder;

	struct cmeNodeEntry_t
	{
		NiPointer<NiNode> node;

		static bool find_visible_geometry(
			NiAVObject*           a_object,
			const BSStringHolder* a_sh) noexcept;

		bool has_visible_geometry(
			const BSStringHolder* a_sh) const noexcept;

		bool has_visible_object(
			NiAVObject* a_findObject) const noexcept;

		mutable const Data::configNodeOverrideEntryTransform_t* cachedConfCME{ nullptr };
	};

	struct movNodeEntry_t
	{
		NiPointer<NiNode> node;
	};

	class ActorObjectHolder
	{
		friend class IObjectManager;
		friend class Controller;
		friend class ActorProcessorTask;
		friend class ObjectManagerData;

		struct monitorNodeEntry_t
		{
			NiPointer<NiNode> node;
			NiPointer<NiNode> parent;
			std::uint16_t     size;
			bool              visible;
		};

		struct actorLocationData_t
		{
			bool           inInterior{ false };
			TESWorldSpace* worldspace{ nullptr };
		};

	public:
		using slot_container_type = objectEntrySlot_t[stl::underlying(Data::ObjectSlot::kMax)];

		ActorObjectHolder() = delete;
		ActorObjectHolder(
			const stl::optional<Data::actorStateEntry_t>& a_playerState,
			Actor*                                        a_actor,
			NiNode*                                       a_root,
			NiNode*                                       a_npcroot,
			IObjectManager&                               a_owner,
			Game::ObjectRefHandle                         a_handle,
			bool                                          a_nodeOverrideEnabled,
			bool                                          a_nodeOverrideEnabledPlayer,
			Data::actorStateHolder_t&                     a_actorState);

		~ActorObjectHolder();

		ActorObjectHolder(const ActorObjectHolder&) = delete;
		ActorObjectHolder(ActorObjectHolder&&)      = default;
		ActorObjectHolder& operator=(const ActorObjectHolder&) = delete;
		ActorObjectHolder& operator=(ActorObjectHolder&&) = default;

		[[nodiscard]] inline constexpr auto& GetSlot(
			Data::ObjectSlot a_slot) noexcept
		{
			return m_entriesSlot[stl::underlying(a_slot)];
		}

		[[nodiscard]] inline constexpr const auto& GetSlot(
			Data::ObjectSlot a_slot) const noexcept
		{
			return m_entriesSlot[stl::underlying(a_slot)];
		}

		[[nodiscard]] inline constexpr auto& GetSlots() const noexcept
		{
			return m_entriesSlot;
		}

		[[nodiscard]] inline constexpr auto& GetActor() const noexcept
		{
			return m_actor;
		}

		[[nodiscard]] bool        AnySlotOccupied() const noexcept;
		[[nodiscard]] std::size_t GetNumOccupiedSlots() const noexcept;
		[[nodiscard]] std::size_t GetNumOccupiedCustom() const noexcept;

		[[nodiscard]] inline auto GetAge() const noexcept
		{
			return IPerfCounter::delta_us(m_created, IPerfCounter::Query());
		}

		[[nodiscard]] inline constexpr auto& GetHandle() const noexcept
		{
			return m_handle;
		}

		[[nodiscard]] inline auto& GetCustom(Data::ConfigClass a_class) noexcept
		{
			return m_entriesCustom[stl::underlying(a_class)];
		}

		[[nodiscard]] inline const auto& GetCustom(Data::ConfigClass a_class) const noexcept
		{
			return m_entriesCustom[stl::underlying(a_class)];
		}

		[[nodiscard]] inline constexpr const auto& GetCMENodes() const noexcept
		{
			return m_cmeNodes;
		}

		[[nodiscard]] inline constexpr const auto& GetMOVNodes() const noexcept
		{
			return m_movNodes;
		}

		[[nodiscard]] inline constexpr const auto& GetWeapNodes() const noexcept
		{
			return m_weapNodes;
		}

		[[nodiscard]] inline constexpr bool IsCellAttached() const noexcept
		{
			return m_cellAttached;
		}

		inline constexpr void UpdateCellAttached()
		{
			m_cellAttached = m_actor->IsParentCellAttached();
		}

		inline void RequestTransformUpdateDefer() const noexcept
		{
			if (!m_cmeNodes.empty() ||
			    !m_movNodes.empty())
			{
				m_flags.set(ActorObjectHolderFlags::kRequestTransformUpdateDefer);
			}
		}

		inline void RequestTransformUpdateDeferNoSkip() const noexcept
		{
			if (!m_cmeNodes.empty() ||
			    !m_movNodes.empty())
			{
				m_flags.set(ActorObjectHolderFlags::kWantTransformUpdate);
			}
		}

		inline void RequestTransformUpdate() const noexcept
		{
			if (!m_cmeNodes.empty() ||
			    !m_movNodes.empty())
			{
				m_flags.set(ActorObjectHolderFlags::kRequestTransformUpdateImmediate);
			}
		}

		inline constexpr void RequestEvalDefer() const noexcept
		{
			m_flags.set(ActorObjectHolderFlags::kRequestEval);
			if (m_flagsbf.evalCountdown == 0)
			{
				m_flagsbf.evalCountdown = 2;
			}
		}

		inline constexpr void RequestEval() const noexcept
		{
			m_flags.set(ActorObjectHolderFlags::kRequestEvalImmediate);
		}

		void ApplyActorState(const Data::actorStateEntry_t& a_data);

		[[nodiscard]] bool IsActorNPC(Game::FormID a_npc) const;
		[[nodiscard]] bool IsActorRace(Game::FormID a_race) const;

		using customEntryMap_t  = std::unordered_map<stl::fixed_string, objectEntryCustom_t>;
		using customPluginMap_t = std::unordered_map<stl::fixed_string, customEntryMap_t>;

		template <class Tv>
		void visit(Tv a_func)
		{
			for (auto& e : m_entriesSlot)
			{
				a_func(e);
			}

			for (auto& e : m_entriesCustom)
			{
				for (auto& f : e)
				{
					for (auto& g : f.second)
					{
						a_func(g.second);
					}
				}
			}
		}

		template <class Tv>
		void visit(Tv a_func) const
		{
			for (auto& e : m_entriesSlot)
			{
				a_func(e);
			}

			for (auto& e : m_entriesCustom)
			{
				for (auto& f : e)
				{
					for (auto& g : f.second)
					{
						a_func(g.second);
					}
				}
			}
		}

		[[nodiscard]] inline constexpr auto& GetActorFormID() const noexcept
		{
			return m_formid;
		}

	private:
		void CreateExtraNodes(
			NiNode*                                   a_npcroot,
			bool                                      a_female,
			const NodeOverrideData::extraNodeEntry_t& a_entry);

		void CreateExtraCopyNode(
			NiNode*                                       a_npcroot,
			const NodeOverrideData::extraNodeCopyEntry_t& a_entry);

		Game::ObjectRefHandle m_handle;
		long long             m_created{ 0 };

		union
		{
			mutable stl::flag<ActorObjectHolderFlags> m_flags{ ActorObjectHolderFlags::kNone };
			mutable ActorObjectHolderFlagsBitfield    m_flagsbf;
		};

		slot_container_type m_entriesSlot;
		customPluginMap_t   m_entriesCustom[Data::CONFIG_CLASS_MAX];

		std::vector<monitorNodeEntry_t>                       m_monitorNodes;
		std::unordered_map<stl::fixed_string, cmeNodeEntry_t> m_cmeNodes;
		std::unordered_map<stl::fixed_string, movNodeEntry_t> m_movNodes;
		std::vector<weapNodeEntry_t>                          m_weapNodes;

		NiPointer<Actor>  m_actor;
		NiPointer<NiNode> m_root;
		NiPointer<NiNode> m_npcroot;

		Game::FormID m_formid;
		bool         m_female{ false };

		bool                m_cellAttached{ false };
		bool                m_isPlayerTeammate{ false };
		bool                m_wantLFUpdate{ false };
		long long           m_lastLFStateCheck;
		actorLocationData_t m_locData;
		TESPackage*         m_currentPackage{ nullptr };

		IObjectManager& m_owner;

		static std::atomic_llong m_lfsc_delta;
	};

	using ActorObjectMap = std::unordered_map<Game::FormID, ActorObjectHolder>;

	class ObjectManagerData
	{
	public:
		template <class... Args>
		[[nodiscard]] inline constexpr auto& GetObjectHolder(
			Actor* a_actor,
			Args&&... a_args)
		{
			return m_objects.try_emplace(
								a_actor->formID,
								m_playerState,
								a_actor,
								std::forward<Args>(a_args)...)
			    .first->second;
		}

		[[nodiscard]] inline constexpr const auto& GetData() const noexcept
		{
			return m_objects;
		}

		inline constexpr void ClearPlayerState() noexcept
		{
			m_playerState.clear();
		}

	protected:
		ActorObjectMap                         m_objects;
		stl::optional<Data::actorStateEntry_t> m_playerState;
	};

	/*constexpr bool cmeNodeEntry_t::find_visible_geometry(NiAVObject* a_object) noexcept
	{
		if (!a_object->IsVisible())
		{
			return false;
		}

		if (a_object->GetAsBSGeometry())
		{
			return true;
		}

		if (auto node = a_object->GetAsNiNode())
		{
			for (auto object : node->m_children)
			{
				if (object)
				{
					if (find_visible_geometry(object))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	constexpr bool cmeNodeEntry_t::find_visible_geometry(
		NiAVObject*          a_object,
		const BSFixedString& a_scb,
		const BSFixedString& a_scbLeft) noexcept
	{
		if (!a_object->IsVisible() ||
		    a_object->m_name == a_scb ||
		    a_object->m_name == a_scbLeft)
		{
			return false;
		}

		if (a_object->GetAsBSGeometry())
		{
			return true;
		}

		if (auto node = a_object->GetAsNiNode())
		{
			for (auto object : node->m_children)
			{
				if (object)
				{
					if (find_visible_geometry(
							object,
							a_scb,
							a_scbLeft))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	
	constexpr bool cmeNodeEntry_t::has_visible_geometry() const noexcept
	{
		return find_visible_geometry(node);
	}

	constexpr bool cmeNodeEntry_t::has_visible_geometry(
		const BSFixedString& a_scb,
		const BSFixedString& a_scbLeft) const noexcept
	{
		return find_visible_geometry(node, a_scb, a_scbLeft);
	}*/

}
#pragma once

#include "IED/ActorInfoEntry.h"
#include "IED/ConfigStore.h"

#include "ObjectManagerData.h"

namespace IED
{
	class IActorInfo
	{
	public:
		void UpdateActorInfo(const ActorObjectMap& a_cache);
		void UpdateActorInfo(const ActorObjectHolder& a_objectHolder);
		void UpdateActorInfo(const ActorObjectMap& a_cache, Game::FormID a_actor);
		bool UpdateNPCInfo(Game::FormID a_npc);
		bool UpdateActorInfo(Game::FormID a_actor);

		void ClearActorInfo();

		[[nodiscard]] inline constexpr const auto& GetActorInfo() const noexcept
		{
			return m_actorInfo;
		}

		[[nodiscard]] inline constexpr const auto& GetNPCInfo() const noexcept
		{
			return m_npcInfo;
		}

		[[nodiscard]] inline constexpr auto GetActorInfoUpdateID() const noexcept
		{
			return m_actorInfoUpdateID;
		}

		[[nodiscard]] inline constexpr const auto& GetCrosshairRef() const noexcept
		{
			return m_crosshairRef;
		}

		bool LookupCrosshairRef(NiPointer<TESObjectREFR>& a_out);

		void FillActorInfoEntry(Actor* a_actor, actorInfoEntry_t& a_out, bool a_updateNPC = false);
		void FillNPCInfoEntry(TESNPC* a_npc, npcInfoEntry_t& a_out);

	private:
		virtual constexpr const Data::configStore_t& AIGetConfigStore() noexcept = 0;

		void AddExtraActorEntry(Game::FormID a_formid);
		void AddExtraNPCEntry(Game::FormID a_formid);

		static std::optional<Game::ObjectRefHandle> GetTargetActortHandle();

		std::optional<Game::FormID> m_crosshairRef;

		std::uint64_t m_actorInfoUpdateID{ 0 };

		ActorInfoHolder m_actorInfo;
		NPCInfoHolder   m_npcInfo;
	};
}
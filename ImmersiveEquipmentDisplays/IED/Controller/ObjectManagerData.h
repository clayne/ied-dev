#pragma once

#include "IED/ActorState.h"
#include "ActorObjectHolder.h"

namespace IED
{

	/*class AnimationEventFilter
	{
	public:
		SKMP_FORCEINLINE bool test(const BSFixedString& a_event)
		{
			if (m_allow.contains(a_event))
			{
				return true;
			}

			return !m_denyAll &&
			       !m_deny.contains(a_event);
		}

		bool                              m_denyAll{ false };
		stl::unordered_set<BSFixedString> m_allow;
		stl::unordered_set<BSFixedString> m_deny;
	};*/

	/*class AnimationGraphManagerHolderList
	{
	public:
		void Add(RE::WeaponAnimationGraphManagerHolderPtr& a_ptr)
		{
			stl::scoped_lock lock(m_lock);

			m_data.emplace_back(a_ptr);
		}

		void Remove(RE::WeaponAnimationGraphManagerHolderPtr& a_ptr)
		{
			stl::scoped_lock lock(m_lock);

			auto it = std::find(m_data.begin(), m_data.end(), a_ptr);
			if (it != m_data.end())
			{
				m_data.erase(it);
			}
		}

		void Notify(const BSFixedString& a_event) const
		{
			stl::scoped_lock lock(m_lock);

			for (auto& e : m_data)
			{
				e->NotifyAnimationGraph(a_event);
			}
		}

		void Update(const BSAnimationUpdateData& a_data) const;

		void UpdateNoLock(const BSAnimationUpdateData& a_data) const;

		void Clear() noexcept
		{
			stl::scoped_lock lock(m_lock);

			m_data.clear();
		}

		inline constexpr auto& GetList() const noexcept
		{
			return m_data;
		}

		inline bool Empty() const noexcept
		{
			return m_data.empty();
		}

	private:
		mutable stl::critical_section                       m_lock;
		stl::list<RE::WeaponAnimationGraphManagerHolderPtr> m_data;
	};*/ 

	
	using ActorObjectMap = stl::unordered_map<Game::FormID, ActorObjectHolder>;

	class ObjectManagerData
	{
	public:
		template <class... Args>
		[[nodiscard]] inline constexpr auto& GetObjectHolder(
			Actor* a_actor,
			Args&&... a_args)
		{
			auto r = m_objects.try_emplace(
				a_actor->formID,
				a_actor,
				std::forward<Args>(a_args)...);

			if (r.second)
			{
				ApplyActorState(r.first->second);
				OnActorAcquire(r.first->second);
			}

			return r.first->second;
		}

		[[nodiscard]] inline constexpr auto& GetData() const noexcept
		{
			return m_objects;
		}

		[[nodiscard]] inline constexpr auto& GetData() noexcept
		{
			return m_objects;
		}

		inline void ClearPlayerState() noexcept
		{
			m_playerState.reset();
		}

		void StorePlayerState(ActorObjectHolder& a_holder);

	private:
		void ApplyActorState(ActorObjectHolder& a_holder);

		virtual void OnActorAcquire(ActorObjectHolder& a_holder) = 0;

	protected:
		ActorObjectMap                           m_objects;
		std::unique_ptr<Data::actorStateEntry_t> m_playerState;
		Data::actorStateHolder_t                 m_storedActorStates;
	};

}
#include "pch.h"

#include "I3DIActorContext.h"

#include "I3DICommonData.h"
#include "I3DIObjectController.h"
#include "I3DIActorObject.h"

#include "IED/Controller/Controller.h"
#include "IED/Controller/NodeOverrideData.h"

namespace IED
{
	namespace UI
	{
		I3DIActorContext::I3DIActorContext(
			I3DICommonData&                    a_data,
			Controller&                        a_controller,
			Game::FormID                       a_actor,
			const std::shared_ptr<I3DIActorObject>& a_actorObject) noexcept(false) :
			m_controller(a_controller),
			m_actor(a_actor),
			m_actorObject(a_actorObject)
		{
			auto& data = m_controller.GetData();

			auto it = data.find(m_actor);
			if (it == data.end())
			{
				throw std::exception("actor doesn't exist");
			}

			auto& wn = it->second.GetWeapNodes();

			auto& nod = NodeOverrideData::GetWeaponNodeData();

			for (auto& e : nod)
			{
				auto& model = a_data.assets.GetModel(e.second.modelID);
				if (!model)
				{
					continue;
				}

				auto itwn = std::find_if(
					wn.begin(),
					wn.end(),
					[&](auto& a_v) {
						return a_v.nodeName == e.first;
					});

				if (itwn == wn.end())
				{
					continue;
				}

				m_weaponNodes.emplace(
					e.first,
					std::make_unique<I3DIWeaponNode>(
						a_data.scene.GetDevice().Get(),
						a_data.scene.GetContext().Get(),
						model,
						e.first,
						e.second,
						*this));
			}
		}

		void I3DIActorContext::RegisterObjects(
			I3DIObjectController& a_objectController)
		{
			for (auto& e : m_weaponNodes)
			{
				a_objectController.RegisterObject(e.second);
			}
		}

		void I3DIActorContext::UnregisterObjects(
			I3DIObjectController& a_objectController)
		{
			for (auto& e : m_weaponNodes)
			{
				a_objectController.UnregisterObject(e.second);
			}
		}

		bool I3DIActorContext::Update()
		{
			auto& data = m_controller.GetData();

			auto it = data.find(m_actor);
			if (it == data.end())
			{
				m_lastUpdateFailed = true;
				return false;
			}

			auto& wn = it->second.GetWeapNodes();

			for (auto& e : wn)
			{
				auto itwn = m_weaponNodes.find(e.nodeName);
				if (itwn == m_weaponNodes.end())
				{
					continue;
				}

				itwn->second->UpdateLocalMatrix(e.node->m_localTransform);
				itwn->second->UpdateWorldMatrix(e.node->m_worldTransform);
				itwn->second->UpdateBound();
			}

			m_lastUpdateFailed = false;
			m_ranFirstUpdate   = true;

			return true;
		}

		void I3DIActorContext::Draw(I3DICommonData& a_data)
		{
			if (!m_ranFirstUpdate)
			{
				return;
			}
		}

		void I3DIActorContext::Render(I3DICommonData& a_data)
		{
			if (!m_ranFirstUpdate)
			{
				return;
			}

			for (auto& e : m_weaponNodes)
			{
				e.second->RenderObject(a_data.scene);
			}
		}
	}
}
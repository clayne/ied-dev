#include "pch.h"

#include "Input.h"

#include <ext/IHook.h>
#include <ext/JITASM.h>

namespace IED
{
	namespace Drivers
	{
		using namespace ::Events;

		Input Input::m_Instance;

		bool Input::PlayerControls_InputEvent_ProcessEvent_Hook(
			InputEvent** a_evns)
		{
			bool blocked = m_Instance.m_playerInputHandlingBlocked.load(
				std::memory_order_relaxed);

			if (blocked)
			{
				// nasty, implement this properly
				if (auto pc = PlayerControls::GetSingleton())
				{
					pc->lookInput = { 0, 0 };
					pc->moveInput = { 0, 0 };
					pc->prevMove = { 0, 0 };
					pc->prevLook = { 0, 0 };
				}
			}

			if (a_evns)
			{
				for (auto inputEvent = *a_evns; inputEvent; inputEvent = inputEvent->next)
				{
					if (inputEvent->eventType != InputEvent::kEventType_Button)
					{
						continue;
					}

					auto buttonEvent = RTTI<ButtonEvent>()(inputEvent);
					if (!buttonEvent)
					{
						continue;
					}

					std::uint32_t deviceType = buttonEvent->deviceType;
					std::uint32_t keyCode;

					if (deviceType == kDeviceType_Mouse)
					{
						keyCode = InputMap::kMacro_MouseButtonOffset + buttonEvent->keyMask;
					}
					else if (deviceType == kDeviceType_Keyboard)
					{
						keyCode = buttonEvent->keyMask;
					}
					else
					{
						continue;
					}

					if (keyCode >= InputMap::kMaxMacros)
					{
						continue;
					}

					if (buttonEvent->flags != 0)
					{
						if (buttonEvent->timer == 0.0f)
						{
							m_Instance.DispatchPriorityKeyEvent(
								Handlers::KeyEventType::KeyDown,
								keyCode);
						}
					}
					else
					{
						m_Instance.DispatchPriorityKeyEvent(
							Handlers::KeyEventType::KeyUp,
							keyCode);
					}
				}
			}

			if (m_Instance.m_nextIEPCall)
			{
				blocked |= m_Instance.m_nextIEPCall(a_evns);
			}

			return blocked;
		}

		auto Input::ReceiveEvent(InputEvent** evns, InputEventDispatcher* dispatcher)
			-> EventResult
		{
			if (*evns)
			{
				for (auto inputEvent = *evns; inputEvent; inputEvent = inputEvent->next)
				{
					if (inputEvent->eventType != InputEvent::kEventType_Button)
					{
						continue;
					}

					auto buttonEvent = RTTI<ButtonEvent>::Cast(inputEvent);
					if (!buttonEvent)
					{
						continue;
					}

					if (buttonEvent->deviceType != kDeviceType_Keyboard)
					{
						continue;
					}

					std::uint32_t keyCode = buttonEvent->keyMask;

					if (keyCode >= InputMap::kMaxMacros)
					{
						continue;
					}

					if (buttonEvent->flags != 0)
					{
						if (buttonEvent->timer == 0.0f)
						{
							DispatchKeyEvent(Handlers::KeyEventType::KeyDown, keyCode);
						}
					}
					else
					{
						DispatchKeyEvent(Handlers::KeyEventType::KeyUp, keyCode);
					}
				}
			}

			return kEvent_Continue;
		}

		template <bool _IsAE, class T>
		static constexpr bool ExtractHookCallAddr(std::uintptr_t a_dst, T& a_out)
		{
#pragma pack(push, 1)
			struct payload_se_t
			{
				// mov rcx, r12
				std::uint8_t m[3];
				// call ..
				std::uint8_t escape;
				std::uint8_t modrm;
				std::uint32_t displ;
				// ...
			};

			struct payload_ae_t
			{
				/*	
					push rax
					push rcx
					push r11
					sub rsp,
					mov rcx, r12
				*/
				std::uint8_t m[11];
				// call ..
				std::uint8_t escape;
				std::uint8_t modrm;
				std::uint32_t displ;
				// ...
			};

#pragma pack(pop)

			using payload_t = std::conditional_t<_IsAE, payload_ae_t, payload_se_t>;

			std::uintptr_t tmp;
			if (!Hook::GetDst5<0xE9>(a_dst, tmp))
			{
				return false;
			}

			payload_t* p{ nullptr };

			if (!Hook::GetDst6<0x25>(tmp, p))
			{
				return false;
			}

			if constexpr (_IsAE)
			{
				constexpr std::uint8_t d[]{
					0x50,
					0x51,
					0x41,
					0x53,
					0x48,
					0x83,
					0xEC,
					0x28,
					0x4C,
					0x89,
					0xE1
				};

				if (!Patching::validate_mem(std::uintptr_t(p->m), d))
				{
					return false;
				}
			}
			else
			{
				constexpr std::uint8_t d[]{
					0x4C,
					0x89,
					0xE1
				};

				if (!Patching::validate_mem(std::uintptr_t(p->m), d))
				{
					return false;
				}
			}

			return Hook::GetDst6<0x15>(std::uintptr_t(std::addressof(p->escape)), a_out);
		}

		void Input::InstallPriorityHook()
		{
			struct ProcessInputEvent : JITASM::JITASM
			{
				ProcessInputEvent(std::uintptr_t a_targetAddr) :
					JITASM(ISKSE::GetLocalTrampoline())
				{
					Xbyak::Label exitSkipLabel;
					Xbyak::Label exitContinueLabel;
					Xbyak::Label callLabel;

					Xbyak::Label skip;

					if (IAL::IsAE())
					{
						push(rax);
						push(rcx);
						push(r11);
						sub(rsp, 0x28);

						mov(rcx, r12);
						call(ptr[rip + callLabel]);
						mov(dl, al);

						add(rsp, 0x28);
						pop(r11);
						pop(rcx);
						pop(rax);

						test(dl, dl);
						jne(skip);

						cmp(rsi, rbx);
					}
					else
					{
						mov(rcx, r12);
						call(ptr[rip + callLabel]);
						test(al, al);
						jne(skip);

						cmp(rdi, rbx);
					}

					je(skip);

					jmp(ptr[rip + exitContinueLabel]);

					L(skip);
					jmp(ptr[rip + exitSkipLabel]);

					L(exitSkipLabel);
					dq(a_targetAddr + (IAL::IsAE() ? 0x66 : 0x6A));

					L(exitContinueLabel);
					dq(a_targetAddr + 0x5);

					L(callLabel);
					dq(std::uintptr_t(PlayerControls_InputEvent_ProcessEvent_Hook));
				}
			};

			auto ehresult = IAL::IsAE() ?
                                ExtractHookCallAddr<true>(m_unkIED_a, m_Instance.m_nextIEPCall) :
                                ExtractHookCallAddr<false>(m_unkIED_a, m_Instance.m_nextIEPCall);

			if (ehresult)
			{
				m_Instance.Debug(
					"%s: extracted previous hook call address @%p",
					__FUNCTION__,
					m_Instance.m_nextIEPCall);
			}

			m_Instance.LogPatchBegin(__FUNCTION__);
			{
				ProcessInputEvent code(m_unkIED_a);

				ISKSE::GetBranchTrampoline().Write5Branch(m_unkIED_a, code.get());
			}
			m_Instance.LogPatchEnd(__FUNCTION__);
		}

		bool Input::SinkToInputDispatcher()
		{
			if (auto evd = InputEventDispatcher::GetSingleton())
			{
				evd->AddEventSink(std::addressof(m_Instance));
				return true;
			}
			else
			{
				return false;
			}
		}

		void Input::RegisterForPriorityKeyEvents(
			EventSink<Handlers::KeyEvent>* const a_handler)
		{
			m_Instance.m_prioHandlers.AddSink(a_handler);
		}

		void Input::RegisterForPriorityKeyEvents(
			EventSink<Handlers::KeyEvent>& a_handler)
		{
			m_Instance.m_prioHandlers.AddSink(std::addressof(a_handler));
		}

		void Input::RegisterForKeyEvents(
			EventSink<Handlers::KeyEvent>* const a_handler)
		{
			m_Instance.m_handlers.AddSink(a_handler);
		}

		void Input::RegisterForKeyEvents(
			EventSink<Handlers::KeyEvent>& a_handler)
		{
			m_Instance.m_handlers.AddSink(std::addressof(a_handler));
		}

		void Input::DispatchPriorityKeyEvent(
			Handlers::KeyEventType a_event,
			std::uint32_t a_keyCode)
		{
			m_prioHandlers.SendEvent({ a_event, a_keyCode });
		}

		void Input::DispatchKeyEvent(
			Handlers::KeyEventType a_event,
			std::uint32_t a_keyCode)
		{
			m_handlers.SendEvent({ a_event, a_keyCode });
		}
	}  // namespace Drivers
}  // namespace IED
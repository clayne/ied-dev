#include "pch.h"

#include "FormHolder.h"

namespace IED
{
	FormHolder FormHolder::m_Instance;

	void FormHolder::Populate()
	{
		m_Instance.layDown = Game::FormID(FID_LAYDOWN_KEYWORD).As<BGSKeyword>();
	}
}
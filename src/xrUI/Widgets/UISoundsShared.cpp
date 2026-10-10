#include "stdafx.h"
#include "UISoundsShared.h"

CUISoundsShared* g_soundsSharedInstance = nullptr;

CUISoundsShared::CUISoundsShared()
{
	m_accept_sound.create("interface\\console\\menu_accept", st_Effect, sg_SourceType);
	m_decline_sound.create("interface\\console\\menu_decline", st_Effect, sg_SourceType);
	m_select_sound.create("interface\\console\\menu_select", st_Effect, sg_SourceType);
	m_switch_sound.create("interface\\console\\menu_switch", st_Effect, sg_SourceType);
}

void CUISoundsShared::PlayAcceptSound()
{
	if (m_accept_sound.handle())
	{
		m_accept_sound.play(nullptr, sm_2D);
	}
}

void CUISoundsShared::PlayDeclineSound()
{
	if (m_decline_sound.handle())
	{
		m_decline_sound.play(nullptr, sm_2D);
	}
}

void CUISoundsShared::PlaySelectSound()
{
	if (m_select_sound.handle())
	{
		m_select_sound.play(nullptr, sm_2D);
	}
}

void CUISoundsShared::PlaySwitchSound()
{
	if (m_switch_sound.handle())
	{
		m_switch_sound.play(nullptr, sm_2D);
	}
}

UI_API CUISoundsShared* GetSoundsSharedInstance()
{
	if (!g_soundsSharedInstance)
	{
		g_soundsSharedInstance = new CUISoundsShared();
	}
	return g_soundsSharedInstance;
}
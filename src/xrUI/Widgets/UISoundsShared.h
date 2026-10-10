#pragma once

class UI_API CUISoundsShared
{
protected:
	ref_sound m_accept_sound;
	ref_sound m_decline_sound;
	ref_sound m_select_sound;
	ref_sound m_switch_sound;
public:
	CUISoundsShared();

	void PlayAcceptSound();
	void PlayDeclineSound();
	void PlaySelectSound();
	void PlaySwitchSound();
};

extern UI_API CUISoundsShared* GetSoundsSharedInstance();

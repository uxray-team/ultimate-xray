////////////////////////////////////////////////////////////////////////////
//	Module 		: UIGamepadKeyboard.h
//	Created 	: 09.11.2026
//	Author		: St4lker0k765
//	Description : On-screen keyboard for gamepad
////////////////////////////////////////////////////////////////////////////

#include "UIDialogWnd.h"
#include "UIWndCallback.h"
#include "UI3tButton.h"

class CUIStatic;
class CUIFrameWIndow;
class CUIGamepadLegend;
class CUICustomEdit;

class UI_API CUIOSKButton : public CUI3tButton
{
private:
	typedef CUI3tButton inherited;
public:
	enum GKAction
	{
		none = 0,
		swap_lang,
		caps_lock,
		commit_changes,
		prev_char,
		next_char,
		remove_char,
		space,
	};
	GKAction action = none;
	int grid_width = 1;
	shared_str m_text_lower;
	shared_str m_text_upper;

	virtual void OnClick();
	virtual void Update();
};

class UI_API CUIGamepadKeyboard : public CUIDialogWnd, public CUIWndCallback
{
private:
	typedef CUIDialogWnd inherited;
protected:
	CUIStatic* m_background = nullptr;
	Ivector2 m_active;

	typedef xr_vector<CUIOSKButton*> OSKButtonVec;
	xr_map<shared_str, xr_vector<OSKButtonVec>> ButtonRows;

	u8 m_current_language_idx = u8(-1);
	xr_map<u8, shared_str> LanguageNames;

	bool MoveSelectorInternal(eUIDirection4 dir);
	void MoveSelector(eUIDirection4 dir);
	void UpdateSelector();
	CUIOSKButton* GetButtonAt(const Ivector2& pos);
public:
	CUICustomEdit* m_custom_edit = nullptr;
	bool m_capitalized = false;
	bool m_capitalized_button_held = false;

	CUIGamepadKeyboard();
	virtual ~CUIGamepadKeyboard();

	CUIOSKButton* InitOSKButton(CUIXml& xml, const char* ui_path, int node_index);
	void AttachToCustomEdit(CUICustomEdit* edit);
	void UpdateButtonsText();
	void ChangeInputLanguage();
	shared_str GetInputLanguage();

	virtual void Update();
	virtual bool NeedCenterCursor() const { return false; }
	virtual void HideDialog();
	virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

	virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
	virtual bool OnGamepadKeyAction(int id, EUIMessages gamepad_action);
	virtual bool OnGamepadKeyHold(int id);
	virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
};

extern UI_API CUIGamepadKeyboard* GetGamepadKeyboardInstance();
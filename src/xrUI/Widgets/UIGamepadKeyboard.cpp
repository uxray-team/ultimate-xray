////////////////////////////////////////////////////////////////////////////
//	Module 		: UIGamepadKeyboard.cpp
//	Created 	: 09.11.2026
//	Author		: St4lker0k765
//	Description : On-screen keyboard for gamepad
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "UIGamepadKeyboard.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "UI3tButton.h"
#include "UISoundsShared.h"
#include "UICustomEdit.h"
#include "../../xrEngine/xr_input.h"
#include "../../xrEngine/xr_level_controller.h"
#include "../UICursor.h"

void CUIOSKButton::OnClick()
{
	inherited::OnClick();
	GetSoundsSharedInstance()->PlayAcceptSound();
	CUIGamepadKeyboard* parent = smart_cast<CUIGamepadKeyboard*>(GetParent());
	if (parent)
	{
		switch (action)
		{
		case none:
		{
			parent->m_custom_edit->CaptureFocus(true);
			parent->m_custom_edit->InsertCharacter(parent->m_capitalized ? m_text_upper.c_str() : m_text_lower.c_str());
			break;
		}
		case caps_lock:
		{
			parent->m_capitalized = !parent->m_capitalized;
			parent->m_custom_edit->ForceCapitalize(parent->m_capitalized);
			break;
		}
		case swap_lang:
		{
			parent->ChangeInputLanguage();
			break;
		}
		case commit_changes:
		{
			parent->HideDialog();
			break;
		}
		case prev_char:
		{
			parent->m_custom_edit->CaptureFocus(true);
			parent->m_custom_edit->OnKeyboardAction(SDL_SCANCODE_LEFT, WINDOW_KEY_PRESSED);
			break;
		}
		case next_char:
		{
			parent->m_custom_edit->CaptureFocus(true);
			parent->m_custom_edit->OnKeyboardAction(SDL_SCANCODE_RIGHT, WINDOW_KEY_PRESSED);
			break;
		}
		case remove_char:
		{
			parent->m_custom_edit->CaptureFocus(true);
			parent->m_custom_edit->OnKeyboardAction(SDL_SCANCODE_BACKSPACE, WINDOW_KEY_PRESSED);
			break;
		}
		case space:
		{
			parent->m_custom_edit->CaptureFocus(true);
			parent->m_custom_edit->OnKeyboardAction(SDL_SCANCODE_SPACE, WINDOW_KEY_PRESSED);
			break;
		}
		}
	}
}

void CUIOSKButton::Update()
{
	inherited::Update();
	if (action == caps_lock)
	{
		CUIGamepadKeyboard* parent = smart_cast<CUIGamepadKeyboard*>(GetParent());
		if (parent)
		{
			SetHighlighted(parent->m_capitalized);
		}
	}
}

#define OSK_INITIAL_DELAY 200
#define OSK_PERIOD 100
/////////////////////////////////
CUIGamepadKeyboard::CUIGamepadKeyboard()
{
	CUIXml xml;
	xml.Load(CONFIG_PATH, UI_PATH, "on_screen_keyboard.xml");

	CUIXmlInit::InitWindow(xml, "main", 0, this);

	m_background = UIHelper::CreateStatic(xml, "background", this);

	XML_NODE* rootSaved = xml.GetLocalRoot();
	int layoutCount = xml.GetNodesNum(xml.GetRoot(), "layout");
	for (int l = 0; l < layoutCount; ++l)
	{
		const char* language = xml.ReadAttrib("layout", l, "language", "default");
		LanguageNames[l] = language;
		xml.SetLocalRoot(xml.NavigateToNode("layout", l));
		XML_NODE* rootSavedLocal = xml.GetLocalRoot();

		int rowCount = xml.GetNodesNum(xml.GetLocalRoot(), "row");
		for (int i = 0; i < rowCount; ++i)
		{
			xml.SetLocalRoot(xml.NavigateToNode("row", i));

			int buttonCount = xml.GetNodesNum(xml.GetLocalRoot(), "button");
			xr_vector<CUIOSKButton*> btns;

			for (int j = 0; j < buttonCount; ++j)
			{
				CUIOSKButton* btn = InitOSKButton(xml, "button", j);
				btns.push_back(btn);
			}

			R_ASSERT(btns.size());
			ButtonRows[language].push_back(btns);
			xml.SetLocalRoot(rootSavedLocal);
		}
		xml.SetLocalRoot(rootSaved);
	}
	R_ASSERT(ButtonRows.size());

	// Use singular scale for all resolutions
	for (CUIWindow* child : m_ChildWndList)
	{
		Fvector2 pos = child->GetWndPos();
		pos.x *= UI().get_current_kx();
		child->SetWndPos(pos);

		float width = child->GetWidth() * UI().get_current_kx();
		child->SetWidth(width);
	}

	// set last lang from list, then reset to default
	m_current_language_idx = layoutCount - 1;
	ChangeInputLanguage();

	ActionRepeaters()->Register(this, kUI_TAB_LEFT, OSK_INITIAL_DELAY, OSK_PERIOD);
	ActionRepeaters()->Register(this, kUI_TAB_RIGHT, OSK_INITIAL_DELAY, OSK_PERIOD);

	ActionRepeaters()->Register(this, kUI_LEFT, OSK_INITIAL_DELAY, OSK_PERIOD);
	ActionRepeaters()->Register(this, kUI_RIGHT, OSK_INITIAL_DELAY, OSK_PERIOD);
	ActionRepeaters()->Register(this, kUI_UP, OSK_INITIAL_DELAY, OSK_PERIOD);
	ActionRepeaters()->Register(this, kUI_DOWN, OSK_INITIAL_DELAY, OSK_PERIOD);

	ActionRepeaters()->Register(this, kUI_ACTION_1, OSK_INITIAL_DELAY, OSK_PERIOD);
	ActionRepeaters()->Register(this, kUI_ACTION_2, OSK_INITIAL_DELAY, OSK_PERIOD);
}

CUIGamepadKeyboard::~CUIGamepadKeyboard()
{
}

CUIOSKButton* CUIGamepadKeyboard::InitOSKButton(CUIXml& xml, const char* ui_path, int node_index)
{
	auto ui = new CUIOSKButton();
	if (!CUIXmlInit::Init3tButton(xml, ui_path, node_index, ui, true))
	{
		R_ASSERT4(false, "Failed to create button for on-screen keyboard", ui_path, xml.m_xml_file_name);
	}
	else
	{
		const char* action_str = xml.ReadAttrib("button", node_index, "action", "none");
		auto action = magic_enum::enum_cast<CUIOSKButton::GKAction>(action_str);
		if (action.has_value())
		{
			ui->action = action.value();
		}
		else
		{
			Msg("! CUIGamepadKeyboard: Invalid action reference in node [%s], button number [%d], action string [%s]", xml.GetLocalRoot()->Value(), node_index, action_str);
			ui->action = CUIOSKButton::none;
		}

		ui->grid_width = xml.ReadAttribInt("button", node_index, "grid_width", 1);
		ui->m_text_lower = xml.Read("button:text", node_index, "");
		ui->m_text_upper = xml.Read("button:text_upper", node_index, ui->m_text_lower.c_str());

		AttachChild(ui);
		ui->SetAutoDelete(true);
	}
	return ui;
}

void CUIGamepadKeyboard::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

CUIOSKButton* CUIGamepadKeyboard::GetButtonAt(const Ivector2& pos)
{
	R_ASSERT(ButtonRows[GetInputLanguage()].size() > pos.y && ButtonRows[GetInputLanguage()][pos.y].size() > pos.x);
	CUIOSKButton* btn = ButtonRows[GetInputLanguage()][pos.y][pos.x];
	return btn;
}

// Return true if selector has been moved
bool CUIGamepadKeyboard::MoveSelectorInternal(eUIDirection4 dir)
{
	CUIOSKButton* button = GetButtonAt(m_active);
	button->SetHighlighted(false);

	Ivector2 m_active_old = m_active; 
	int newX = m_active.x;
	int newY = m_active.y;

	switch (dir)
	{
	case eUIDirection4_Down:
	{
		newY += 1;
		break;
	}
	case eUIDirection4_Up:
	{
		newY -= 1;
		break;
	}
	case eUIDirection4_Left:
	{
		newX += -1;
		break;
	}
	case eUIDirection4_Right:
	{
		newX += 1;
		break;
	}
	}

	if (newY < 0)
		newY = 0;
	else if (newY >= ButtonRows[GetInputLanguage()].size())
		newY = ButtonRows[GetInputLanguage()].size() - 1;

	if (newX < 0)
		newX = 0;
	else if (newX >= ButtonRows[GetInputLanguage()][newY].size())
		newX = ButtonRows[GetInputLanguage()][newY].size() - 1;


	m_active.set( newX, newY );
	return !m_active.similar(m_active_old);
}

void CUIGamepadKeyboard::MoveSelector(eUIDirection4 dir)
{
	if (MoveSelectorInternal(dir))
	{
		GetSoundsSharedInstance()->PlaySwitchSound();
	}
}

bool CUIGamepadKeyboard::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if (inherited::OnKeyboardAction(dik, keyboard_action))
		return true;

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
		if (is_binded(kUI_BACK, dik) || is_binded(kQUIT, dik))
		{
			HideDialog();
			return true;
		}
	}
	return false;
}

bool CUIGamepadKeyboard::OnGamepadKeyAction(int id, EUIMessages gamepad_action)
{
	if (inherited::OnGamepadKeyAction(id, gamepad_action))
		return true;

	if (WINDOW_KEY_PRESSED == gamepad_action)
	{
		if (is_binded(kUI_ACCEPT, id))
		{
			GetButtonAt(m_active)->OnClick();
			if (!m_capitalized_button_held)
			{
				m_capitalized = false;
				m_custom_edit->ForceCapitalize(m_capitalized);
			}
			return true;
		}
		else if (is_binded(kUI_TAB_SECONDARY_LEFT, id))
		{
			m_capitalized = !m_capitalized;
			m_capitalized_button_held = true;
			m_custom_edit->ForceCapitalize(m_capitalized);
			GetSoundsSharedInstance()->PlayAcceptSound();
			return true;
		}
		else if (is_binded(kUI_TAB_SECONDARY_RIGHT, id))
		{
			GetSoundsSharedInstance()->PlayAcceptSound();
			HideDialog();
			return true;
		}
		else if (is_binded(kUI_ACTION_1, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_ACTION_1);
			m_custom_edit->CaptureFocus(true);
			m_custom_edit->OnKeyboardAction(SDL_SCANCODE_BACKSPACE, WINDOW_KEY_PRESSED);
			GetSoundsSharedInstance()->PlayAcceptSound();
			return true;
		}
		else if (is_binded(kUI_ACTION_2, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_ACTION_2);
			m_custom_edit->CaptureFocus(true);
			m_custom_edit->OnKeyboardAction(SDL_SCANCODE_SPACE, WINDOW_KEY_PRESSED);
			GetSoundsSharedInstance()->PlayAcceptSound();
			return true;
		}
		else if (is_binded(kACTORMENU_ACTION, id))
		{
			ChangeInputLanguage();
			GetSoundsSharedInstance()->PlayAcceptSound();
			return true;
		}
		else if (is_binded(kUI_TAB_LEFT, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_TAB_LEFT);
			if (!any_binded_key_for_action_pressed_c(kUI_TAB_RIGHT))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_LEFT, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlaySwitchSound();
			}
			return true;
		}
		else if (is_binded(kUI_TAB_RIGHT, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_TAB_RIGHT);
			if (!any_binded_key_for_action_pressed_c(kUI_TAB_LEFT))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_RIGHT, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlaySwitchSound();
			}
			return true;
		}
		else if (is_binded(kUI_LEFT, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_LEFT);
			if (!any_binded_key_for_action_pressed_c(kUI_RIGHT))
			{
				MoveSelector(eUIDirection4_Left);
			}
			return true;
		}
		else if (is_binded(kUI_RIGHT, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_RIGHT);
			if (!any_binded_key_for_action_pressed_c(kUI_LEFT))
			{
				MoveSelector(eUIDirection4_Right);
			}
			return true;
		}
		else if (is_binded(kUI_UP, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_UP);
			if (!any_binded_key_for_action_pressed_c(kUI_DOWN))
			{
				MoveSelector(eUIDirection4_Up);
			}
			return true;
		}
		else if (is_binded(kUI_DOWN, id))
		{
			ActionRepeaters()->SetActionStarted(this, kUI_DOWN);
			if (!any_binded_key_for_action_pressed_c(kUI_UP))
			{
				MoveSelector(eUIDirection4_Down);
			}
			return true;
		}

		if (is_binded(kUI_BACK, id) || is_binded(kQUIT, id))
		{
			HideDialog();
			return true;
		}
	}
	else if (WINDOW_KEY_RELEASED == gamepad_action)
	{
		if (is_binded(kUI_TAB_SECONDARY_LEFT, id))
		{
			m_capitalized_button_held = false;
			return true;
		}
	}
	return false;
}

bool CUIGamepadKeyboard::OnGamepadKeyHold(int id)
{
	if (inherited::OnGamepadKeyHold(id))
		return true;

	switch (get_binded_action(id, agUIGeneral))
	{
		case kUI_ACTION_1:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_ACTION_1))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_BACKSPACE, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlayAcceptSound();
			}
			return true;
		}
		case kUI_ACTION_2:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_ACTION_2))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_SPACE, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlayAcceptSound();
			}
			return true;
		}
		case kUI_TAB_LEFT:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_TAB_LEFT) && !any_binded_key_for_action_pressed_c(kUI_TAB_RIGHT))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_LEFT, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlaySwitchSound();
			}
			return true;
		}
		case kUI_TAB_RIGHT:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_TAB_RIGHT) && !any_binded_key_for_action_pressed_c(kUI_TAB_LEFT))
			{
				m_custom_edit->CaptureFocus(true);
				m_custom_edit->OnKeyboardAction(SDL_SCANCODE_RIGHT, WINDOW_KEY_PRESSED);
				GetSoundsSharedInstance()->PlaySwitchSound();
			}
			return true;
		}
		case kUI_LEFT:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_LEFT) && !any_binded_key_for_action_pressed_c(kUI_RIGHT))
			{
				MoveSelector(eUIDirection4_Left);
			}
			return true;
		}
		case kUI_RIGHT:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_RIGHT) && !any_binded_key_for_action_pressed_c(kUI_LEFT))
			{
				MoveSelector(eUIDirection4_Right);
			}
			return true;
		}
		case kUI_UP:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_UP) && !any_binded_key_for_action_pressed_c(kUI_DOWN))
			{
				MoveSelector(eUIDirection4_Up);
			}
			return true;
		}
		case kUI_DOWN:
		{
			if (ActionRepeaters()->CanRepeatActionNow(this, kUI_DOWN) && !any_binded_key_for_action_pressed_c(kUI_UP))
			{
				MoveSelector(eUIDirection4_Down);
			}
			return true;
		}
	}

	return false;
}

void CUIGamepadKeyboard::AttachToCustomEdit(CUICustomEdit* edit)
{
	m_custom_edit = edit;
	// position keyboard below text control
	Fvector2 pos;
	edit->GetAbsolutePos(pos);
	pos.y += edit->GetHeight();
	SetWndPos(pos);
}

void CUIGamepadKeyboard::Update()
{
	inherited::Update();
	GetButtonAt(m_active)->SetHighlighted(pInput->GetControllerMode());
	UpdateButtonsText();
}

void CUIGamepadKeyboard::UpdateButtonsText()
{
	for (xr_vector<CUIOSKButton*> vec : ButtonRows[GetInputLanguage()])
	{
		for (CUIOSKButton* btn : vec)
		{
			if (btn->action != CUIOSKButton::none)
			{
				continue;
			}

			if (m_capitalized)
			{
				btn->SetText(btn->m_text_upper.c_str());
			}
			else
			{
				btn->SetText(btn->m_text_lower.c_str());
			}
		}
	}
}
shared_str CUIGamepadKeyboard::GetInputLanguage()
{
	return LanguageNames[m_current_language_idx];
}

void CUIGamepadKeyboard::ChangeInputLanguage()
{
	for (xr_vector<CUIOSKButton*> vec : ButtonRows[GetInputLanguage()])
	{
		for (CUIOSKButton* btn : vec)
		{
			btn->Show(false);
		}
	}
	GetButtonAt(m_active)->SetHighlighted(false);

	u8 prev_idx = m_current_language_idx;
	m_current_language_idx++;
	if (m_current_language_idx >= LanguageNames.size())
	{
		m_current_language_idx = 0;
	}

	if (m_active.y + 1 == ButtonRows[LanguageNames[prev_idx]].size())
	{
		m_active.y = ButtonRows[GetInputLanguage()].size() - 1;

	}
	if (m_active.y >= ButtonRows[GetInputLanguage()].size())
	{
		m_active.y = ButtonRows[GetInputLanguage()].size() - 1;
	}

	for (xr_vector<CUIOSKButton*> vec : ButtonRows[GetInputLanguage()])
	{
		for (CUIOSKButton* btn : vec)
		{
			btn->Show(true);
		}
	}
}

void CUIGamepadKeyboard::HideDialog()
{
	if (!IsShown())
	{
		return;
	}

	m_custom_edit->CaptureFocus(false);
	m_custom_edit->ForceCapitalize(false);
	m_custom_edit->GetParent()->SetKeyboardCapture(this, false);
	m_custom_edit->GetMessageTarget()->SendMessage(this, EDIT_TEXT_COMMIT, nullptr);

	GetHolder()->StopDialog(this);
}

bool CUIGamepadKeyboard::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	Fvector2 pos = UI().GetUICursor().GetCursorPosition();
	Frect r;
	GetAbsoluteRect(r);
	if (WINDOW_LBUTTON_DOWN == mouse_action && !r.in(pos.x, pos.y))
	{
		HideDialog();
	}

	return inherited::OnMouseAction(x, y, mouse_action);
}

CUIGamepadKeyboard* g_GamepadKeyboard = nullptr;
UI_API CUIGamepadKeyboard* GetGamepadKeyboardInstance()
{
	if (!g_GamepadKeyboard)
	{
		g_GamepadKeyboard = new CUIGamepadKeyboard();
	}
	return g_GamepadKeyboard;
}
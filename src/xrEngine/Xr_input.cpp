#include "stdafx.h"
#include "string_table.h"

#include "xr_input.h"
#include "IInputReceiver.h"
#include "GamepadService.h"

CInput *	pInput	= nullptr;
IInputReceiver		dummyController;

ENGINE_API float	psMouseSens			= 0.12f;
ENGINE_API float	psMouseUISens		= 1.f;
ENGINE_API float	psMouseSensScale	= 1.f;
ENGINE_API bool		psMouseInvert		= false;

ENGINE_API float	psGamepadSens		= 0.3f;
ENGINE_API bool		psGamepadInvert		= false;

ENGINE_API bool		psGyroscopeInvertX	= false;
ENGINE_API bool		psGyroscopeInvertY	= false;

ENGINE_API float	psTouchpadSens		= 4.0f;

static bool g_exclusive	= true;
static void on_error_dialog			(bool before)
{
	if (!pInput || !g_exclusive)
		return;

	if (before) {
		pInput->unacquire	();
		return;
	}

	pInput->acquire();
}

CInput::CInput(bool bExclusive)
{
	g_exclusive = !!bExclusive;

	Log("Starting INPUT device...");

	ZeroMemory(mouseState, sizeof(mouseState));
	ZeroMemory(KBState, sizeof(KBState));

	GGamepadService = new CGamepadService;

	iCapture(&dummyController);
	Debug.set_on_dialog(&on_error_dialog);

	for (u8 i = 0; i < COUNT_FINGERS_TOUCHPAD; i++)
	{
		TouchpadFingerPresent[i] = false;
	}
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this, REG_PRIORITY_HIGH);
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH);
}

CInput::~CInput()
{
	xr_delete(GGamepadService);

	Device.seqFrame.Remove(this);
	Device.seqAppDeactivate.Remove(this);
	Device.seqAppActivate.Remove(this);
}

//-----------------------------------------------------------------------

void CInput::MouseMotion(float dx, float dy)
{
	mouseMoved = true;
	offs[0] += (int)dx;
	offs[1] += (int)dy;
}

void CInput::MouseScroll(float d)
{
	mouseScrolled = true;
	offs[2] += (int)d;
}

void CInput::MousePressed(int button)
{
	mouseState[button] = 1;
}

void CInput::MouseReleased(int button)
{
	mouseState[button] = 0;
}

void CInput::KeyboardButtonUpdate(int scancode, bool IsPressed)
{
	KBState[scancode] = IsPressed;
}

#define DEADZONE_RADIUS_UI 0.375f
void CInput::LeftAxisUpdate(bool IsX, float value)
{
	if (IsX)
	{
		LeftAxis.x = value;
	}
	else
	{
		LeftAxis.y = value * -1;
	}

	if (LeftAxis.x < -DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_LEFT, true);
	}
	else if (LeftAxis.x > DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_RIGHT, true);
	}
	else
	{
		if (KBState[UXR_GAMEPAD_LSTICK_LEFT])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_LEFT, false);
		}
		if (KBState[UXR_GAMEPAD_LSTICK_RIGHT])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_RIGHT, false);
		}
	}

	if (LeftAxis.y < -DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_DOWN, true);
	}
	else if (LeftAxis.y > DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_UP, true);
	}
	else
	{
		if (KBState[UXR_GAMEPAD_LSTICK_DOWN])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_DOWN, false);
		}
		if (KBState[UXR_GAMEPAD_LSTICK_UP])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_LSTICK_UP, false);
		}
	}
}

void CInput::RightAxisUpdate(bool IsX, float value)
{
	if (IsX)
	{
		RightAxis.x = value;
	}
	else
	{
		RightAxis.y = value;
	}

	if (RightAxis.x < -DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_LEFT, true);
	}
	else if (RightAxis.x > DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_RIGHT, true);
	}
	else
	{
		if (KBState[UXR_GAMEPAD_RSTICK_LEFT])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_LEFT, false);
		}
		if (KBState[UXR_GAMEPAD_RSTICK_RIGHT])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_RIGHT, false);
		}
	}

	if (RightAxis.y < -DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_UP, true);
	}
	else if (RightAxis.y > DEADZONE_RADIUS_UI)
	{
		pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_DOWN, true);
	}
	else
	{
		if (KBState[UXR_GAMEPAD_RSTICK_DOWN])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_DOWN, false);
		}
		if (KBState[UXR_GAMEPAD_RSTICK_UP])
		{
			pInput->KeyboardButtonUpdate(UXR_GAMEPAD_RSTICK_UP, false);
		}
	}
}

void CInput::AdaptiveTriggerUpdate(bool IsX, float value)
{
	if (IsX)
	{
		AdaptiveTrigger.x = value;
	}
	else
	{
		AdaptiveTrigger.y = value;
	}
}

void CInput::GamepadGyroscopeUpdate(Fvector3 value)
{
	Gyroscope += value;
	gyroscopeMoved = true;
}

#define MOUSE_1		(SDL_SCANCODE_COUNT + 100)
#define MOUSE_8		(SDL_SCANCODE_COUNT + 107)

void CInput::KeyboardUpdate()
{
	for (size_t i = 0; i < COUNT_KB_BUTTONS; i++)
	{
		bool isGamepad = i >= SDL_SCANCODE_COUNT && i < MOUSE_1;
		bool Pressed = !!KBState[i];
		if (KBState[i] != old_KBState[i])
		{
			old_KBState[i] = KBState[i];
			if (Pressed)
			{
				if (isGamepad)
				{
					cbStack.back()->IR_GamepadKeyPress(i);
				}
				else
				{
					cbStack.back()->IR_OnKeyboardPress(i);
				}
			}
			else
			{
				if (isGamepad)
				{
					cbStack.back()->IR_GamepadKeyRelease(i);
				}
				else
				{
					cbStack.back()->IR_OnKeyboardRelease(i);
				}
			}
		}
	}

	for (int i = 0; i < COUNT_KB_BUTTONS; i++)
	{
		bool isGamepad = i >= SDL_SCANCODE_COUNT && i < MOUSE_1;
		if (KBState[i])
		{
			if (isGamepad)
			{
				cbStack.back()->IR_GamepadKeyHold(i);
			}
			else
			{
				cbStack.back()->IR_OnKeyboardHold(i);
			}
		}
	}
}

void CInput::GamepadUpdate()
{
	if (GGamepadService->GamePadDevice == nullptr)
	{
		return;
	}

	GGamepadService->Update();
	if (cbStack.empty())
	{
		return;
	}

	auto KeyHolder = cbStack.back();

	KeyHolder->IR_GamepadUpdateStick(0, LeftAxis);
	KeyHolder->IR_GamepadUpdateStick(1, RightAxis);

	KeyHolder->IR_GamepadUpdateStick(2, AdaptiveTrigger);
}

const xr_map<int, char> russian_lookup_key_table = {
	{ SDL_SCANCODE_F, 0xE0 },
	{ SDL_SCANCODE_COMMA, 0xE1 },
	{ SDL_SCANCODE_D, 0xE2 },
	{ SDL_SCANCODE_U, 0xE3 },
	{ SDL_SCANCODE_L, 0xE4 },
	{ SDL_SCANCODE_T, 0xE5 },
	{ SDL_SCANCODE_SEMICOLON, 0xE6 },
	{ SDL_SCANCODE_P, 0xE7 },
	{ SDL_SCANCODE_B, 0xE8 },
	{ SDL_SCANCODE_Q, 0xE9 },
	{ SDL_SCANCODE_R, 0xEA },
	{ SDL_SCANCODE_K, 0xEB },
	{ SDL_SCANCODE_V, 0xEC },
	{ SDL_SCANCODE_Y, 0xED }, 
	{ SDL_SCANCODE_J, 0xEE }, 
	{ SDL_SCANCODE_G, 0xEF }, 	
	
	{ SDL_SCANCODE_H, 0xF0 },
	{ SDL_SCANCODE_C, 0xF1 },
	{ SDL_SCANCODE_N, 0xF2 },
	{ SDL_SCANCODE_E, 0xF3 },
	{ SDL_SCANCODE_A, 0xF4 },
	{ SDL_SCANCODE_LEFTBRACKET, 0xF5 },
	{ SDL_SCANCODE_W, 0xF6 },
	{ SDL_SCANCODE_X, 0xF7 },
	{ SDL_SCANCODE_I, 0xF8 },
	{ SDL_SCANCODE_O, 0xF9 },
	{ SDL_SCANCODE_RIGHTBRACKET, 0xFA },
	{ SDL_SCANCODE_S, 0xFB },
	{ SDL_SCANCODE_M, 0xFC },
	{ SDL_SCANCODE_APOSTROPHE, 0xFD },
	{ SDL_SCANCODE_PERIOD, 0xFE },
	{ SDL_SCANCODE_Z, 0xFF }, 
};

bool CInput::get_dik_name(int dik, LPSTR dest_str, int dest_sz)
{
#ifdef IXR_WINDOWS
	LANGID lang_locale = PRIMARYLANGID(LOWORD(HandleToLong(GetKeyboardLayout(0))));
	if (lang_locale != LANG_RUSSIAN) {
		return false;
	}

	if (g_AppInfo.IsLaunchedViaWineOrProton) {
		return false;
	}
#endif

	if (!russian_lookup_key_table.contains(dik)) {
		return false;
	}

	char sym = russian_lookup_key_table.at(dik);
	dest_str[0] = sym;
	dest_str[1] = 0;

	return true;
}

bool CInput::iGetAsyncKeyState( int dik )
{
	if(dik<COUNT_KB_BUTTONS)
		return !!KBState[dik];
	else
	if(dik>=MOUSE_1 && dik<=MOUSE_8)
	{
		int mk = dik-MOUSE_1;
		return iGetAsyncBtnState(mk);
	}else
		return false; //unknown key ???
}

bool CInput::iGetAsyncBtnState( int btn )
{
	return !!mouseState[btn];
}

#pragma warning(push)
#pragma warning(disable: 4644)
void CInput::NoInputUpdate()
{
	for (size_t i = 0; i < COUNT_KB_BUTTONS; i++) 
	{
		bool Pressed = !!KBState[i];
		if (KBState[i] != old_KBState[i])
		{
			if (!Pressed) 
			{
				cbStack.back()->IR_OnKeyboardRelease((int)i);
			}

			old_KBState[i] = KBState[i];
		}
	}

	for (size_t i = 0; i < COUNT_MOUSE_BUTTONS; i++) 
	{
		bool Pressed = !!mouseState[i];
		if (mouseState[i] != old_mouseState[i]) 
		{
			if (!Pressed) 
			{
				cbStack.back()->IR_OnMouseRelease((int)i);
			}

			old_mouseState[i] = mouseState[i];
		}
	}

	offs[0] = offs[1] = offs[2] = 0;
}

void CInput::MouseUpdate( )
{
	if (Device.dwPrecacheFrame)
		return;

	for (size_t i = 0; i < COUNT_MOUSE_BUTTONS; i++) {
		bool Pressed = !!mouseState[i];
		if (mouseState[i] != old_mouseState[i]) {
			if (Pressed) {
				cbStack.back()->IR_OnMousePress((int)i);
			} else {
				cbStack.back()->IR_OnMouseRelease((int)i);
			}
		}
	}

	for (int i = 0; i < COUNT_MOUSE_BUTTONS; i++) {
		if (mouseState[i] && old_mouseState[i]) {
			cbStack.back()->IR_OnMouseHold(i);
		}
	}

	if (mouseMoved) {
		cbStack.back()->IR_OnMouseMove(offs[0], offs[1]);
		mouseMoved = false;
	}

	if (mouseScrolled) {
		cbStack.back()->IR_OnMouseWheel(offs[2]);
		mouseScrolled = false;
	}

	std::memcpy(old_mouseState, mouseState, sizeof(mouseState));
	offs[0] = offs[1] = offs[2] = 0;
}

void CInput::GyroscopeUpdate()
{
	if (Device.dwPrecacheFrame)
	{
		return;
	}

	if (gyroscopeMoved) 
	{
		cbStack.back()->IR_OnGyroscopeMove(Gyroscope);
		gyroscopeMoved = false;
	}

	Gyroscope.set(0.0f, 0.0f, 0.0f);
}

void CInput::TouchpadUpdate()
{
	if (Device.dwPrecacheFrame)
	{
		return;
	}

	for (u8 i = 0; i < 2; i++)
	{
		if (!TouchpadFingerPresent[i])
		{
			continue;
		}
		Fvector2 posDelta = TouchpadAbsPosition[i];
		posDelta.sub(TouchpadAbsPositionPrev[i]);
		if (!fsimilar(posDelta.magnitude(), EPS_L))
		{
			cbStack.back()->IR_OnTouchpadMove(posDelta);
		}
		TouchpadAbsPositionPrev[i] = TouchpadAbsPosition[i];
	}
}

#pragma warning(pop)

//-------------------------------------------------------
void CInput::iCapture(IInputReceiver *p)
{
	VERIFY(p);

	if (!Device.IsEditorMode() && CImGuiManager::Instance().IsCapturingInputs())
	{
		NoInputUpdate();
	} 
	else
	{
		MouseUpdate();
		GamepadUpdate();
		GyroscopeUpdate();
		TouchpadUpdate();
		KeyboardUpdate();
	}

    // change focus
	if (!cbStack.empty())
		cbStack.back()->IR_OnDeactivate();

	cbStack.push_back(p);
	cbStack.back()->IR_OnActivate();
}

void CInput::iRelease(IInputReceiver *p)
{
	if (p == cbStack.back())
	{
		cbStack.back()->IR_OnDeactivate();
		cbStack.pop_back();
		IInputReceiver * ir = cbStack.back();
		ir->IR_OnActivate();
	}else{// we are not topmost receiver, so remove the nearest one
		u32 cnt = (u32)cbStack.size();
		for(;cnt>0;--cnt)
			if( cbStack[cnt-1] == p ){
				xr_vector<IInputReceiver*>::iterator it = cbStack.begin();
				std::advance	(it,cnt-1);
				cbStack.erase	(it);
				break;
			}
	}
}

void CInput::OnAppActivate		(void)
{
	if (CurrentIR())
		CurrentIR()->IR_OnActivate();

	acquire();

	ZeroMemory		( mouseState,	sizeof(mouseState) );
	ZeroMemory		( KBState,		sizeof(KBState) );
}

void CInput::OnAppDeactivate	(void)
{
	if (CurrentIR())
		CurrentIR()->IR_OnDeactivate();

	unacquire();

	ZeroMemory		( mouseState,	sizeof(mouseState) );
	ZeroMemory		( KBState,		sizeof(KBState) );
}

void CInput::OnFrame()
{
	PROF_EVENT("CInput::OnFrame");
	CScopeTimer Input(RDEVICE.Statistic->Input);

	dwCurTime = RDEVICE.TimerAsync_MMT();
#if !defined(MASTER_GOLD)
	if (!Device.IsEditorMode() && CImGuiManager::Instance().IsCapturingInputs())
	{
		NoInputUpdate();
	} 
	else 
#endif
	{
		MouseUpdate();
		GamepadUpdate();
		GyroscopeUpdate();
		TouchpadUpdate();
		KeyboardUpdate();
	}
}

IInputReceiver* CInput::CurrentIR()
{
	if(cbStack.size())
		return cbStack.back();
	else
		return nullptr;
}


void CInput::unacquire()
{
	SDL_SetWindowRelativeMouseMode(g_AppInfo.Window, false);
	IsAcquire = false;
}

void CInput::acquire()
{
	IsAcquire = true;
	SDL_SetWindowRelativeMouseMode(g_AppInfo.Window, true);
}

void  CInput::feedback(u16 s1, u16 s2, float time)
{
	if (GetControllerMode())
	{
		GGamepadService->Rumble(s1, s2, time * 1000);
	}
}

void CInput::SetControllerMode(bool val)
{
	if (val == controllerMode)
		return;

	controllerMode = val;
	CStringTable::ReparseKeyBindings();
}

extern u32 ps_gamepad_prefix_override;
extern xr_token gamepad_prefix_override_token[];

void CInput::SelectGamepadPrefix()
{
	if (GGamepadService->GamePadDevice == nullptr)
	{
		return;
	}

	if (ps_gamepad_prefix_override != 0)
	{
		GamepadTypeName = gamepad_prefix_override_token[ps_gamepad_prefix_override].name;
		return;
	}
	
	GamepadTypeName = GGamepadService->GetGamepadPrefix();
}

bool CInput::LeftMouseButtonPressed()
{
	return iGetAsyncKeyState(MOUSE_1);
}

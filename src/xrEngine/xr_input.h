#ifndef __XR_INPUT__
#define __XR_INPUT__

enum
{
	UXR_GAMEPAD_A = SDL_SCANCODE_COUNT,
	UXR_GAMEPAD_B,
	UXR_GAMEPAD_X,
	UXR_GAMEPAD_Y,
	UXR_GAMEPAD_BACK,
	UXR_GAMEPAD_GUIDE,
	UXR_GAMEPAD_START,
	UXR_GAMEPAD_LS_CLICK,
	UXR_GAMEPAD_RS_CLICK,
	UXR_GAMEPAD_LB,
	UXR_GAMEPAD_RB,
	UXR_GAMEPAD_DPAD_UP,
	UXR_GAMEPAD_DPAD_DOWN,
	UXR_GAMEPAD_DPAD_LEFT,
	UXR_GAMEPAD_DPAD_RIGHT,
	UXR_GAMEPAD_MICROPHONE,
	UXR_GAMEPAD_RIGHT_PADDLE1,
	UXR_GAMEPAD_LEFT_PADDLE1,
	UXR_GAMEPAD_RIGHT_PADDLE2,
	UXR_GAMEPAD_LEFT_PADDLE2,
	UXR_GAMEPAD_TOUCHPAD,
	UXR_GAMEPAD_MISC2,
	UXR_GAMEPAD_MISC3,
	UXR_GAMEPAD_MISC4,
	UXR_GAMEPAD_MISC5,
	UXR_GAMEPAD_MISC6,

	UXR_GAMEPAD_LTRIGGER,
	UXR_GAMEPAD_RTRIGGER,

	UXR_GAMEPAD_LSTICK_UP,
	UXR_GAMEPAD_LSTICK_DOWN,
	UXR_GAMEPAD_LSTICK_LEFT,
	UXR_GAMEPAD_LSTICK_RIGHT,

	UXR_GAMEPAD_RSTICK_UP,
	UXR_GAMEPAD_RSTICK_DOWN,
	UXR_GAMEPAD_RSTICK_LEFT,
	UXR_GAMEPAD_RSTICK_RIGHT,

	UXR_BUTTONS_COUNT
};

class	ENGINE_API				IInputReceiver;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//описание класса
class ENGINE_API CInput
	:
	public pureFrame,
	public pureAppActivate,
	public pureAppDeactivate
{
public:
	enum 
	{
		COUNT_MOUSE_BUTTONS			= 8,
		COUNT_MOUSE_AXIS			= 3,
		COUNT_KB_BUTTONS			= UXR_BUTTONS_COUNT, // keyboard + gamepad
		COUNT_FINGERS_TOUCHPAD		= 2 // More than 2 fingers is not supported by DualShock 4 & DualSense
	};

	std::function<void(int)> xrgame_sdk_input_pressed;
	std::function<void(int)> xrgame_sdk_input_released;

private:
	bool						mouseMoved = false;
	bool						mouseScrolled = false;
	char						mouseState[COUNT_MOUSE_BUTTONS] = {};
	char						KBState[COUNT_KB_BUTTONS] = {};
	int 						offs[COUNT_MOUSE_AXIS] = {};
	char						old_mouseState[COUNT_MOUSE_BUTTONS] = {};
	char						old_KBState[COUNT_KB_BUTTONS] = {};
	bool						controllerMode = false;
	bool						touchpadMode = false;
	shared_str					GamepadTypeName = "xbox1";

	Fvector2					LeftAxis = { 0, 0 };
	Fvector2					RightAxis = { 0, 0 };

	Fvector2					AdaptiveTrigger = { 0, 0 };

	bool						gyroscopeMoved = false;
	Fvector3					Gyroscope = {0, 0, 0};

	Fvector2					TouchpadAbsPosition[COUNT_FINGERS_TOUCHPAD] = {0, 0};
	Fvector2					TouchpadAbsPositionPrev[COUNT_FINGERS_TOUCHPAD] = { 0, 0 };

	xr_vector<IInputReceiver*>	cbStack;

	void						NoInputUpdate				();
	void						MouseUpdate					();
	void						KeyboardUpdate				();
	void						GamepadUpdate				();
	void						GyroscopeUpdate				();
	void						TouchpadUpdate				();

public:
	u32							dwCurTime;
	
	void						MouseMotion					(float dx, float dy);
	void						MouseScroll					(float d);
	void						MousePressed				(int button);
	void						MouseReleased				(int button);
		
	void                        KeyboardButtonUpdate        (int scancode, bool IsPressed);
	
	void						LeftAxisUpdate				(bool IsX, float value);
	void						RightAxisUpdate				(bool IsX, float value);
	void						AdaptiveTriggerUpdate		(bool IsX, float value);
	void						GamepadGyroscopeUpdate		(Fvector3 value);


	void						iCapture					( IInputReceiver *pc );
	void						iRelease					( IInputReceiver *pc );
	bool						iGetAsyncKeyState			( int dik );
	bool						iGetAsyncBtnState			( int btn );

								CInput						( bool bExclusive = true);
	virtual						~CInput						( );

	virtual void	_BCL		OnFrame						(void);
	virtual void				OnAppActivate				(void);
	virtual void				OnAppDeactivate				(void);

	IInputReceiver*				CurrentIR					();
	bool						IsAcquire = false;
	bool						TouchpadFingerPresent[COUNT_FINGERS_TOUCHPAD];

	void						unacquire					();
	void						acquire						();
	bool						get_dik_name				(int dik, LPSTR dest, int dest_sz);

	void						feedback					(u16 s1, u16 s2, float time);

	void						SetControllerMode			(bool val);
	bool						GetControllerMode			() { return controllerMode; }
	void						SelectGamepadPrefix			();
	const char*					GamepadPrefix				() const { return GamepadTypeName.c_str(); }

	IC void						SetTouchpadMode				(bool val) { touchpadMode = val; }
	bool						GetTouchpadMode				() { return touchpadMode; }
	
	IC void						SetTouchpadAbsoulutePosPrev	(Fvector2 pos, u8 finger = 0) { TouchpadAbsPositionPrev[finger] = pos; }
	IC void						SetTouchpadAbsoulutePos		(Fvector2 pos, u8 finger = 0) { TouchpadAbsPosition[finger] = pos; }
	IC Fvector2					GetTouchpadAbsoulutePos		(u8 finger = 0) { return TouchpadAbsPosition[finger]; }

	bool						LeftMouseButtonPressed		();
};

extern ENGINE_API CInput *		pInput;

#endif //__XR_INPUT__

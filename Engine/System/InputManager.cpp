#include "Shared.h"
#include "InputManager.h"

#include <Engine/System/Window.h>

#include <direct.h>
#include <deque>
#include <algorithm>
#include <windows.h>

InputManager::InputManager()
	: activeMod( KEY_MOD_NONE )
{
	memset( &mouseInfos, 0, sizeof( mouseInfos ) );
}

InputManager::~InputManager()
{
	memset( &mouseInfos, 0, sizeof( mouseInfos ) );
}

const bool InputManager::Initialize()
{
	constexpr RAWINPUTDEVICE Rid[2] = {
		{
			0x01,
			0x02,
			0x00,
			0x00,
		},
		{
			0x01,
			0x06,
			0x00,
			0x00,
		},
	};

	POINT p = {};
	GetCursorPos( &p );

	mouseInfos.absolutePositionX = p.x;
	mouseInfos.absolutePositionY = p.y;

	return ( RegisterRawInputDevices( Rid, 2, sizeof( RAWINPUTDEVICE ) ) == TRUE );
}

void InputManager::Shutdown()
{
	memset( &mouseInfos, 0, sizeof( mouseInfos ) );

	eventList.clear();

	for ( int i = 0; i < KEY_MOD_COUNT; ++i ) {
		callbacks[i].clear();
	}
}

void InputManager::Flush()
{
	memset( &mouseInfos, 0, sizeof( mouseInfos ) );
	eventList.clear();
}

namespace
{
	keyModifier_t VirtualKeyToKeyModifier( const unsigned short vk, const unsigned short depressedKey, const unsigned short flags )
	{
		switch ( vk ) {
		case VK_CONTROL:
			return ( flags == 0x2 ) ? KEY_MOD_RCONTROL : KEY_MOD_LCONTROL;

		case VK_SHIFT:
			return ( depressedKey == 0x36 ) ? KEY_MOD_RSHIFT : KEY_MOD_LSHIFT;

		case VK_MENU:
			return ( flags == 0x2 ) ? KEY_MOD_RMENU : KEY_MOD_LMENU;

		default:
			return KEY_MOD_NONE;
		}
	}
}

void InputManager::Process( const HWND win, const WPARAM infos, const uint64_t timestamp )
{
	RAWINPUT rawInput	= {};
	UINT szData			= sizeof( RAWINPUT );

	GetRawInputData( reinterpret_cast<HRAWINPUT>( infos ), RID_INPUT, &rawInput, &szData, sizeof( RAWINPUTHEADER ) );

	if ( rawInput.header.dwType == RIM_TYPEMOUSE ) {
		mouseInfos.positionX = rawInput.data.mouse.lLastX;
		mouseInfos.positionY = rawInput.data.mouse.lLastY;

		POINT p = {};
		GetCursorPos( &p );
		ScreenToClient( win, &p );

		mouseInfos.absolutePositionX = p.x;
		mouseInfos.absolutePositionY = p.y;

		if ( rawInput.data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN || rawInput.data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_UP ) {
			mouseInfos.leftButton = !mouseInfos.leftButton;
		}

		if ( rawInput.data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN || rawInput.data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_UP ) {
			mouseInfos.rightButton = !mouseInfos.rightButton;
		}

		if ( rawInput.data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN || rawInput.data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_UP ) {
			mouseInfos.middleButton = !mouseInfos.middleButton;
		}

		mouseInfos.wheelPosition = ( RI_MOUSE_WHEEL & rawInput.data.mouse.usButtonFlags ) ? static_cast<short>( rawInput.data.mouse.usButtonData ) : 0;
	} else if ( rawInput.header.dwType == RIM_TYPEKEYBOARD ) {
		const keyModifier_t keyModifier = VirtualKeyToKeyModifier( rawInput.data.keyboard.VKey, rawInput.data.keyboard.MakeCode, rawInput.data.keyboard.Flags );

		if ( keyModifier != KEY_MOD_NONE ) {
			activeMod = ( rawInput.data.keyboard.Message != WM_KEYUP ) ? keyModifier : KEY_MOD_NONE;
		} else {
			eventList.push_back( {
				KEY_COMBINAISON(
					rawInput.data.keyboard.VKey,
					static_cast<unsigned short>( ( rawInput.data.keyboard.Message == WM_KEYDOWN ) ? 1 : 0 ),
					0,
					0
				),
				timestamp
			} );
		}
	}
}

void InputManager::Acknowledge( const window_t* win )
{
	mouseInfos.positionX = mouseInfos.positionY = 0;

	POINT screenPoint = {
		static_cast< LONG >( win->width / 2 ),
		static_cast< LONG >( win->height / 2 ),
	};

	ClientToScreen( win->handle, &screenPoint );
	SetCursorPos( screenPoint.x, screenPoint.y );
}

void InputManager::Poll( const float dt )
{
	static std::pair<bool, uint64_t> keyboard[KEY_MOD_COUNT][KEY_COUNT] = { std::make_pair( false, 0 ) };
	keyboard[activeMod][VK_NONE] = std::make_pair( true, 0 );

	std::deque<std::pair<uint64_t, inputCallback>> callbackList = {};

	// update keyboard according to the timestamp
	for ( const keyEvent_t& e : eventList ) {
		const int keyCode	= ( ( e.keyCodeAndState >> 24 ) & 0xFF );
		const int keyState	= ( ( e.keyCodeAndState >> 16 ) & 0xFF );

		keyboard[activeMod][keyCode] = std::make_pair( keyState == 1 ? true : false, e.timestamp );
	}

	for ( auto it = callbacks[activeMod].begin(); it != callbacks[activeMod].end(); ++it ) {
		if ( keyboard[activeMod][KEYCOMB_GETKEY( it->first )].first ) {
			callbackList.push_back( { keyboard[activeMod][( it->first >> 24 ) & 0xFF].second, it->second } );
		}
	}

	std::sort( callbackList.begin(), callbackList.end(), 
		[]( const std::pair<uint64_t, inputCallback>& t1, const std::pair<uint64_t, inputCallback>& t2 ) { return t1.first > t2.first; } );

	for ( const std::pair<uint64_t, inputCallback>& k : callbackList ) {
		k.second( dt );
	}
}

#pragma once

struct window_t;

#include <map>
#include <functional>
#include <vector>

static constexpr int KEY_COUNT = 256;

enum keyModifier_t
{
	KEY_MOD_NONE,

	KEY_MOD_LCONTROL,
	KEY_MOD_RCONTROL,

	KEY_MOD_LSHIFT,
	KEY_MOD_RSHIFT,

	KEY_MOD_LMENU,
	KEY_MOD_RMENU,

	KEY_MOD_COUNT,
};

using inputCallback = std::function<void( float )>;
using callbackList	= std::map<unsigned int, inputCallback>[KEY_MOD_COUNT];

// key: VK value
// isDelay: true if the key combinaison can only be triggered once every 60 frames
// z,w: unused
#define KEY_COMBINAISON( key, isDelayed, z, w )( unsigned int )( ( key << 24ll ) | ( isDelayed << 16ll ) | ( z << 8ll ) | ( ( w ) ) )

#define KEYCOMB_GETKEY( keyCombo ) ( ( keyCombo >> 24 ) & 0xFF )
#define KEYCOMB_GETISDELAYED( keyCombo ) ( ( keyCombo >> 16 ) & 0xFF ) 

#define VK_NONE 0x0

#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39

#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5A

class InputManager
{
public:
	struct
	{
		long positionX;
		long positionY;

		long absolutePositionX;
		long absolutePositionY;

		short wheelPosition;

		bool leftButton;
		bool rightButton;
		bool middleButton;
	} mouseInfos;

public:
	void RegisterCallback( const unsigned int key, const bool isDelayed, const keyModifier_t modifier, const inputCallback& callback )
	{
		callbacks[modifier].insert( std::make_pair( KEY_COMBINAISON( key, isDelayed, 0x0, 0x0 ), callback ) );
	}

public:
							InputManager();
							InputManager( InputManager& ) = delete;
							~InputManager();

	const bool				Initialize();
	void					Shutdown();
	void					Flush();

	void					Process( const HWND win, const WPARAM infos, const uint64_t timestamp );
	void					Acknowledge( const window_t* win );
	void					Poll( const float dt );

private:
	struct keyEvent_t
	{
		unsigned int		keyCodeAndState;
		uint64_t			timestamp;
	};

private:
	std::vector<keyEvent_t> eventList;
	keyModifier_t			activeMod;
	callbackList			callbacks;	// TODO: per-state callback list
};

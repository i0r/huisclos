#pragma once

enum winDispMode_t
{
	WIN_DISP_MODE_WINDOWED		= 0x01,
	WIN_DISP_MODE_FULLSCREEN	= 0x02,
	WIN_DISP_MODE_BORDERLESS	= 0x04,
};

enum winFlag_t
{
	WIN_FLAG_HAS_FOCUS			= 0x08
};

struct window_t
{
	HINSTANCE			instance;	// 8 
	HWND				handle;		// 8
	LPCSTR				className;	// 8
	unsigned int		flags;		// 4
	unsigned short		width;		// 2
	unsigned short		height;		// 2
};

const int	Sys_CreateWindow( window_t* win );
void		Sys_DestroyWindow( window_t* win );

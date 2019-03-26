#include "Shared.h"
#include "Window.h"

#include "InputManager.h"

#include <windows.h>
#include <Engine/ThirdParty/imgui/imgui.h>
#include <Engine/ThirdParty/imgui/examples/directx11_example/imgui_impl_dx11.h>

LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	window_t* win = ( window_t* )( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );

	if ( win == nullptr ) {
		return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}

	switch ( uMsg ) {
	case WM_DESTROY:
		DestroyWindow( hwnd );
		return 0;

	case WM_CLOSE:
		PostQuitMessage( 0 );
		return 0;

	case WM_SETCURSOR: {
		static bool isMouseVisible = false;
		WORD ht = LOWORD( lParam );

		if ( HTCLIENT == ht && isMouseVisible
			|| HTCLIENT != ht && !isMouseVisible ) {
			isMouseVisible = !isMouseVisible;
			ShowCursor( isMouseVisible );
		}
	} return 0;

	case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpMinMaxInfo = reinterpret_cast<LPMINMAXINFO>( lParam );

		lpMinMaxInfo->ptMinTrackSize.x = static_cast<LONG>( win->width / 4.0f );
		lpMinMaxInfo->ptMinTrackSize.y = static_cast<LONG>( win->height / 4.0f );
	} return 0;

	case WM_ACTIVATE:
	case WM_SETFOCUS:
		win->flags |= WIN_FLAG_HAS_FOCUS;
		return 0;

	case WA_INACTIVE:
	case WM_KILLFOCUS:	
		win->flags &= ~WIN_FLAG_HAS_FOCUS;
		return 0;

	default:
		return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
}

const int Sys_CreateWindow( window_t* win )
{
	win->instance = GetModuleHandle( NULL );

	const WNDCLASSEX wc = {
		sizeof( WNDCLASSEX ),									// cbSize
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,						// style
		::WndProc,												// lpfnWndProc
		NULL,													// cbClsExtra
		NULL,													// cbWndExtra
		win->instance,											// hInstance
		NULL,													// hIcon
		LoadCursor( 0, IDC_ARROW ),								// hCursor
		static_cast<HBRUSH>( GetStockObject( BLACK_BRUSH ) ),	// hbrBackground
		NULL,													// lpszMenuName
		win->className,											// lpszClassName
		NULL 													// hIconSm
	};
	
	if ( RegisterClassEx( &wc ) == FALSE ) {
		return 1;
	}

	DWORD	windowExFlags	= WS_EX_APPWINDOW,
			windowFlags		= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	int screenWidth		= GetSystemMetrics( SM_CXSCREEN ),
		screenHeight	= GetSystemMetrics( SM_CYSCREEN );

	if ( win->flags & WIN_DISP_MODE_FULLSCREEN || win->flags & WIN_DISP_MODE_BORDERLESS ) {
		DEVMODE devMode = {};
		devMode.dmSize			= sizeof( devMode );
		devMode.dmPelsWidth		= win->width;
		devMode.dmPelsHeight	= win->height;
		devMode.dmBitsPerPel	= 32;
		devMode.dmFields		= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( win->flags & WIN_DISP_MODE_BORDERLESS ) {
			win->width	= screenWidth;
			win->height = screenHeight;
		} else if ( win->width != screenWidth && win->height != screenHeight && ChangeDisplaySettings( &devMode, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL ) {
			return 2;
		}

		windowFlags |= WS_POPUP;
	} else {
		windowExFlags	|= WS_EX_WINDOWEDGE;
		windowFlags		|= ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_BORDER | WS_VISIBLE );
	}

	win->handle = CreateWindowEx( windowExFlags, win->className, win->className, windowFlags, 
								( screenWidth - win->width ) / 2, ( screenHeight - win->height ) / 2, 
								win->width, win->height, NULL, NULL, win->instance, NULL );

	if ( win->handle == nullptr ) {
		return 3;
	}

	ShowCursor( FALSE );

	ShowWindow( win->handle, SW_SHOWNORMAL );

	UpdateWindow( win->handle );

	SetForegroundWindow( win->handle );
	SetFocus( win->handle );

	SetWindowLongPtr( win->handle, GWLP_USERDATA, ( LONG_PTR )win );

	win->flags |= WIN_FLAG_HAS_FOCUS;

	RECT windowRect = {};
	GetClientRect( win->handle, &windowRect );

	win->width	= static_cast<unsigned short>( windowRect.right - windowRect.left );
	win->height = static_cast<unsigned short>( windowRect.bottom - windowRect.top );

	return 0;
}

void Sys_DestroyWindow( window_t* win )
{
	ShowCursor( TRUE );

	UnregisterClass( win->className, win->instance );

	DeleteObject( win->instance );
	win->instance = nullptr;

	DestroyWindow( win->handle );
	win->handle = nullptr;

	memset( win, 0, sizeof( window_t ) );
}

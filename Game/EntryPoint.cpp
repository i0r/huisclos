#include <Engine/Shared.h>	

#include <Engine/System/Window.h>
#include <Engine/System/InputManager.h>
#include <Engine/Graphics/RenderManager.h>
#include <Engine/System/Timer.h>

extern LRESULT ImGui_ImplDX11_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

#include <timeapi.h>

int WINAPI CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	const HCURSOR defaultCursor = ::SetCursor( LoadCursor( 0, IDC_WAIT ) );

	timeBeginPeriod( 1 ); // set timer def. to 1ms

	SetProcessWorkingSetSize( GetCurrentProcess(), 256 << 20, 2048 << 20 );
	SetErrorMode( SEM_FAILCRITICALERRORS );

	InputManager inputMan = {};
	RenderManager renderMan = {};
	window_t window =
	{
		nullptr,					// HINSTANCE		instance
		nullptr,					// HWND				handle
		"Ten Hours",				// LPCSTR			className
		WIN_DISP_MODE_BORDERLESS,	// unsigned int		flags
		1280,						// unsigned short	width
		720,						// unsigned short	height
	};

	if ( Sys_CreateWindow( &window ) != 0 ) {
		return 1;
	}

	if ( !inputMan.Initialize() ) {
		return 2;
	}

	if ( renderMan.Initialize( &window ) != 0 ) {
		return 3;
	}

#ifdef _DEBUG
	// TEMPORARY; TO REMOVE LATER
	inputMan.RegisterCallback( KEY_COMBINAISON( VK_Z, VK_NONE, VK_NONE, VK_NONE ),
		std::bind( &Camera::MoveForward, &renderMan.freeCam, std::placeholders::_1 ) );

	inputMan.RegisterCallback( KEY_COMBINAISON( VK_S, VK_NONE, VK_NONE, VK_NONE ),
		std::bind( &Camera::MoveBackward, &renderMan.freeCam, std::placeholders::_1 ) );

	inputMan.RegisterCallback( KEY_COMBINAISON( VK_Q, VK_NONE, VK_NONE, VK_NONE ),
		std::bind( &Camera::MoveLeft, &renderMan.freeCam, std::placeholders::_1 ) );

	inputMan.RegisterCallback( KEY_COMBINAISON( VK_D, VK_NONE, VK_NONE, VK_NONE ),
		std::bind( &Camera::MoveRight, &renderMan.freeCam, std::placeholders::_1 ) );

	inputMan.RegisterCallback( KEY_COMBINAISON( VK_ESCAPE, VK_NONE, VK_NONE, VK_NONE ),
		std::bind( [&]( const float dt ) { PostThreadMessage( GetCurrentThreadId(), WM_QUIT, 0, 0 ); }, std::placeholders::_1 ) );
#endif

	::SetCursor( defaultCursor ); // restore default cursor

	MSG msg = {};

	timer_t gameLogicTimer = {};
	Timer_Start( &gameLogicTimer );

	constexpr double ref_dt = 10.0;	// 10ms / 100FPS cap (for now)
	double dt = 0.00;
	double accumulator = 0.00;

	while ( 1 ) {
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
			ImGui_ImplDX11_WndProcHandler( msg.hwnd, msg.message, msg.wParam, msg.lParam );

			if ( msg.message == WM_QUIT ) {
				inputMan.Shutdown();
				renderMan.Shutdown();
				Sys_DestroyWindow( &window );
				return 0;
			} else if ( msg.message == WM_INPUT ) {
				inputMan.Process( msg.hwnd, msg.lParam, msg.time );
				continue;
			}

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		if ( window.flags & WIN_FLAG_HAS_FOCUS ) {
			dt = Timer_GetDelta( &gameLogicTimer ); // get delta in ms

			if ( dt > 250.0 ) {
				dt = 250.0;
			}

			accumulator += dt;

			inputMan.Poll( static_cast<float>( dt ) );
			renderMan.freeCam.Update( inputMan.mouseInfos.positionX, inputMan.mouseInfos.positionY, static_cast<float>( dt ) );
			inputMan.ResetRelativeMouse( &window );

			while ( accumulator >= ref_dt ) {
				/* active state -> Update(); */
				accumulator -= dt;
			}

			const double alpha = accumulator / dt;

	//		renderMan.Frame( static_cast< float >( alpha + ( 1.0 - alpha ) ) );
			renderMan.Swap();
		}
	}

	return -1;
}

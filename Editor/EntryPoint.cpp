#include <Engine/Shared.h>

#include <Engine/System/Window.h>
#include <Engine/System/InputManager.h>
#include <Engine/Graphics/RenderManager.h>
#include <Engine/System/Timer.h>
#include <Engine/Game/World.h>

#include <Engine/Graphics/Mesh.h>

#include <timeapi.h>
#include <OleIdl.h>

// Editor specifics
#include "Graphics/UI/UIManager.h"
#include "Game/WorldEditor.h"
#include "Graphics/Surfaces/Icon.h"
#include "System/DragDropWatchdog.h"

#include <Editor/ThirdParty/ImGuizmo/ImGuizmo.h>

int WINAPI CALLBACK WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	HCURSOR defaultCursor = ::SetCursor( LoadCursor( 0, IDC_WAIT ) );

	timeBeginPeriod( 1 ); // set timer def. to 1ms

	SetProcessWorkingSetSize( GetCurrentProcess(), 256 << 20, 2048 << 20 );
	SetErrorMode( SEM_FAILCRITICALERRORS );

	UIManager		uiMan = {};
	InputManager	inputMan = {};
	RenderManager	renderMan = {};
	World			worldMan = {};
	WorldEditor		worldEdMan = {};

	window_t window =
	{
		nullptr,					// HINSTANCE		instance
		nullptr,					// HWND				handle
		"HuisClosEditor",			// LPCSTR			className
		WIN_DISP_MODE_WINDOWED,		// unsigned int		flags
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

	worldMan.CreateEmptyArea();
	worldEdMan.Initialize( &uiMan, renderMan.GetContext(), &window, &inputMan, renderMan.GetMaterialManager() );
	worldEdMan.SetActiveWorld( &worldMan );

	inputMan.RegisterCallback( VK_Z, false, KEY_MOD_NONE, std::bind( &Camera::MoveForward, worldEdMan.GetActiveCameraObj(), std::placeholders::_1 ) );
	inputMan.RegisterCallback( VK_S, false, KEY_MOD_NONE, std::bind( &Camera::MoveBackward, worldEdMan.GetActiveCameraObj( ), std::placeholders::_1 ) );
	inputMan.RegisterCallback( VK_Q, false, KEY_MOD_NONE, std::bind( &Camera::MoveLeft, worldEdMan.GetActiveCameraObj(), std::placeholders::_1 ) );
	inputMan.RegisterCallback( VK_D, false, KEY_MOD_NONE, std::bind( &Camera::MoveRight, worldEdMan.GetActiveCameraObj(), std::placeholders::_1 ) );
	inputMan.RegisterCallback( VK_W, true, KEY_MOD_NONE, std::bind( &UIManager::SetTranslationMode, &uiMan ) );

	inputMan.RegisterCallback( VK_X, true, KEY_MOD_NONE, std::bind( &UIManager::SetRotationMode, &uiMan ) );
	inputMan.RegisterCallback( VK_C, true, KEY_MOD_NONE, std::bind( &UIManager::SetScaleMode, &uiMan ) );
	inputMan.RegisterCallback( VK_F, true, KEY_MOD_NONE, std::bind( &WorldEditor::FocusOn, &worldEdMan ) );
	inputMan.RegisterCallback( VK_L, true, KEY_MOD_NONE, std::bind( &WorldEditor::ToggleFocusOn, &worldEdMan ) );
    inputMan.RegisterCallback( VK_DELETE, true, KEY_MOD_NONE, std::bind( &WorldEditor::RemoveSelectedNode, &worldEdMan ) );

	inputMan.RegisterCallback( VK_C, true, KEY_MOD_LCONTROL, std::bind( &WorldEditor::CopyNode, &worldEdMan ) );
	inputMan.RegisterCallback( VK_V, true, KEY_MOD_LCONTROL, std::bind( &WorldEditor::PasteNode, &worldEdMan ) );

	inputMan.RegisterCallback( VK_ESCAPE, true, KEY_MOD_NONE, std::bind( [&]( const float dt ) { PostThreadMessage( GetCurrentThreadId(), WM_QUIT, 0, 0 ); }, std::placeholders::_1 ) );
	inputMan.RegisterCallback( VK_F1, true, KEY_MOD_NONE, std::bind( &UIManager::Toggle, &uiMan ) );
	inputMan.RegisterCallback( VK_F2, true, KEY_MOD_NONE, std::bind( &RenderManager::SwapEnvMap, &renderMan ) );

	uiMan.Initialize( renderMan.GetContext(), renderMan.GetTextureManager(), &window );
    
    DragDropWatchdog dragDropWatchdog = {};
    OleInitialize( nullptr );
    
    if ( FAILED( RegisterDragDrop( window.handle, &dragDropWatchdog ) ) ) {
        return 5;
    }

    dragDropWatchdog.SetDropCallback( std::bind( &WorldEditor::MeshInsertCallback, &worldEdMan, std::placeholders::_1 ) );

    ShowCursor( TRUE );

	::SetCursor( defaultCursor ); // restore default cursor

	MSG msg = {};

	timer_t gameLogicTimer = {};
	Timer_Start( &gameLogicTimer );

	constexpr double ref_dt = 10.0;	// 10ms / 100FPS cap (for now)

	double dt			= 0.00;
	double accumulator	= 0.00;

	while ( 1 ) {
		while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
			if ( uiMan.IsToggled() ) {
				if ( ImGui_ImplDX11_WndProcHandler( msg.hwnd, msg.message, msg.wParam, msg.lParam ) && uiMan.IsInputingText() && msg.message == WM_CHAR ) 
					continue;
			}
	
			if ( msg.message == WM_QUIT ) {
				inputMan.Shutdown();
				renderMan.Shutdown();
				Sys_DestroyWindow( &window );
				return 0;
			} else if ( msg.message == WM_INPUT && !uiMan.IsInputingText() ) {
				inputMan.Process( msg.hwnd, msg.lParam, msg.time );
			} else if ( msg.message == WM_SIZE ) {
				uiMan.Resize();
			}

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		if ( window.flags & WIN_FLAG_HAS_FOCUS ) {
			dt = Timer_GetDelta( &gameLogicTimer ); // get delta as ms

			if ( dt > 250.0 ) {
				dt = 250.0;
			}

			accumulator += dt;

			inputMan.Poll( static_cast<float>( dt ) );
				
			while ( accumulator >= ref_dt ) {
				worldEdMan.Frame( static_cast<float>( dt ) );
				accumulator -= dt;
			}
			
			const float	alpha					= static_cast< float >( accumulator / dt ),
						interpolatedFrametime	= alpha + ( 1.0f - alpha );

			renderMan.FrameWorld( interpolatedFrametime, worldEdMan.GetActiveCameraObj(), worldMan.GetActiveArea() );

			if ( uiMan.IsToggled() ) {
                uiMan.Draw( interpolatedFrametime, worldEdMan.GetActiveCamera() );
            }

			renderMan.Swap();
		} else {
			inputMan.Flush(); // TODO: flush this only once...
		}
	}

	return -1;
}

#include <Engine/Shared.h>
#include <Engine/ThirdParty/imgui/imgui.h>
#include <Engine/ThirdParty/imgui/examples/directx11_example/imgui_impl_dx11.h>

#include "UIManager.h"

#include "Clock.h"
#include "ColorPicker.h"
#include "TransformationFeature.h"
#include "LightingFeatures.h"

#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/RenderContext.h>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Game/World.h>
#include <Engine/System/Window.h>
#include <Engine/Graphics/LightManager.h>
#include <Engine/Io/AreaFileReaderWriter.h>

#include <string>

UIManager::UIManager()
	: activeNode( nullptr )
	, activeManipulationMode( 0 )
	, isToggled( true )
	, isInputingText( false )
	, sceneHiearchyWinToggled( true )
    , debugOverlayToggled( true )
    , todClockToggled( false )
	, activeColorMode( 0 )
{

}

void UIManager::Initialize( const renderContext_t* context, TextureManager* texMan, const window_t* win )
{
	ImGui_ImplDX11_Init( win->handle, context->device, context->deviceContext );

	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 5, 5 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 5, 5 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 12, 8 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_GrabMinSize, 5.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemInnerSpacing, ImVec2( 8, 6 ) );

    iconSurf.Create( context, texMan );

    renderContext = context;
}

void UIManager::Draw( const float dt, const Camera* cam )
{
    for ( auto& iconToRender : iconsToRender ) {
        iconSurf.Render( renderContext, iconToRender.second, iconToRender.first );
    }
    iconsToRender.clear();

	ImGui_ImplDX11_NewFrame();

    DrawEdPanel( dt, cam );

    if ( debugOverlayToggled ) {
        DrawDebugOverlay( dt, cam );
    }

    if ( sceneHiearchyWinToggled  ) {
        DrawSceneHiearchy();
    }

	ImGui::Render();
}

void UIManager::Resize()
{
	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_CreateDeviceObjects();
}

const bool UIManager::IsManipulating() const
{
	return ( ImGuizmo::IsUsing() || ImGuizmo::IsOver() );
}

void UIManager::DrawEdPanel( const float dt, const Camera* cam )
{
    ImVec2 winSize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
    {
        if ( !ImGui::Begin( "huisclos_toolbar1", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings ) ) {
            ImGui::End();
            return;
        }

        ImGui::SetWindowSize( ImVec2( winSize.x / 3.5f, winSize.y / 2.0f ) );

        DrawMenuBar( cam );

        if ( activeNode != nullptr && activeNode->content != nullptr ) {
            ImGuizmo::BeginFrame();
            ImGuizmo::Enable( true );

			if ( ImGui::InputText( "Name", activeNode->name, 128, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue )  ) {
				isInputingText = !isInputingText;
			}

			if ( ImGui::IsItemClicked() ) isInputingText = true;

            if ( activeNode->flags & NODE_FLAG_CONTENT_MESH ) {
				ImGui::TextColored( ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ), "Type: Mesh" );
				ImGui::Separator();

                const mesh_t* activeMesh = static_cast< mesh_t* >( activeNode->content );

				{
					DirectX::XMFLOAT4X4 edModel = {};
					DirectX::XMStoreFloat4x4( &edModel, activeMesh->transformation->modelMatrix );

					Ed_PanelTransformation( cam, activeManipulationMode, edModel, ( float* )&activeMesh->transformation->translation, ( float* )&activeMesh->transformation->rotation, ( float* )&activeMesh->transformation->scale );
					
					activeMesh->transformation->modelMatrix = DirectX::XMLoadFloat4x4( &edModel );
					activeMesh->transformation->boundingSphere.Transform( activeMesh->transformation->boundingSphere, DirectX::XMMatrixTranspose( activeMesh->transformation->modelMatrix ) );
				}
            } else if ( activeNode->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ) {
                ImGui::TextColored( ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ), "Type: Sphere Light" );
				ImGui::Separator();

                sphereAreaLight_t* sphereLight = static_cast< sphereAreaLight_t* >( activeNode->content );

				{
					DirectX::XMFLOAT4X4 edModel = {};
					DirectX::XMStoreFloat4x4( &edModel, 
						DirectX::XMMatrixTranslation( 
							sphereLight->worldPositionRadius.x, 
							sphereLight->worldPositionRadius.y, 
							sphereLight->worldPositionRadius.z 
						) 
					);

					Ed_PanelTransformation( cam, activeManipulationMode, edModel, ( float* )&sphereLight->worldPositionRadius );
				}

                ImGui::SliderFloat( "Radius", &sphereLight->worldPositionRadius.w, 0.001f, 16.0f );

				{
					Ed_PanelLuminousPower( &sphereLight->color.w );
					Ed_PanelColor( activeColorMode, ( float* )&sphereLight->color );
				}       
			} else if ( activeNode->flags & NODE_FLAG_CONTENT_DISK_LIGHT ) {
				ImGui::TextColored( ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ), "Type: Disk Light" );
				ImGui::Separator();

				diskAreaLight_t* diskLight = static_cast< diskAreaLight_t* >( activeNode->content );

				{
					DirectX::XMFLOAT4X4 edModel = {};
					DirectX::XMStoreFloat4x4( &edModel, 
						DirectX::XMMatrixTranslation( 
							diskLight->worldPositionRadius.x, 
							diskLight->worldPositionRadius.y, 
							diskLight->worldPositionRadius.z 
						) 
					);

					Ed_PanelTransformation( cam, activeManipulationMode, edModel, ( float* )&diskLight->worldPositionRadius );
				}

				ImGui::DragFloat( "Radius", &diskLight->worldPositionRadius.w, 0.001f, 16.0f );
				ImGui::InputFloat3( "Plane Normal", ( float* )&diskLight->planeNormal, 3 );

				{
					Ed_PanelLuminousPower( &diskLight->color.w );
					Ed_PanelColor( activeColorMode, ( float* )&diskLight->color );
				}
			} else if ( activeNode->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
				ImGui::TextColored( ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ), "Type: Sun Light" );
				ImGui::Separator();

				sunLight_t* sunLight = static_cast< sunLight_t* >( activeNode->content );

				ImGui::SliderFloat2( "Theta & Gamma", (float*)&sunLight->sphericalThetaGammaAndPADDING, -1.00f, 1.0f );

				{
					Ed_PanelLuminousPower( &sunLight->colorAndIntensityLux.w );
					Ed_PanelColor( activeColorMode, ( float* )&sunLight->colorAndIntensityLux );
				}
			} else if ( activeNode->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
				ImGui::TextColored( ImVec4( 0.9f, 0.9f, 0.9f, 1.0f ), "Type: Rectangle Light" );
				ImGui::Separator();

				rectangleAreaLight_t* rectLight = static_cast< rectangleAreaLight_t* >( activeNode->content );

				{
					DirectX::XMFLOAT4X4 edModel = {};
					DirectX::XMStoreFloat4x4( &edModel, 
						DirectX::XMMatrixTranslation( 
							rectLight->worldPositionRadius.x, 
							rectLight->worldPositionRadius.y, 
							rectLight->worldPositionRadius.z 
						)
					);

					Ed_PanelTransformation( cam, activeManipulationMode, edModel, ( float* )&rectLight->worldPositionRadius );
				}

				ImGui::InputFloat3( "Plane Normal", ( float* )&rectLight->planeNormal, 3 );
				ImGui::InputFloat3( "Up", ( float* )&rectLight->up, 3 );
				ImGui::InputFloat3( "Left", ( float* )&rectLight->left, 3 );

				if ( ImGui::Button( "Auto Compute Left and Up" ) ) {
					constexpr DirectX::XMFLOAT3 rDir = { 0.0f, 1.0f, 0.0f };
					constexpr DirectX::XMFLOAT3 lDir = { 0.0f, -1.0f, 0.0f };

					DirectX::XMFLOAT3 planeDir3 = { rectLight->planeNormal.x, rectLight->planeNormal.y, rectLight->planeNormal.z };
					DirectX::XMVECTOR rightVec	= DirectX::XMVector3Cross( DirectX::XMLoadFloat3( &planeDir3 ), DirectX::XMLoadFloat3( &rDir ) );
					DirectX::XMVECTOR leftVec	= DirectX::XMVector3Cross( DirectX::XMLoadFloat3( &planeDir3 ), DirectX::XMLoadFloat3( &lDir ) );
					DirectX::XMVECTOR top		= DirectX::XMVector3Cross( rightVec, DirectX::XMLoadFloat3( &planeDir3 ) );

					top		= DirectX::XMVector3Normalize( top );
					leftVec = DirectX::XMVector3Normalize( leftVec );

					DirectX::XMStoreFloat3( ( DirectX::XMFLOAT3* )&rectLight->up, top );
					DirectX::XMStoreFloat3( ( DirectX::XMFLOAT3* )&rectLight->left, leftVec );
				}

				ImGui::SliderFloat( "Width", &rectLight->widthHeight.x, 0.001f, 16.0f );
				ImGui::SliderFloat( "Height", &rectLight->widthHeight.y, 0.001f, 16.0f );

				{
					Ed_PanelLuminousPower( &rectLight->color.w );
					Ed_PanelColor( activeColorMode, ( float* )&rectLight->color );
				}
            }
        }

        ImGui::End();
    }
}

void UIManager::DrawDebugOverlay( const float dt, const Camera* cam )
{
    ImVec2 winSize = ImGui::GetIO().DisplaySize;
    float* worldPos = cam->GetPosition();

    std::string worldPosStr = "WorldPos: " + std::to_string( worldPos[0] ) + ", " + std::to_string( worldPos[1] ) + ", " + std::to_string( worldPos[2] ),              
                fpsStr = std::to_string( ( int )winSize.x ) + "x" + std::to_string( ( int )winSize.y ) + " | " + std::to_string( ( int )ImGui::GetIO().Framerate ) + " FPS | " + std::to_string( 1000.0f / ImGui::GetIO().Framerate ) + " ms";

    const ImVec2 fpsCSize = ImGui::CalcTextSize( fpsStr.c_str() );
    const ImVec2 posCSize = ImGui::CalcTextSize( worldPosStr.c_str() );

    ImGui::SetNextWindowPos( ImVec2( winSize.x - ( posCSize.x + 15.0f ), 0 ) );

    if ( !ImGui::Begin( "debug_overlay", nullptr, winSize, 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings ) ) {
        ImGui::End();
        return;
    }

    ImGui::SetWindowSize( winSize );

    if ( todClockToggled ) {
        Ed_RenderToDClock( 0 );
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32 col32 = ImColor( ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
    draw_list->AddText( ImVec2( winSize.x - ( fpsCSize.x + 10.0f ), 5 ), col32, fpsStr.c_str() );
    draw_list->AddText( ImVec2( winSize.x - ( posCSize.x + 10.0f ), 20 ), col32, worldPosStr.c_str() );

    ImGui::End();
}

void UIManager::PrintNode( areaNode_t* parentNode, areaNode_t* previousNode, areaNode_t* toInsertNext )
{
	for ( int i = 0; i < parentNode->children.size(); ++i ) {
		if ( toInsertNext != nullptr ) {
			parentNode->children.push_back( toInsertNext );
			toInsertNext = nullptr;
		}

		bool hasChildren = parentNode->children.at( i )->children.size() > 0;

		ImGuiTreeNodeFlags flags = ( !hasChildren ) ? ImGuiTreeNodeFlags_Leaf : 0;
		if ( parentNode->children.at( i ) == activeNode ) flags |= ImGuiTreeNodeFlags_Selected; // TODO: 100% CERTIFIED QUALITY CODE (to remove)

		if ( ImGui::TreeNodeEx( parentNode->children.at( i )->name, flags ) ) {
			if ( ImGui::IsItemHoveredRect() && ImGui::IsMouseClicked( 0 ) ) {
				activeNode = parentNode->children.at( i );
			}

			previousNode = parentNode->children.at( i );

			if ( hasChildren ) {
				PrintNode( parentNode->children.at( i ), previousNode, toInsertNext );
			}

			ImGui::TreePop();
		}
	}
}

void UIManager::DrawSceneHiearchy()
{
    ImVec2 winSize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowPos( ImVec2( 0.0f, winSize.y / 2.0f ) );

    if ( !ImGui::Begin( "huisclos_toolbar2", &sceneHiearchyWinToggled, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) ) {
        ImGui::End();
        return;
    }

    ImGui::SetWindowSize( ImVec2( winSize.x / 3.5f, winSize.y / 2.0f ) );

    if ( ImGui::TreeNodeEx( "Scene Hiearchy", ImGuiTreeNodeFlags_CollapsingHeader ) ) {
		areaNode_t* previousNode = nullptr, *toInsertNode = nullptr;

		PrintNode( activeWorld->GetActiveArea()->nodes, previousNode, toInsertNode );
        ImGui::TreePop();
    }
 
    ImGui::End();
}

void UIManager::DrawMenuBar( const Camera* cam )
{
    if ( ImGui::BeginMenuBar() ) {
        if ( ImGui::BeginMenu( "Edit" ) ) {
            ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Entity" ) ) {
            const float* worldPos = cam->GetPosition();
            const float* eyeDir = cam->GetEyeDirection();

            if ( ImGui::BeginMenu( "Lights" ) ) {
				if ( ImGui::MenuItem( "Sun Light" ) ) {
					sunLight_t* sunLight = new sunLight_t();
					sunLight->worldPositionRadius = { worldPos[0] + eyeDir[0] * 2.0f, worldPos[1] + eyeDir[1] * 2.0f, worldPos[2] + eyeDir[2] * 2.0f, 1.0f };

					sunLight->colorAndIntensityLux = { 1.0f, 1.0f, 1.0f, 59800.0f };
					sunLight->sphericalThetaGammaAndPADDING = { 1.0f, 0.50f, 1.0f, 1.0f };

					areaNode_t* node = activeWorld->InsertNode( sunLight, NODE_FLAG_CONTENT_SUN_LIGHT );
					if ( node != nullptr ) strcpy( node->name, "Sun Light" );
					SetNodeEdit( nullptr ); // avoid dirty object pointer
				}

                if ( ImGui::MenuItem( "Sphere Light" ) ) {
                    sphereAreaLight_t* areaLight = new sphereAreaLight_t();
                    areaLight->worldPositionRadius  = { worldPos[0] + eyeDir[0] * 2.0f, worldPos[1] + eyeDir[1] * 2.0f, worldPos[2] + eyeDir[2] * 2.0f, 1.0f };
                    areaLight->color                = { 1.0f, 0.87f, 0.70f, 75.0f };

					areaNode_t* node = activeWorld->InsertNode( areaLight, NODE_FLAG_CONTENT_SPHERE_LIGHT );
					if ( node != nullptr ) strcpy( node->name, "Sphere Light" );

                    SetNodeEdit( nullptr ); // avoid dirty object pointer
                }

                if ( ImGui::MenuItem( "Disk Light" ) ) {
                    diskAreaLight_t* areaLight = new diskAreaLight_t();
                    areaLight->worldPositionRadius = { worldPos[0] + eyeDir[0] * 2.0f, worldPos[1] + eyeDir[1] * 2.0f, worldPos[2] + eyeDir[2] * 2.0f, 1.0f };
                    areaLight->planeNormal = { eyeDir[0], eyeDir[1], eyeDir[2], 1.0f };
                    areaLight->color = { 1.0f, 0.87f, 0.70f, 75.0f };

					areaNode_t* node = activeWorld->InsertNode( areaLight, NODE_FLAG_CONTENT_DISK_LIGHT );
					if ( node != nullptr ) strcpy( node->name, "Disk Light" );

                    SetNodeEdit( nullptr ); // avoid dirty object pointer
                }

				if ( ImGui::MenuItem( "Rectangle Light" ) ) {
					rectangleAreaLight_t* areaLight = new rectangleAreaLight_t();
					areaLight->worldPositionRadius = { worldPos[0] + eyeDir[0] * 2.0f, worldPos[1] + eyeDir[1] * 2.0f, worldPos[2] + eyeDir[2] * 2.0f, 1.0f };
					areaLight->planeNormal = { eyeDir[0], eyeDir[1], eyeDir[2], 1.0f };
					areaLight->color = { 1.0f, 0.87f, 0.70f, 75.0f };
					areaLight->widthHeight = { 4.0f, 4.0f, 0.0f, 0.0f };

					DirectX::XMFLOAT3 planeDir3 = { eyeDir[0], eyeDir[1], eyeDir[2] };
					DirectX::XMFLOAT3 rDir = { 0.0f, 1.0f, 0.0f };
					DirectX::XMFLOAT3 lDir = { 0.0f, -1.0f, 0.0f };

					DirectX::XMVECTOR rightVec = DirectX::XMVector3Cross( DirectX::XMLoadFloat3( &planeDir3 ), DirectX::XMLoadFloat3( &rDir ) );
					DirectX::XMVECTOR leftVec = DirectX::XMVector3Cross( DirectX::XMLoadFloat3( &planeDir3 ), DirectX::XMLoadFloat3( &lDir ) );
					DirectX::XMVECTOR top = DirectX::XMVector3Cross( rightVec, DirectX::XMLoadFloat3( &planeDir3 ) );

					top = DirectX::XMVector3Normalize( top );
					leftVec = DirectX::XMVector3Normalize( leftVec );

					DirectX::XMStoreFloat3( ( DirectX::XMFLOAT3* )&areaLight->up, top );
					DirectX::XMStoreFloat3( ( DirectX::XMFLOAT3* )&areaLight->left, leftVec );

					areaNode_t* node = activeWorld->InsertNode( areaLight, NODE_FLAG_CONTENT_RECTANGLE_LIGHT );
					if ( node != nullptr ) strcpy( node->name, "Rectangle Light" );

					SetNodeEdit( nullptr ); // avoid dirty object pointer
				}

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Tools" ) ) {
            if ( ImGui::MenuItem( ( sceneHiearchyWinToggled ) ? "Hide scene hiearchy" : "Show scene hiearchy" ) ) {
                sceneHiearchyWinToggled = !sceneHiearchyWinToggled;
            }

            ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "World" ) ) {
			if ( ImGui::MenuItem( "Save Current Area" ) ) {
				Io_WriteAreaFile( "test.area", activeWorld->GetActiveArea() );
			}
            ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "System" ) ) {
            ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Debug" ) ) {
            if ( ImGui::MenuItem( "Hide editor UI", "F1" ) ) {
                isToggled = !isToggled;
            }

            if ( ImGui::MenuItem( ( debugOverlayToggled ) ? "Hide debug overlay" : "Show debug overlay" ) ) {
                debugOverlayToggled = !debugOverlayToggled;
            }

            if ( ImGui::MenuItem( ( todClockToggled ) ? "Hide time of day" : "Show time of day" ) ) {
                todClockToggled = !todClockToggled;
            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

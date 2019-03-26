#include <Engine/Game/World.h>
#include "WorldEditor.h"

#include <Engine/Graphics/RenderContext.h>
#include <Engine/System/Window.h>
#include <Engine/System/InputManager.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/LightManager.h>

#include <Editor/Graphics/UI/UIManager.h>

WorldEditor::WorldEditor()
	: uiMan( nullptr )
	, activeWorld( nullptr )
    , renderContext( nullptr )
    , window( nullptr )
    , inputMan( nullptr )
    , selectedNode( nullptr )
	, isFocusing( false )
    , freeCam{}
{

}

const int WorldEditor::Initialize( UIManager* edUI, const renderContext_t* context, const window_t* win, InputManager* inputManager, MaterialManager* materialManager )
{
    if ( edUI == nullptr || inputManager == nullptr || win == nullptr || context == nullptr ) {
        return 1;
    }

	uiMan           = edUI;
	inputMan        = inputManager;
	window          = win;
	renderContext   = context;
    matMan          = materialManager;

    const float aspectRatio = static_cast<float>( win->width ) / static_cast<float>( win->height );

	if ( !freeCam.Create( context, aspectRatio, DirectX::XMConvertToRadians( 75.0f ), 0.01f, 1000.0f ) ) {
		return 2;
	}

	freeCam.SetPosition( 0.0f, 4.0f, 3.0f );
	freeCam.RotateDegX( 180.0f );
	freeCam.SetActive( context );

	return 0;
}

void WorldEditor::SetActiveWorld( World* world )
{
	activeWorld = world;

    uiMan->SetActiveWorld( world );
}

void WorldEditor::Frame( const float dt )
{
	if ( inputMan->mouseInfos.rightButton ) {
		freeCam.Update( inputMan->mouseInfos.positionX, inputMan->mouseInfos.positionY, dt );
		inputMan->Acknowledge( window );
	} else {
		freeCam.Update( 0, 0, dt );
	}

    const long edPanelWidth = static_cast<long>( window->width / 3.50f );
	if ( inputMan->mouseInfos.leftButton
		&& inputMan->mouseInfos.absolutePositionX > edPanelWidth
		&& !uiMan->IsManipulating() ) {
		SelectNodeByMouse( &freeCam );
	}

	if ( inputMan->mouseInfos.wheelPosition != 0 ) {
		if ( inputMan->mouseInfos.wheelPosition < 0 ) {
			freeCam.MoveBackward( dt );
		} else {
			freeCam.MoveForward( dt );
		}

		inputMan->mouseInfos.wheelPosition = 0; // todo: make the wheel position reset implicit?
	}

	if ( isFocusing ) {
        FocusOn();
    }

    for ( areaNode_t* child : activeWorld->GetActiveArea()->nodes->children ) {
        if ( child->content != nullptr ) {
            if ( child->flags & NODE_FLAG_CONTENT_DISK_LIGHT ) {
                const diskAreaLight_t* diskLight = static_cast< diskAreaLight_t* >( child->content );          
                uiMan->AddIconToRenderList( { diskLight->worldPositionRadius }, ED_ICON_DISK_LIGHT );
            } else if ( child->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ) {
                const sphereAreaLight_t* sphereLight = static_cast< sphereAreaLight_t* >( child->content );
                uiMan->AddIconToRenderList( { sphereLight->worldPositionRadius }, ED_ICON_SPHERE_LIGHT );
            } else if ( child->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
				const rectangleAreaLight_t* rectLight = static_cast< rectangleAreaLight_t* >( child->content );
				uiMan->AddIconToRenderList( { rectLight->worldPositionRadius }, ED_ICON_ERROR );
			} else if ( child->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
				const sunLight_t* sunLight = static_cast< sunLight_t* >( child->content );
				uiMan->AddIconToRenderList( { sunLight->worldPositionRadius }, ED_ICON_SUN_LIGHT );
			} else {
                continue;
            }   
        }
    }

}

void WorldEditor::SelectNodeByMouse( const Camera* cam )
{
	D3D11_VIEWPORT viewport = {};
	UINT numOfVP = 1;

    renderContext->deviceContext->RSGetViewports( &numOfVP, &viewport );

	POINT cursorPos = {};

	GetCursorPos( &cursorPos );
	ScreenToClient( window->handle, &cursorPos );

	DirectX::XMMATRIX* projMat = ( DirectX::XMMATRIX* )cam->GetProjectionMatrix();
	DirectX::XMMATRIX* viewMat = ( DirectX::XMMATRIX* )cam->GetViewMatrix();
	DirectX::XMMATRIX invViewMat = DirectX::XMMatrixInverse( nullptr, *viewMat );

	DirectX::XMFLOAT4X4 projMatVec = {};
	DirectX::XMFLOAT4X4 invViewMatVec = {};

	DirectX::XMStoreFloat4x4( &projMatVec, *projMat );
	DirectX::XMStoreFloat4x4( &invViewMatVec, invViewMat );

	DirectX::XMFLOAT3 ray =
	{
		( ( ( 2.0f * cursorPos.x ) / viewport.Width ) - 1 ) / projMatVec._11,
		-( ( ( 2.0f * cursorPos.y ) / viewport.Height ) - 1 ) / projMatVec._22,
		1.0f
	};

	DirectX::XMFLOAT3 rayOrigin =
	{
		invViewMatVec._41,
		invViewMatVec._42,
		invViewMatVec._43
	};

	DirectX::XMFLOAT3 rayDirection =
	{
		ray.x * invViewMatVec._11 + ray.y * invViewMatVec._21 + ray.z * invViewMatVec._31,
		ray.x * invViewMatVec._12 + ray.y * invViewMatVec._22 + ray.z * invViewMatVec._32,
		ray.x * invViewMatVec._13 + ray.y * invViewMatVec._23 + ray.z * invViewMatVec._33
	};

	DirectX::XMVECTOR rayOriginVec = DirectX::XMLoadFloat3( &rayOrigin );
	DirectX::XMVECTOR rayDirectionVec = DirectX::XMLoadFloat3( &rayDirection );
	
	#undef min
	#undef max

	float intersectionDist		= 0.0f;
	float closesIntersection	= std::numeric_limits<float>::max();
	int	  closestNodeIndex		= -1;
	int	  currentNodeIndex		= 0;

	// holy crap that's a lot of indirection!
	for ( areaNode_t* child : activeWorld->GetActiveArea()->nodes->children ) {
		if ( child->content != nullptr ) {
			if ( child->flags & NODE_FLAG_CONTENT_MESH ) {
				mesh_t* mesh = static_cast< mesh_t* >( child->content );

				DirectX::XMMATRIX invModelMat = DirectX::XMMatrixInverse( nullptr, mesh->transformation->modelMatrix );

				DirectX::XMVECTOR rayObjOrigin = DirectX::XMVector3TransformCoord( rayOriginVec, invModelMat );
				DirectX::XMVECTOR rayObjDirection = DirectX::XMVector3TransformNormal( rayDirectionVec, invModelMat );

				rayObjDirection = DirectX::XMVector3Normalize( rayObjDirection );

				if ( mesh->transformation->boundingSphere.Intersects( rayObjOrigin, rayObjDirection, intersectionDist ) ) {
					if ( intersectionDist < closesIntersection ) {
						closesIntersection = intersectionDist;
						closestNodeIndex = currentNodeIndex;
					}
				}
			} else if ( child->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ||
						child->flags & NODE_FLAG_CONTENT_DISK_LIGHT  ||
						child->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
				sphereAreaLight_t* light = static_cast< sphereAreaLight_t* >( child->content );

				DirectX::BoundingSphere lightProxy = { DirectX::XMFLOAT3( light->worldPositionRadius.x, light->worldPositionRadius.y, light->worldPositionRadius.z ), light->worldPositionRadius.w };

				DirectX::XMMATRIX invModelMat = DirectX::XMMatrixInverse( nullptr, DirectX::XMMatrixIdentity() );

				DirectX::XMVECTOR rayObjOrigin = DirectX::XMVector3TransformCoord( rayOriginVec, invModelMat );
				DirectX::XMVECTOR rayObjDirection = DirectX::XMVector3TransformNormal( rayDirectionVec, invModelMat );

				rayObjDirection = DirectX::XMVector3Normalize( rayObjDirection );

				if ( lightProxy.Intersects( rayObjOrigin, rayObjDirection, intersectionDist ) ) {
					if ( intersectionDist < closesIntersection ) {
						closesIntersection = intersectionDist;
						closestNodeIndex = currentNodeIndex;
					}
				}
			} else if ( child->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
				rectangleAreaLight_t* light = static_cast< rectangleAreaLight_t* >( child->content );

				DirectX::BoundingBox lightProxy = { 
					DirectX::XMFLOAT3( light->worldPositionRadius.x, light->worldPositionRadius.y, light->worldPositionRadius.z ), 
					DirectX::XMFLOAT3( light->widthHeight.x, light->widthHeight.y, light->widthHeight.x )
				};

				DirectX::XMMATRIX invModelMat = DirectX::XMMatrixInverse( nullptr, DirectX::XMMatrixIdentity() );

				DirectX::XMVECTOR rayObjOrigin = DirectX::XMVector3TransformCoord( rayOriginVec, invModelMat );
				DirectX::XMVECTOR rayObjDirection = DirectX::XMVector3TransformNormal( rayDirectionVec, invModelMat );

				rayObjDirection = DirectX::XMVector3Normalize( rayObjDirection );

				if ( lightProxy.Intersects( rayObjOrigin, rayObjDirection, intersectionDist ) ) {
					if ( intersectionDist < closesIntersection ) {
						closesIntersection = intersectionDist;
						closestNodeIndex = currentNodeIndex;
					}
				}
			}
		}

		currentNodeIndex++;
	}

	selectedNode = ( closestNodeIndex == -1 ) ? nullptr : activeWorld->GetActiveArea()->nodes->children.at( closestNodeIndex );

    uiMan->SetNodeEdit( selectedNode );
}

void WorldEditor::FocusOn()
{
	if ( selectedNode == nullptr ) {
        ToggleFocusOn(); // untoggle focus incase we lose our selected node
		return;
	}

	// focusing should only be available for mesh... I guess???
	if ( selectedNode->flags & NODE_FLAG_CONTENT_MESH ) {
		const mesh_t* meshContent = static_cast< mesh_t* >( selectedNode->content );

		freeCam.LookAt( meshContent->transformation->translation );
	}
}

void WorldEditor::RemoveSelectedNode()
{
    if ( selectedNode == nullptr ) {
        return;
    }

    if ( selectedNode->flags & NODE_FLAG_CONTENT_MESH ) {
        Render_ReleaseMesh( static_cast<mesh_t*>( selectedNode->content ) );
    } else if ( selectedNode->flags & NODE_FLAG_CONTENT_DISK_LIGHT ) {
        delete static_cast<diskAreaLight_t*>( selectedNode->content );
    } else if ( selectedNode->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ) {
        delete static_cast<sphereAreaLight_t*>( selectedNode->content );
    } else if ( selectedNode->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
		delete static_cast<sunLight_t*>( selectedNode->content );
	} else if ( selectedNode->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
		delete static_cast<rectangleAreaLight_t*>( selectedNode->content );
	}

    activeWorld->RemoveNode( selectedNode->hash );

    selectedNode = nullptr;
}

void WorldEditor::MeshInsertCallback( char* absolutePath )
{
    mesh_t* insertedMesh = new mesh_t();

    if ( Render_CreateMeshFromFile( renderContext, matMan, insertedMesh, absolutePath ) != 0 ) {
        delete insertedMesh;
        return;
    }

    areaNode_t* insertedNode = activeWorld->InsertNode( insertedMesh, NODE_FLAG_CONTENT_MESH );
	
	if ( insertedNode != nullptr ) {
		std::string pathStr( absolutePath );
		strcpy( insertedNode->name, pathStr.substr( pathStr.find_last_of( "/\\" ) + 1 ).c_str() );
	}

    uiMan->SetNodeEdit( nullptr ); // avoid dirty object pointer
	inputMan->mouseInfos.leftButton = false;
}

void WorldEditor::PasteNode()
{
	if ( copyNode == nullptr ) {
		return;
	}

	areaNode_t* copiedNode = new areaNode_t( *copyNode );

	if ( copiedNode->flags & NODE_FLAG_CONTENT_MESH ) {
		copiedNode->content = new mesh_t( *static_cast< mesh_t* >( copiedNode->content ) );
	} else if ( copiedNode->flags & NODE_FLAG_CONTENT_DISK_LIGHT ) {
		copiedNode->content = new diskAreaLight_t( *static_cast< diskAreaLight_t* >( copiedNode->content ) );
	} else if ( copiedNode->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ) {
		copiedNode->content = new sphereAreaLight_t( *static_cast< sphereAreaLight_t* >( copiedNode->content ) );
	} else if ( copiedNode->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
		copiedNode->content = new sunLight_t( *static_cast< sunLight_t* >( copiedNode->content ) );
	} else if ( copiedNode->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
		copiedNode->content = new rectangleAreaLight_t( *static_cast< rectangleAreaLight_t* >( copiedNode->content ) );
	}
	activeWorld->GetActiveArea()->nodes->children.push_back( copiedNode );
}

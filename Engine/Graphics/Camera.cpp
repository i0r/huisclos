#include "Shared.h"
#include "Camera.h"
#include "RenderContext.h"

Camera::Camera()
	: worldPos{}
	, right{}
	, eye{}
	, pitch( 0.0f )
	, yaw( 0.0f )
	, roll( 0.0f )
	, matrices{}
	, cbuffer( nullptr )
{

}

Camera::~Camera()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( cbuffer )
}

static constexpr float FREECAM_MOVESPEED_THRESHOLD = 100.0f;

FreeCamera::FreeCamera()
{
	moveSpeed			= 0.00025f;
	unitVelocity		= moveSpeed * 0.25f;

	memset( &isMoving, 0, sizeof( bool ) * DIRECTION_COUNT );
	memset( &velocity, 0, sizeof( float ) * DIRECTION_COUNT );
}

FreeCamera::FreeCamera( FreeCamera& cam )
{
	worldPos	= cam.worldPos;
	right		= cam.right;
	eye			= cam.eye;

	pitch = yaw = roll = 0.0f;

	matrices	= cam.matrices;
	cbuffer		= cam.cbuffer;

	memset( &isMoving, 0, sizeof( bool ) * DIRECTION_COUNT );
	memset( &velocity, 0, sizeof( float ) * DIRECTION_COUNT );
}

FreeCamera::~FreeCamera()
{
	memset( &isMoving, 0, sizeof( bool ) * DIRECTION_COUNT );
	memset( &velocity, 0, sizeof( float ) * DIRECTION_COUNT );
}

const bool FreeCamera::Create( const renderContext_t* context, const float aspectRatio, const float fov, const float nearPlane, const float farPlane )
{
	if ( !Render_CreateCBuffer( context, cbuffer, sizeof( camCbuffer_t ) ) ) {
		return false;
	}

	SetProjection( aspectRatio, fov, nearPlane, farPlane );

	return true;
}

void FreeCamera::Update( long mouseDx, long mouseDy, float frameTime )
{
	pitch	-= ( static_cast<float>( mouseDy ) * moveSpeed * frameTime );
	yaw		+= ( static_cast<float>( mouseDx ) * moveSpeed * frameTime );

	if ( velocity[DIRECTION_FWRD] > 0.0f && !isMoving[DIRECTION_FWRD] ) {
		velocity[DIRECTION_FWRD] -= frameTime * unitVelocity;

		if ( velocity[DIRECTION_FWRD] < 0.0f ) velocity[DIRECTION_FWRD] = 0.0f;

		worldPos = DirectX::XMFLOAT3( worldPos.x + eye.x * frameTime * velocity[DIRECTION_FWRD],
									  worldPos.y + eye.y * frameTime * velocity[DIRECTION_FWRD],
									  worldPos.z + eye.z * frameTime * velocity[DIRECTION_FWRD] );
	}

	if ( velocity[DIRECTION_BCKWD] > 0.0f && !isMoving[DIRECTION_BCKWD] ) {
		velocity[DIRECTION_BCKWD] -= frameTime * unitVelocity;

		if ( velocity[DIRECTION_BCKWD] < 0.0f ) velocity[DIRECTION_BCKWD] = 0.0f;

		worldPos = DirectX::XMFLOAT3( worldPos.x - eye.x * frameTime * velocity[DIRECTION_BCKWD],
									  worldPos.y - eye.y * frameTime * velocity[DIRECTION_BCKWD],
									  worldPos.z - eye.z * frameTime * velocity[DIRECTION_BCKWD] );
	}

	if ( velocity[DIRECTION_LEFT] > 0.0f && !isMoving[DIRECTION_LEFT]) {
		velocity[DIRECTION_LEFT] -= frameTime * unitVelocity;

		if ( velocity[DIRECTION_LEFT] < 0.0f ) velocity[DIRECTION_LEFT] = 0.0f;

		worldPos = DirectX::XMFLOAT3( worldPos.x + right.x * frameTime * velocity[DIRECTION_LEFT],
								  	  worldPos.y + right.y * frameTime * velocity[DIRECTION_LEFT],
									  worldPos.z + right.z * frameTime * velocity[DIRECTION_LEFT] );
	}

	if ( velocity[DIRECTION_RIGHT] > 0.0f && !isMoving[DIRECTION_RIGHT] ) {
		velocity[DIRECTION_RIGHT] -= frameTime * unitVelocity;

		if ( velocity[DIRECTION_RIGHT] < 0.0f ) velocity[DIRECTION_RIGHT] = 0.0f;

		worldPos = DirectX::XMFLOAT3( worldPos.x - right.x * frameTime * velocity[DIRECTION_RIGHT],
									  worldPos.y - right.y * frameTime * velocity[DIRECTION_RIGHT],
									  worldPos.z - right.z * frameTime * velocity[DIRECTION_RIGHT] );
	}

	if ( yaw < -DirectX::XM_PI ) {
		yaw += DirectX::XM_2PI;
	} else if ( yaw > DirectX::XM_PI ) {
		yaw -= DirectX::XM_2PI;
	}

	if ( pitch < -DirectX::XM_PIDIV2 ) {
		pitch = -DirectX::XM_PIDIV2;
	} else if ( pitch > DirectX::XM_PIDIV2 ) {
		pitch = DirectX::XM_PIDIV2;
	}

	eye.x = cosf( pitch ) * sinf( yaw );
	eye.y = sinf( pitch );
	eye.z = cosf( pitch ) * cosf( yaw );

	DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3( &eye );
	eyeVec = DirectX::XMVector3Normalize( eyeVec );

	const float yawResult = yaw - DirectX::XM_PIDIV2;

	right.x = sinf( yawResult + roll );
	right.y = 0.0f;
	right.z = cosf( yawResult + roll );

	DirectX::XMVECTOR rightVec = DirectX::XMLoadFloat3( &right );
	rightVec = DirectX::XMVector3Normalize( rightVec );

	DirectX::XMVECTOR top = DirectX::XMVector3Cross( rightVec, eyeVec );
	top = DirectX::XMVector3Normalize( top );

	const DirectX::XMVECTOR positionVector	= DirectX::XMLoadFloat3( &worldPos );
	const DirectX::XMVECTOR lookAtVector	= DirectX::XMVectorAdd( positionVector, eyeVec );

	viewMatrix = DirectX::XMMatrixLookAtLH( positionVector, lookAtVector, top );

	memset( &isMoving, false, sizeof( bool ) * DIRECTION_COUNT );
}

void FreeCamera::SetActive( const renderContext_t* context )
{
	context->deviceContext->VSSetConstantBuffers( 0, 1, &cbuffer );
	context->deviceContext->PSSetConstantBuffers( 0, 1, &cbuffer );

	UpdateMatrices( context );
}

void FreeCamera::SetProjection( const float aspectRatio, const float fov, const float nearPlane, const float farPlane )
{
	projectionMatrix = DirectX::XMMatrixPerspectiveFovLH( fov, aspectRatio, nearPlane, farPlane );
}

void FreeCamera::MoveForward( float frameTime )
{
	velocity[DIRECTION_FWRD] += moveSpeed * frameTime;

	if ( velocity[DIRECTION_FWRD] > ( frameTime * FREECAM_MOVESPEED_THRESHOLD ) ) {
		velocity[DIRECTION_FWRD] = frameTime * FREECAM_MOVESPEED_THRESHOLD;
	}

	worldPos = DirectX::XMFLOAT3( worldPos.x + eye.x * frameTime * velocity[DIRECTION_FWRD],
								  worldPos.y + eye.y * frameTime * velocity[DIRECTION_FWRD],
								  worldPos.z + eye.z * frameTime * velocity[DIRECTION_FWRD] );

	isMoving[DIRECTION_FWRD] = true;
}

void FreeCamera::MoveBackward( float frameTime )
{
	velocity[DIRECTION_BCKWD] += moveSpeed * frameTime;

	if ( velocity[DIRECTION_BCKWD] > ( frameTime * FREECAM_MOVESPEED_THRESHOLD ) ) {
		velocity[DIRECTION_BCKWD] = frameTime * FREECAM_MOVESPEED_THRESHOLD;
	}

	worldPos = DirectX::XMFLOAT3( worldPos.x - eye.x * frameTime * velocity[DIRECTION_BCKWD],
								  worldPos.y - eye.y * frameTime * velocity[DIRECTION_BCKWD],
								  worldPos.z - eye.z * frameTime * velocity[DIRECTION_BCKWD] );

	isMoving[DIRECTION_BCKWD] = true;
}

void FreeCamera::MoveLeft( float frameTime )
{
	velocity[DIRECTION_LEFT] += moveSpeed * frameTime;

	if ( velocity[DIRECTION_LEFT] > ( frameTime * FREECAM_MOVESPEED_THRESHOLD ) ) {
		velocity[DIRECTION_LEFT] = frameTime * FREECAM_MOVESPEED_THRESHOLD;
	}

	worldPos = DirectX::XMFLOAT3( worldPos.x + right.x * frameTime * velocity[DIRECTION_LEFT],
								  worldPos.y + right.y * frameTime * velocity[DIRECTION_LEFT],
							      worldPos.z + right.z * frameTime * velocity[DIRECTION_LEFT] );

	isMoving[DIRECTION_LEFT] = true;
}

void FreeCamera::MoveRight( float frameTime )
{
	velocity[DIRECTION_RIGHT] += moveSpeed * frameTime;

	if ( velocity[DIRECTION_RIGHT] > ( frameTime * FREECAM_MOVESPEED_THRESHOLD ) ) {
		velocity[DIRECTION_RIGHT] = frameTime * FREECAM_MOVESPEED_THRESHOLD;
	}

	worldPos = DirectX::XMFLOAT3( worldPos.x - right.x * frameTime * velocity[DIRECTION_RIGHT],
								  worldPos.y - right.y * frameTime * velocity[DIRECTION_RIGHT],
								  worldPos.z - right.z * frameTime * velocity[DIRECTION_RIGHT] );

	isMoving[DIRECTION_RIGHT] = true;
}

void FreeCamera::UpdateMatrices( const renderContext_t* context )
{
	matrices.viewProjection = viewMatrix * projectionMatrix;
	matrices.inverseView = DirectX::XMMatrixInverse( nullptr, viewMatrix );
	matrices.inverseProjection = DirectX::XMMatrixInverse( nullptr, projectionMatrix );
	
	matrices.position = DirectX::XMLoadFloat3( &worldPos );

	Render_UploadCBuffer( context, cbuffer, &matrices, sizeof( camCbuffer_t ) );
}

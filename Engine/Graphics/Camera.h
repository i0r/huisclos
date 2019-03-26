#pragma once

#include <d3d11.h>
#include "CBuffer.h"
#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>

enum direction_t
{
	DIRECTION_FWRD,
	DIRECTION_BCKWD,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,

	DIRECTION_COUNT,
};

struct camCbuffer_t
{
	DirectX::XMVECTOR position;
	DirectX::XMMATRIX viewProjection;
	DirectX::XMMATRIX inverseProjection;
	DirectX::XMMATRIX inverseView;
};

class Camera
{
public:
	float*		 GetPosition() const										{ return ( float* )&worldPos; }
	void		 SetPosition( const float x, const float y, const float z ) { worldPos = DirectX::XMFLOAT3( x, y, z ); }
	void		 ResetPosition()											{ worldPos = DirectX::XMFLOAT3(); }

	void		 RotateDegX( const float x )								{ yaw += DirectX::XMConvertToRadians( x ); }

	const float* GetViewMatrix() const										{ return ( float* )&viewMatrix; }
	const float* GetProjectionMatrix() const								{ return ( float* )&projectionMatrix; }

	const float* GetEyeDirection() const									{ return ( float* )&eye; }

public:
					Camera();
	virtual			~Camera();

	virtual const bool	Create( const renderContext_t* context, const float aspectRatio, const float fov, const float nearPlane, const float farPlane ) = 0;
	virtual void	Update( long mouseDx, long mouseDy, float frameTime ) = 0;

	virtual void	SetActive( const renderContext_t* context ) = 0;
	virtual void	SetProjection( const float aspectRatio, const float fov, const float nearPlane, const float farPlane ) = 0;
	virtual void	UpdateMatrices( const renderContext_t* context ) = 0;

	virtual void	MoveForward( float frameTime ) = 0;
	virtual void	MoveBackward( float frameTime ) = 0;
	virtual void	MoveLeft( float frameTime ) = 0;
	virtual void	MoveRight( float frameTime ) = 0;

	void LookAt( const DirectX::XMFLOAT3& targetWorldPos )
	{
		eye = DirectX::XMFLOAT3( ( targetWorldPos.x - worldPos.x ), ( targetWorldPos.y - worldPos.y ), ( targetWorldPos.z - worldPos.z ) );

		DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3( &eye );
		eyeVec = DirectX::XMVector3Normalize( eyeVec );
		DirectX::XMStoreFloat3( &eye, eyeVec );

		pitch = asin( -eye.y );
		yaw = atan2( eye.x, eye.z );
		roll = 0.0f;

		const float yawResult = yaw - DirectX::XM_PIDIV2;

		right.x = sinf( yawResult + roll );
		right.y = 0.0f;
		right.z = cosf( yawResult + roll );

		DirectX::XMVECTOR rightVec = DirectX::XMLoadFloat3( &right );
		rightVec = DirectX::XMVector3Normalize( rightVec );

		DirectX::XMVECTOR top = DirectX::XMVector3Cross( rightVec, eyeVec );
		top = DirectX::XMVector3Normalize( top );

		DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat3( &worldPos );
		const DirectX::XMVECTOR lookAtVector = DirectX::XMVectorAdd( positionVector, eyeVec );

		viewMatrix = DirectX::XMMatrixLookAtLH( positionVector, lookAtVector, top );
	}

protected:
	DirectX::XMFLOAT3	worldPos;
	DirectX::XMFLOAT3	right;
	DirectX::XMFLOAT3	eye;

	DirectX::XMMATRIX	viewMatrix;
	DirectX::XMMATRIX	projectionMatrix;

	float				pitch;
	float				yaw;
	float				roll;

	camCbuffer_t		matrices;
	CBuffer				cbuffer;
};

class FreeCamera : public Camera
{
public:
	inline void			SetVelocity( const float velocityVal )	{ unitVelocity = velocityVal; }
	inline void			SetVelocityFactor( const float factor ) { unitVelocity = moveSpeed * factor; } // set velocity as a factor of the camera move speed
	inline void			SetMoveSpeed( const float speed )		{ moveSpeed = speed; }

public:
						FreeCamera();
						FreeCamera( FreeCamera& cam );
						~FreeCamera();

	const bool			Create( const renderContext_t* context, const float aspectRatio, const float fov, const float nearPlane, const float farPlane );
	void				Update( long mouseDx, long mouseDy, float frameTime );

	void				SetActive( const renderContext_t* context );
	void				SetProjection( const float aspectRatio, const float fov, const float nearPlane, const float farPlane );
	void				UpdateMatrices( const renderContext_t* context );

	void				MoveForward( float frameTime );
	void				MoveBackward( float frameTime );
	void				MoveLeft( float frameTime );
	void				MoveRight( float frameTime );

private:
	float				velocity[DIRECTION_COUNT];
	bool				isMoving[DIRECTION_COUNT];

	float				moveSpeed;
	float				unitVelocity;
};
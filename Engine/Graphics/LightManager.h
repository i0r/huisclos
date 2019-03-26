#pragma once

struct renderContext_t;

#include "CBuffer.h"
#include "RenderContext.h"
#include "Texture.h"
#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>

struct sphereAreaLight_t
{
	DirectX::XMFLOAT4 worldPositionRadius;
	DirectX::XMFLOAT4 color;
};

struct diskAreaLight_t
{
	DirectX::XMFLOAT4 worldPositionRadius;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 planeNormal;
};

struct rectangleAreaLight_t
{
	DirectX::XMFLOAT4 worldPositionRadius;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT4 planeNormal;
	DirectX::XMFLOAT4 up;
	DirectX::XMFLOAT4 left;
	DirectX::XMFLOAT4 widthHeight;
};

struct pointLight_t
{
	DirectX::XMFLOAT4 worldPositionRadius;
	DirectX::XMFLOAT4 color;
};

struct sunLight_t // CPU struct required to generate GPU data
{
	DirectX::XMFLOAT4	worldPositionRadius;
	DirectX::XMFLOAT4	colorAndIntensityLux;
	DirectX::XMFLOAT4	sphericalThetaGammaAndPADDING;

	renderTarget_t		cascadeAtlas;
};

struct worldArea_t;
struct areaNode_t;

class LightManager
{
public:
			LightManager()					= default;
			LightManager( LightManager& )	= delete;
			~LightManager()					= default;

	bool	Initialize( const renderContext_t* context );
	void	Update( const renderContext_t* context, const worldArea_t* activeArea, const bool isNight ); // only called on world update; no need to update it per frame!

private:
	struct LightCBuffer_t
	{
		DirectX::XMFLOAT4	sunColorAndAngularRadius;
		DirectX::XMFLOAT4	sunDirectionAndIlluminanceInLux;

		DirectX::XMINT4		lightTypeCount; // sphere, disk, rect

		sphereAreaLight_t	sphereAreaLights[12];
		diskAreaLight_t		diskAreaLights[12];
		rectangleAreaLight_t rectAreaLights[12];
	} lightManData_t;

	CBuffer cbuffer;

private:
	void	SetSun( const sunLight_t& sunInfos );
	void	IterateScene( areaNode_t* curNode );
};

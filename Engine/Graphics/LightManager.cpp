#include "Shared.h"
#include "LightManager.h"
#include "RenderContext.h"

#include <Engine/Game/World.h>

bool LightManager::Initialize( const renderContext_t* context )
{
	Render_CreateCBuffer( context, cbuffer, sizeof( lightManData_t ) );
	Render_UploadCBuffer( context, cbuffer, &lightManData_t, sizeof( lightManData_t ) );

	context->deviceContext->PSSetConstantBuffers( 3, 1, &cbuffer );

	return true;
}

void LightManager::Update( const renderContext_t* context, const worldArea_t* activeArea, const bool isNight )
{
	memset( &lightManData_t, 0, sizeof( lightManData_t ) ); // TODO: flushing the cbuffer each time isnt a great idea... a partital rebuild would be nice in the future

	IterateScene( activeArea->nodes );

	lightManData_t.lightTypeCount.w = ( isNight ) ? 1.0f : 3.0f;

	// update cbuffer data
	Render_UploadCBuffer( context, cbuffer, &lightManData_t, sizeof( lightManData_t ) );
}

void LightManager::IterateScene( areaNode_t* curNode )
{
	for ( areaNode_t* node : curNode->children ) {
		if ( node->flags & NODE_FLAG_CONTENT_DISK_LIGHT ) {
			lightManData_t.diskAreaLights[lightManData_t.lightTypeCount.y++] = *static_cast<diskAreaLight_t*>( node->content );
		} else if ( node->flags & NODE_FLAG_CONTENT_SPHERE_LIGHT ) {
			lightManData_t.sphereAreaLights[lightManData_t.lightTypeCount.x++] = *static_cast<sphereAreaLight_t*>( node->content );
		} else if ( node->flags & NODE_FLAG_CONTENT_RECTANGLE_LIGHT ) {
			lightManData_t.rectAreaLights[lightManData_t.lightTypeCount.z++] = *static_cast<rectangleAreaLight_t*>( node->content );
		} else if ( node->flags & NODE_FLAG_CONTENT_SUN_LIGHT ) {
			SetSun( *static_cast<sunLight_t*>( node->content ) );
		}

		if ( node->children.size() > 0 ) {
			IterateScene( node );
		}
	}
}

void LightManager::SetSun( const sunLight_t& sunInfos )
{
	// compute sun direction from spherical coordinates
	const float vAngle = asin( sunInfos.sphericalThetaGammaAndPADDING.y );
	const float hAngle = atan2( sunInfos.sphericalThetaGammaAndPADDING.x, 1.0f );

	// compute illuminance using lux as a metric ( Moving Frostbite to Physically Based Rendering 3.0 )
	const float angularDiameterDeg = 0.526f; // TODO: allow to tune this value?

	const float angularRadius	= DirectX::XMConvertToRadians( angularDiameterDeg / 2.0f );
	const float angularDiameter = DirectX::XMConvertToRadians( angularDiameterDeg );

	const float solidAngle = ( 2.0f * DirectX::XM_PI ) * ( 1.0f - cos( 0.5f * angularDiameter ) ); // cf. cone solid angle formulae 2PI(1-cos(0.5*AD*(PI/180)))

	lightManData_t.sunDirectionAndIlluminanceInLux	= DirectX::XMFLOAT4( cos( vAngle ) * cos( hAngle ), cos( vAngle ) * sin( hAngle ), sin( vAngle ), ( sunInfos.colorAndIntensityLux.w * solidAngle ) );
	lightManData_t.sunColorAndAngularRadius			= DirectX::XMFLOAT4( sunInfos.colorAndIntensityLux.x, sunInfos.colorAndIntensityLux.y, sunInfos.colorAndIntensityLux.z, angularRadius );
}

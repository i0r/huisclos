Texture2D alphaMap			: register( t9 );
Texture2D shadowMapTest     : register( t8 );
TextureCube iblEnvTest		: register( t7 );
TextureCube iblDiffuseTest	: register( t6 );
Texture2D iblBRDF           : register( t5 );
Texture2D roughMap          : register( t4 );
Texture2D metalMap          : register( t3 );
Texture2D aoMap             : register( t2 );
Texture2D normalMap         : register( t1 );
Texture2D albedoMap         : register( t0 );

SamplerState shadowSampler;
SamplerState defaultSampler;

struct psData_t
{
	float4 position     : SV_POSITION;
	float4 positionWS   : POSITION0;
	float2 uvCoord      : TEXCOORD0;
	//float2 depth      : TEXCOORD1;
	//float4 ray        : TEXCOORD2;
	float3 normal       : NORMAL0;
	float3 tangent      : TANGENT0;
	float3 binormal		: BINORMAL0;
	float4 shadowCoords	: POSITION1;
};

cbuffer MatrixBuffer : register( b0 )
{
	float4 camPosition;

	matrix viewMatrix;
	matrix projectionMatrix;
	matrix viewProjectionMatrix;

	matrix inverseViewMatrix;
	matrix inverseProjectionMatrix;

	float4 camEye;
};

cbuffer MaterialData : register( b1 )
{
	float3 diffuseColor;
	float  emissivity;
	float3 f0;
	unsigned int flags;
}

struct sphereLight_t
{
	float4 worldPositionRadius;
	float4 color;
};

struct diskAreaLight_t
{
	float4 worldPositionRadius;
	float4 color;
	float4 planeNormal;
};

struct rectangleAreaLight_t
{
	float4 worldPositionRadius;
	float4 color;
	float4 planeNormal;
	float4 up;
	float4 left;
	float4 widthHeight;
};

cbuffer LightData : register( b3 )
{
	float4 sunColorAndAngularRadius;
	float4 sunDirectionAndIlluminanceInLux;

	int4 lightTypeCount; // sphere, disk, rectangle

	sphereLight_t sphereAreaLights[12];
	diskAreaLight_t diskAreaLights[12];
	rectangleAreaLight_t rectAreaLights[12];
}

struct psOutput_t
{
	float4 lightResult;
	float4 bloomResult;
};

#include "common.hlsli"

#define IS_SHADELESS( flags ) ( flags >> 0 ) & 1 

#define IS_ALBEDO_MAPPED( flags ) ( flags >> 1 ) & 1 
#define IS_NORMAL_MAPPED( flags ) ( flags >> 2 ) & 1 
#define IS_AO_MAPPED( flags ) ( flags >> 3 ) & 1 
#define IS_ROUGH_MAPPED( flags ) ( flags >> 4 ) & 1 
#define IS_METAL_MAPPED( flags ) ( flags >> 5 ) & 1 
#define IS_ALPHA_MAPPED( flags ) ( flags >> 6 ) & 1 

psOutput_t main( psData_t p ) : SV_TARGET
{
	psOutput_t output = 
	{
		float4( 0.0f, 0.0f, 0.0f, 1.0f ),
		float4( 0.0f, 0.0f, 0.0f, 1.0f )
	};

	float3 albedo = float3( 1.0f, 1.0f, 1.0f );

	if ( IS_ALBEDO_MAPPED( flags ) ) {
		albedo = ( albedoMap.Sample( defaultSampler, p.uvCoord ).xyz );
	}

	[branch]
	if ( IS_SHADELESS( flags ) ) {
		output.lightResult.rgb = albedo * ( diffuseColor );
	} else {
		float metalness = 0.0f;
		if ( IS_METAL_MAPPED( flags ) ) {
			metalness = metalMap.Sample( defaultSampler, p.uvCoord ).x;
		}

		float3 reflectance = 0.16f * ( f0 * f0 ) * ( 1.0f - metalness ) + albedo * metalness;
		albedo *= diffuseColor; // add diffuse color only AFTER f0 compute (otherwise metal will recieve diffuse lighting!)

		float3 V = normalize( camPosition.xyz - p.positionWS.xyz );
		float3 N = normalize( p.normal );

		if ( IS_NORMAL_MAPPED( flags ) ) {
			float3 normal = normalMap.Sample( defaultSampler, p.uvCoord ).xyz * 2.0f - 1.0f;
			N = normalize( ( normal.x * p.tangent ) + ( normal.y * p.binormal ) + ( normal.z * p.normal ) );
		}

		float ao = 1.0f;
		
		if ( IS_AO_MAPPED( flags ) ) {
			ao = aoMap.Sample( defaultSampler, p.uvCoord ).x;
		}

		float normalLength = length( N * 2.0 - 1.0 );
		float roughness = 0.999f;
		if ( IS_ROUGH_MAPPED( flags ) ) {
			roughness = roughMap.Sample( defaultSampler, p.uvCoord ).x;
		}

		roughness = adjustRoughness( roughness, normalLength );

		output.lightResult.rgb += L_Sun( V, N, albedo, ao, roughness, metalness, reflectance );

		[loop]
		for ( int k = 0; k < lightTypeCount.z; ++k ) {
			output.lightResult.rgb += L_AreaRectangle( V, N, p.positionWS.xyz, albedo, ao, roughness, metalness, reflectance, rectAreaLights[k] );
		}

		[loop]
		for ( int j = 0; j < lightTypeCount.y; ++j ) {
			output.lightResult.rgb += L_AreaDisk( V, N, p.positionWS.xyz, albedo, ao, roughness, metalness, reflectance, p.shadowCoords, diskAreaLights[j] );
		}

		[loop]
		for ( int i = 0; i < lightTypeCount.x; ++i ) {
			output.lightResult.rgb += L_AreaSphere( V, N, p.positionWS.xyz, albedo, ao, roughness, metalness, reflectance, sphereAreaLights[i] );
		}
	}

	// since we are aiming for a rgba16 rt, avoiding NaN and Inf would be cool
	output.bloomResult.rgb = output.lightResult.rgb  * ( emissivity * 2.0 );

	if ( IS_ALPHA_MAPPED( flags ) ) output.lightResult.a *= alphaMap.Sample( defaultSampler, p.uvCoord ).r;

	return output;
}

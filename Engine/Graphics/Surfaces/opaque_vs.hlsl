struct vsData_t
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uvCoord      : TEXCOORD0;
    float3 tangent      : TANGENT;
    float3 binormal		: BINORMAL;
};

cbuffer MatrixBuffer : register( b0 )
{
	float4 camPosition;
	matrix viewProjectionMatrix;
};

cbuffer MatrixModelBuffer : register( b2 )
{
	float4x4 modelMatrix;
};

cbuffer LightMatrixModelBuffer : register( b4 )
{
	float4x4 lightMatrix;
	float4x4 mMatrix;
};

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

psData_t main( vsData_t input )
{
	psData_t output = ( psData_t )0;

	output.positionWS	= mul( modelMatrix, float4( input.position, 1.0f ) );
	output.position		= mul( viewProjectionMatrix, output.positionWS );

	//output.depth		= output.position.zw;

	output.uvCoord		= input.uvCoord;

	output.normal		= normalize( mul( modelMatrix, float4( input.normal, 0.0f ) ) ).xyz;
	output.tangent		= normalize( mul( modelMatrix, float4( input.tangent, 0.0f ) ) ).xyz;
	output.binormal		= normalize( mul( modelMatrix, float4( input.binormal, 0.0f ) ) ).xyz;

	output.shadowCoords = mul( output.positionWS, lightMatrix );

	return output;
}
struct vsData_t
{
	float3 position     : POSITION;
	float3 normal       : NORMAL;
	float2 uvCoord      : TEXCOORD0;
	float3 tangent      : TANGENT;
	float3 bitangent    : BINORMAL;
};

cbuffer LightMatrixModelBuffer : register( b4 )
{
	float4x4 lightMatrix;
	float4x4 modelMatrix;
};

float4 main( vsData_t v ) : SV_POSITION
{
	return mul( float4( v.position, 1.0f ), mul( modelMatrix, lightMatrix ) );
}

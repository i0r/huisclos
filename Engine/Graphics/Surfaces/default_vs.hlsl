struct vsData_t
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uvCoord      : TEXCOORD0;
    float3 tangent      : TANGENT;
    float3 bitangent    : BINORMAL;
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

struct psData_t
{
	float4 position : SV_POSITION;
};

psData_t main( vsData_t v )
{
    psData_t ps;

	ps.position = mul( float4( v.position, 1.0 ), viewProjectionMatrix );
	
	return ps;
}

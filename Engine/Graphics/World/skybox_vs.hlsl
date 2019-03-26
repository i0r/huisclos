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
	matrix viewProjectionMatrix;
};

struct psData_t
{
	float4 position : SV_POSITION;
	float3 uvCoord  : TEXCOORD0;
};

psData_t main( vsData_t v )
{
    psData_t ps;

	ps.position = mul( viewProjectionMatrix, float4( v.position + camPosition.xyz, 1.0 ) );
	ps.uvCoord	= v.position;
	
	return ps;
}

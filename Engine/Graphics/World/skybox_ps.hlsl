TextureCube envMap : register( t0 );

SamplerState defaultSampler;

struct psData_t
{
    float4 position : SV_POSITION;
	float3 uvCoord  : TEXCOORD0;
};

float4 main( psData_t p ) : SV_TARGET
{
	return float4( envMap.Sample( defaultSampler, p.uvCoord ).xyz, 1.0f );
}

struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float3 uvAndDistance : TEXCOORD0;
};

Texture2D diffuseTex : register( t0 );
SamplerState defaultSampler : register( s0 );

float4 main( psDataScreenQuad_t p ) : SV_TARGET
{
	float4 iconColor = diffuseTex.Sample( defaultSampler, p.uvAndDistance.xy );
	iconColor.a *= p.uvAndDistance.z;

	return iconColor;
}

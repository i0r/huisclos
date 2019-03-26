struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D diffuseTex : register( t0 );
SamplerState linearSampler : register( s0 );

float4 main( psDataScreenQuad_t p ) : SV_TARGET
{
	return diffuseTex.Sample( linearSampler, p.uv );
}

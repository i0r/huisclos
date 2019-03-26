struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D diffuseTex : register( t0 );
SamplerState defaultSampler : register( s0 );

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight( int sampleDist, float sigma )
{
	float g = 1.0f / sqrt( 2.0f * 3.14159 * sigma * sigma );
	return ( g * exp( -( sampleDist * sampleDist ) / ( 2 * sigma * sigma ) ) );
}

float4 main( psDataScreenQuad_t p ) : SV_TARGET
{
	float4 color = 0;

	half2 texSize = 0.0f;
	diffuseTex.GetDimensions( texSize.x, texSize.y );

	for ( int i = -6; i < 6; i++ ) {
		float weight = CalcGaussianWeight( i, 0.5f );
		float2 texCoord = p.uv;
		texCoord += ( i / texSize ) * float2( 0, 1 );
		float4 sample = diffuseTex.Sample( defaultSampler, texCoord );
		color += sample * weight;
	}

	return color;
}

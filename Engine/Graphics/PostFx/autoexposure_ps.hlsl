struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D mainRenderTargetTex : register( t0 );
SamplerState defaultSampler : register( s0 );

float3 ACESFilm( float3 x )
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate( ( x*( a*x + b ) ) / ( x*( c*x + d ) + e ) );
}

float3 accurateLinearToSRGB( in float3 linearCol )
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = ( pow( abs( linearCol ), 1.0 / 2.4 ) * 1.055 ) - 0.055;
	float3 sRGB = ( linearCol <= 0.0031308 ) ? sRGBLo : sRGBHi;
	return sRGB;
}

float4 main( psDataScreenQuad_t p ) : SV_TARGET
{
	// computes geometric luminance from a render target
	// source: Reinhard image calibration paper
	float3 texelColor = mainRenderTargetTex.Sample( defaultSampler, p.uv ).rgb;
	texelColor.rgb = ACESFilm( texelColor.rgb ); 
	texelColor = accurateLinearToSRGB( texelColor.rgb );

	float luminance = max( dot( texelColor, float3( 0.299f, 0.587f, 0.114f ) ), 0.0001f ); // magic numbers FTW
	
	return float4( luminance, 1.0f, 1.0f, 1.0f );
}

cbuffer CommonData : register( b4 )
{
	float4 deltaTime;
}

struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D diffuseTex : register( t0 );
Texture2D bloomTex : register( t1 );
Texture2D avgLuminance : register( t2 );
Texture2D avgLuminancePrevious : register( t3 );

SamplerState defaultSampler : register( s0 );
SamplerState defaultSamplerMipMapped : register( s1 );

// CoD: AW interleaved gradient dithering 
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float4 Dither_InterleavedGradient( in float4 finalColor, in float2 pos )
{
	const float3 magic = float3( 0.06711056, 0.00583715, 52.9829189 );
	return float4( finalColor.rgb + frac( magic.z * frac( dot( pos.xy, magic.xy ) ) ) / 255.0, finalColor.a );
}

float4 FilmGrain( in float4 finalColor, const in float2 fragCoordinates )
{
	float strength = 8.0;

	float x = ( fragCoordinates.x + 4.0 ) * ( fragCoordinates.y + 4.0 ) * 10.0f;
	float g = fmod( ( fmod( x, 13.0 ) + 1.0 ) * ( fmod( x, 123.0 ) + 1.0 ), 0.01 ) - 0.005;
	float4 grain = float4( g, g, g, g ) * strength;
	grain = 1.0 - grain;

	return finalColor * grain;
}

float Vignette( const in float2 fragCoordinates )
{
	return 0.3 + 0.7 * pow( 16.0 * fragCoordinates.x * fragCoordinates.y * ( 1.0 - fragCoordinates.x ) * ( 1.0 - fragCoordinates.y ), 0.2 );
}

float3 ACESFilm( float3 x )
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;

	return saturate( ( x*( a*x + b ) ) / ( x*( c*x + d ) + e ) );
}

float computeEV100( float aperture, float shutter_time, float ISO ) {
	// EV number is defined as:
	// 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
	// This gives
	// EV_s = log2 (N^2 / t)
	// EV_100 + log2 (S /100) = log2 (N^2 / t)
	// EV_100 = log2 (N^2 / t) - log2 (S /100)
	// EV_100 = log2 (N^2 / t . 100 / S)
	return log2( ( aperture * aperture ) / shutter_time * 100 / ISO );
}

float computeEV100FromAvgLuminance( float avg_luminance ) {
	// We later use the middle gray at 12.7% in order to have
	// a middle gray at 18% with a sqrt (2) room for specular highlights
	// But here we deal with the spot meter measuring the middle gray
	// which is fixed at 12.5 for matching standard camera
	// constructor settings (i.e. calibration constant K = 12.5)
	return log2( avg_luminance * 100.0 / 12.5 );
}

float convertEV100ToExposure( float EV100 ) {
	// Compute the maximum luminance possible with H_sbs sensitivity
	// maxLum = 78 / ( S * q ) * N^2 / t
	// = 78 / ( S * q ) * 2^ EV_100
	// = 78 / (100 * 0.65) * 2^ EV_100
	// = 1.2 * 2^ EV
	float max_luminance = 1.2 * pow( 2.0, EV100 );
	return 1.0 / max_luminance;
}

float3 accurateLinearToSRGB( in float3 linearCol )
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = ( pow( abs( linearCol ), 1.0 / 2.4 ) * 1.055 ) - 0.055;
	float3 sRGB = ( linearCol <= 0.0031308 ) ? sRGBLo : sRGBHi;
	return sRGB;
}

float3 computeBloomLuminance( float3 bloomColor, float bloomEC, float currentEV )
{
	// currentEV is the value calculated at the previous frame
	float bloomEV = currentEV + bloomEC;
	
	// convert to luminance
	// See equation (12) for explanation about converting EV to luminance
	return bloomColor * pow( 2, bloomEV - 3 );
}

float4 main( psDataScreenQuad_t p ) : SV_TARGET
{
	float4 finalColor = diffuseTex.Sample( defaultSampler, p.uv );

	// exposition
	float lum = avgLuminance.SampleLevel( defaultSamplerMipMapped, p.uv, 10 ).r;
	float prevLum = avgLuminancePrevious.SampleLevel( defaultSamplerMipMapped, p.uv, 10 ).r;

	// Adapt the luminance using Pattanaik's technique
	float adaptedLum = prevLum + ( lum - prevLum ) * ( 1.0 - exp( -deltaTime.x * 0.5f ) );

	float exposure_val = computeEV100FromAvgLuminance( adaptedLum );
	float exposure = convertEV100ToExposure( exposure_val );

	finalColor.rgb *= exposure; // ComputeExposedColor( finalColor.rgb, lum );

	// bloom
	float4 bloomColor = bloomTex.Sample( defaultSampler, p.uv );
	finalColor.rgb += computeBloomLuminance( bloomColor.rgb, 1.0, prevLum );

	// tonemapping + gamma correction
	finalColor.rgb = ACESFilm( finalColor.rgb );
	finalColor.rgb = accurateLinearToSRGB( finalColor.rgb );

	// pbc stuff (vignetting, film grain, ...)
	finalColor *= Vignette( p.uv );

	finalColor = Dither_InterleavedGradient( finalColor, p.position.xy );
	
	return float4( finalColor.rgb, 1.0 );
}

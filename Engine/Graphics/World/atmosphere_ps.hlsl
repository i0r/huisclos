#include "common_bruneton.hlsli"

cbuffer MatrixBuffer : register( b0 )
{
	float4 camPosition;
	matrix viewProjectionMatrix;
	matrix inverseProjectionMatrix;
	matrix inverseViewMatrix;
};

struct psData_t
{
    float4 position : SV_POSITION;
    float4 ray : TEXCOORD0;
};

struct sphereLight_t
{
	float4 worldPositionRadius;
	float4 color;
};

struct diskAreaLight_t
{
	float4 worldPositionRadius;
	float4 color;
	float4 planeNormal;
};

struct rectangleAreaLight_t
{
	float4 worldPositionRadius;
	float4 color;
	float4 planeNormal;
	float4 up;
	float4 left;
	float4 widthHeight;
};

cbuffer LightData : register( b3 )
{
	float4 sunColorAndAngularRadius;
	float4 sunDirectionAndIlluminanceInLux;

	int4 lightTypeCount; // sphere, disk, rectangle

	sphereLight_t sphereAreaLights[12];
	diskAreaLight_t diskAreaLights[12];
	rectangleAreaLight_t rectAreaLights[12];
}

Texture3D inscatterTex : register( t0 );
Texture2D irradianceTex : register( t1 );
Texture2D transmittanceTex : register( t2 );

SamplerState defaultSampler;

#define FIX

float4 texture4D( Texture3D table, float r, float mu, float muS, float nu )
{
	float H = sqrt( Rt * Rt - Rg * Rg );
	float rho = sqrt( r * r - Rg * Rg );
	float rmu = r * mu;
	float delta = rmu * rmu - r * r + Rg * Rg;
	float4 cst = rmu < 0.0 && delta > 0.0 ? float4( 1.0, 0.0, 0.0, 0.5 - 0.5 / float( RES_MU ) ) : float4( -1.0, H * H, H, 0.5 + 0.5 / float( RES_MU ) );
	float uR = 0.5 / float( RES_R ) + rho / H * ( 1.0 - 1.0 / float( RES_R ) );
	float uMu = cst.w + ( rmu * cst.x + sqrt( delta + cst.y ) ) / ( rho + cst.z ) * ( 0.5 - 1.0 / float( RES_MU ) );
	// paper formula
	//float uMuS = 0.5 / float(RES_MU_S) + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / float(RES_MU_S));
	// better formula
	float uMuS = 0.5 / float( RES_MU_S ) + ( atan( max( muS, -0.1975 ) * tan( 1.26 * 1.1 ) ) / 1.1 + ( 1.0 - 0.26 ) ) * 0.5 * ( 1.0 - 1.0 / float( RES_MU_S ) );

	float lerp = ( nu + 1.0 ) / 2.0 * ( float( RES_NU ) - 1.0 );
	float uNu = floor( lerp );
	lerp = lerp - uNu;
	return table.Sample( defaultSampler, float3( ( uNu + uMuS ) / float( RES_NU ), uMu, uR ) ) * ( 1.0 - lerp ) +
		table.Sample( defaultSampler, float3( ( uNu + uMuS + 1.0 ) / float( RES_NU ), uMu, uR ) ) * lerp;
}


//inscattered light along ray x+tv, when sun in direction s (=S[L]-T(x,x0)S[L]|x0)
float3 inscatter( inout float3 x, inout float t, float3 v, float3 s,
	out float r, out float mu, out float3 attenuation )
{
	attenuation = float3( 1.0, 1.0, 1.0 );

	float3 result;
	r = length( x );
	mu = dot( x, v ) / r;
	float d = -r * mu - sqrt( r * r * ( mu * mu - 1.0 ) + Rt * Rt );
	if ( d > 0.0 ) { // if x in space and ray intersects atmosphere
					 // move x to nearest intersection of ray with top atmosphere boundary
		x += d * v;
		t -= d;
		mu = ( r * mu + d ) / Rt;
		r = Rt;
	}
	if ( r <= Rt ) { // if ray intersects atmosphere
		float nu = dot( v, s );
		float muS = dot( x, s ) / r;
		float phaseR = phaseFunctionR( nu );
		float phaseM = phaseFunctionM( nu );
		float4 inscatter = max( texture4D( inscatterTex, r, mu, muS, nu ), 0.0 );
		if ( t > 0.0 ) {
			float3 x0 = x + t * v;
			float r0 = length( x0 );
			float rMu0 = dot( x0, v );
			float mu0 = rMu0 / r0;
			float muS0 = dot( x0, s ) / r0;
#ifdef FIX
			// avoids imprecision problems in transmittance computations based on textures
			attenuation = analyticTransmittance( r, mu, t );
#else
			attenuation = transmittance( r, mu, v, x0 );
#endif
			if ( r0 > Rg + 0.01 ) {
				// computes S[L]-T(x,x0)S[L]|x0
				inscatter = max( inscatter - attenuation.rgbr *
					texture4D( inscatterTex, r0, mu0, muS0, nu ), 0.0 );
#ifdef FIX
				// avoids imprecision problems near horizon by interpolating between two points above and below horizon
				const float EPS = 0.004;
				float muHoriz = -sqrt( 1.0 - ( Rg / r ) * ( Rg / r ) );
				if ( abs( mu - muHoriz ) < EPS ) {
					float a = ( ( mu - muHoriz ) + EPS ) / ( 2.0 * EPS );

					mu = muHoriz - EPS;
					r0 = sqrt( r * r + t * t + 2.0 * r * t * mu );
					mu0 = ( r * mu + t ) / r0;
					float4 inScatter0 = texture4D( inscatterTex,
						r, mu, muS, nu );
					float4 inScatter1 = texture4D( inscatterTex,
						r0, mu0, muS0, nu );
					float4 inScatterA = max( inScatter0 - attenuation.rgbr * inScatter1, 0.0 );

					mu = muHoriz + EPS;
					r0 = sqrt( r * r + t * t + 2.0 * r * t * mu );
					mu0 = ( r * mu + t ) / r0;
					inScatter0 = texture4D( inscatterTex, r, mu, muS, nu );
					inScatter1 = texture4D( inscatterTex, r0, mu0, muS0, nu );
					float4 inScatterB = max( inScatter0 - attenuation.rgbr * inScatter1, 0.0 );

					inscatter = lerp( inScatterA, inScatterB, a );
				}
#endif
			}
		}
#ifdef FIX
		// avoids imprecision problems in Mie scattering when sun is below horizon
		inscatter.w *= smoothstep( 0.00, 0.02, muS );
#endif
		result = max( inscatter.rgb * phaseR + getMie( inscatter ) * phaseM, 0.0 );
	} else { // x in space and ray looking in space
		result = float3( 0.0, 0.0, 0.0 );
	}

	float sunIntensity = sunDirectionAndIlluminanceInLux.w * saturate( dot( 1.0f, sunDirectionAndIlluminanceInLux.xyz ) );

	return result * sunIntensity;
}

float3 transmittance( float r, float mu )
{
	float2 uv = getTransmittanceUV( r, mu );

	return transmittanceTex.Sample( defaultSampler, uv ).xyz;
}

// transmittance(=transparency) of atmosphere for infinite ray (r,mu)
// (mu=cos(view zenith angle)), or zero if ray intersects ground
float3 transmittanceWithShadow( float r, float mu ) {
	return mu < -sqrt( 1.0 - ( Rg / r ) * ( Rg / r ) ) ? float3( 0.0, 0.0, 0.0 ) : transmittance( r, mu );
}

float3 irradiance( float r, float muS ) {
	float2 uv = getIrradianceUV( r, muS );
	return irradianceTex.Sample( defaultSampler, uv ).rgb;
}

// direct sun light for ray x+tv, when sun in direction s (=L0)
float3 sunColor( float3 x, float t, float3 v, float3 s, float r, float mu )
{
    if ( t > 0.0 ) {
        return float3( 0.0, 0.0, 0.0 );
    } else {
        float3 transmittance = r <= Rt ?
            transmittanceWithShadow( r, mu ) : float3( 1.0, 1.0, 1.0 ); // T(x,xo)

		float sunIntensity = sunDirectionAndIlluminanceInLux.w * saturate( dot( 1.0f, sunDirectionAndIlluminanceInLux.xyz ) );
        float isun = step( cos( M_PI / 180.0 ), dot( v, s ) ) * sunIntensity; // Lsun
        
		return transmittance * isun * 10.0f; // Eq (9)
    }
}

float3 accurateLinearToSRGB( in float3 linearCol )
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = ( pow( abs( linearCol ), 1.0 / 2.4 ) * 1.055 ) - 0.055;
	float3 sRGB = ( linearCol <= 0.0031308 ) ? sRGBLo : sRGBHi;
	return sRGB;
}

struct psOutput_t
{
	float4 lightResult;
	float4 bloomResult;
};

float3 accurateSRGBToLinear( in float3 sRGBCol )
{
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow( ( sRGBCol + 0.055 ) / 1.055, 2.4 );
	float3 linearRGB = ( sRGBCol <= 0.04045 ) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

psOutput_t main( psData_t input ) : SV_TARGET
{
	psOutput_t output =
	{
		float4( 0.0f, 0.0f, 0.0f, 1.0f ),
		float4( 0.0f, 0.0f, 0.0f, 1.0f )
	};

    float3 v = normalize( input.ray.xyz );
	float3 x = float3( 0.0f, Rg, 0.0f );

    float r = length( x );
    float mu = dot( x, v ) / r;
    float t = -r * mu - sqrt( r * r * ( mu * mu - 1.0 ) + Rg * Rg );

	float3 g = x - float3( 0.0, 0.0, Rg + 10.0 );
	float a = v.x * v.x + v.y * v.y - v.z * v.z;
	float b = 2.0 * ( g.x * v.x + g.y * v.y - g.z * v.z );
	float c = g.x * g.x + g.y * g.y - g.z * g.z;
	float d = -( b + sqrt( b * b - 4.0 * a * c ) ) / ( 2.0 * a );
	bool cone = d > 0.0 && abs( x.z + d * v.z - Rg ) <= 10.0;

	if ( t > 0.0 ) {
		if ( cone && d < t ) {
			t = d;
		}
	} else if ( cone ) {
		t = d;
	}

	float3 attenuation;

	float3 inscatterColor = inscatter( x, t, v, sunDirectionAndIlluminanceInLux.xyz, r, mu, attenuation ); //S[L]-T(x,xs)S[l]|xs
	float3 sunColor0 = sunColor( x, t, v, sunDirectionAndIlluminanceInLux.xyz, r, mu ); //L0

	float4 atmosphere = float4( ( sunColor0 + inscatterColor ), 1.0 );
	
	output.lightResult = atmosphere;
	output.bloomResult.rgb = saturate( output.lightResult.rgb ) * 0.25f;

	return output;  // Eq (16)
}

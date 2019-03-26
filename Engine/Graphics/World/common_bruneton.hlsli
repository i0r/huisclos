#include "AtmosphereConstants.h"

struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

cbuffer PrecoputeAtmosphere : register( b0 )
{
    float4 dhdH;
    float r;
    float k;
    int layer;
    float first;
};

// ----------------------------------------------------------------------------
// PHYSICAL MODEL PARAMETERS
// ----------------------------------------------------------------------------

static const float PI = 3.1415926535897932384626433f;
static const float AVERAGE_GROUND_REFLECTANCE = 0.1;

// Rayleigh
static const float HR = 8.0;
static const float3 betaR = float3( 5.8e-3, 1.35e-2, 3.31e-2 );

// Mie
// DEFAULT
static const float HM = 1.2;
static const float3 betaMSca = float3( 4e-3, 4e-3, 4e-3 );
static const float3 betaMEx = betaMSca / 0.9;
static const float mieG = 0.8;
// CLEAR SKY
//static const float HM = 1.2;
//static const float3 betaMSca = float3( 20e-3, 20e-3, 20e-3 );
//static const float3 betaMEx = betaMSca / 0.9;
//static const float mieG = 0.76;
// PARTLY CLOUDY
//static const float HM = 3.0;
//static const float3 betaMSca = float3( 3e-3, 3e-3, 3e-3 );
//static const float3 betaMEx = betaMSca / 0.9;
//static const float mieG = 0.65;

// ----------------------------------------------------------------------------
// NUMERICAL INTEGRATION PARAMETERS
// ----------------------------------------------------------------------------

static const int TRANSMITTANCE_INTEGRAL_SAMPLES = 500;
static const int INSCATTER_INTEGRAL_SAMPLES = 50;
static const int IRRADIANCE_INTEGRAL_SAMPLES = 32;
static const int INSCATTER_SPHERICAL_INTEGRAL_SAMPLES = 16;

static const float M_PI = 3.141592657;

// nearest intersection of ray r,mu with ground or top atmosphere boundary
// mu=cos(ray zenith angle at ray origin)
float limit( float r, float mu )
{
    float dout = -r * mu + sqrt( r * r * ( mu * mu - 1.0 ) + RL * RL );
    float delta2 = r * r * ( mu * mu - 1.0 ) + Rg * Rg;
    if ( delta2 >= 0.0 ) {
        float din = -r * mu - sqrt( delta2 );
        if ( din >= 0.0 ) {
            dout = min( dout, din );
        }
    }
    return dout;
}

void getTransmittanceRMu( in float4 position, out float r, out float muS )
{
    r = -position.y / float( TRANSMITTANCE_H );
    muS = position.x / float( TRANSMITTANCE_W );

    r = Rg + ( r * r ) * ( Rt - Rg );
    muS = -0.15 + tan( 1.5 * muS ) / tan( 1.5 ) * ( 1.0 + 0.15 );
}

void getIrradianceRMuS( in float4 position, out float r, out float muS ) 
{
	r = Rg + ( position.y - 0.5 ) / ( float( SKY_H ) - 1.0 ) * ( Rt - Rg );
	muS = -0.2 + ( position.x - 0.5 ) / ( float( SKY_W ) - 1.0 ) * ( 1.0 + 0.2 );
}

float2 getIrradianceUV( float r, float muS ) 
{
	float uR = ( r - Rg ) / ( Rt - Rg );
	float uMuS = ( muS + 0.2 ) / ( 1.0 + 0.2 );

	return float2( uMuS, uR );
}

float2 getTransmittanceUV( float r, float mu )
{
	float uR, uMu;
	uR = sqrt( ( r - Rg ) / ( Rt - Rg ) );
	uMu = atan( ( mu + 0.15 ) / ( 1.0 + 0.15 ) * tan( 1.5 ) ) / 1.5;

	return float2( uMu, uR );
}

void getMuMuSNu( in float4 pos, float r, float4 dhdH, out float mu, out float muS, out float nu )
{
    float x = pos.x - 0.5;
    float y = pos.y - 0.5;

    if ( y < float( RES_MU ) / 2.0 ) {
        float d = 1.0 - y / ( float( RES_MU ) / 2.0 - 1.0 );
        d = min( max( dhdH.z, d * dhdH.w ), dhdH.w * 0.999 );
        mu = ( Rg * Rg - r * r - d * d ) / ( 2.0 * r * d );
        mu = min( mu, -sqrt( 1.0 - ( Rg / r ) * ( Rg / r ) ) - 0.001 );
    } else {
        float d = ( y - float( RES_MU ) / 2.0 ) / ( float( RES_MU ) / 2.0 - 1.0 );
        d = min( max( dhdH.x, d * dhdH.y ), dhdH.y * 0.999 );
        mu = ( Rt * Rt - r * r - d * d ) / ( 2.0 * r * d );
    }
    muS = fmod( x, float( RES_MU_S ) ) / ( float( RES_MU_S ) - 1.0 );
    // paper formula
    //muS = -(0.6 + log(1.0 - muS * (1.0 -  exp(-3.6)))) / 3.0;
    // better formula
    muS = tan( ( 2.0 * muS - 1.0 + 0.26 ) * 1.1 ) / tan( 1.26 * 1.1 );
    nu = -1.0 + floor( x / float( RES_MU_S ) ) / ( float( RES_NU ) - 1.0 ) * 2.0;
}

// Rayleigh phase function
float phaseFunctionR( float mu )
{
    return ( 3.0 / ( 16.0 * PI ) ) * ( 1.0 + mu * mu );
}

// Mie phase function
float phaseFunctionM( float mu )
{
    return 1.5 * 1.0 / ( 4.0 * PI ) * ( 1.0 - mieG*mieG ) * pow( 1.0 + ( mieG*mieG ) - 2.0*mieG*mu, -3.0 / 2.0 ) * ( 1.0 + mu * mu ) / ( 2.0 + mieG*mieG );
}

// optical depth for ray (r,mu) of length d, using analytic formula
// (mu=cos(view zenith angle)), intersections with ground ignored
// H=height scale of exponential density function
float opticalDepth( float H, float r, float mu, float d )
{
    float a = sqrt( ( 0.5 / H )*r );
    float2 a01 = a*float2( mu, mu + d / r );
    float2 a01s = sign( a01 );
    float2 a01sq = a01*a01;
    float x = a01s.y > a01s.x ? exp( a01sq.x ) : 0.0;
    float2 y = a01s / ( 2.3193*abs( a01 ) + sqrt( 1.52*a01sq + 4.0 ) ) * float2( 1.0, exp( -d / H*( d / ( 2.0*r ) + mu ) ) );
    return sqrt( ( 6.2831*H )*r ) * exp( ( Rg - r ) / H ) * ( x + dot( y, float2( 1.0, -1.0 ) ) );
}

// transmittance(=transparency) of atmosphere for ray (r,mu) of length d
// (mu=cos(view zenith angle)), intersections with ground ignored
// uses analytic formula instead of transmittance texture
float3 analyticTransmittance( float r, float mu, float d )
{
    return exp( -betaR * opticalDepth( HR, r, mu, d ) - betaMEx * opticalDepth( HM, r, mu, d ) );
}

// approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
float3 getMie( float4 rayMie )
{ // rayMie.rgb=C*, rayMie.w=Cm,r
    return rayMie.rgb * rayMie.w / max( rayMie.r, 1e-4 ) * ( betaR.r / betaR );
}

float3 HDR( float3 L )
{
    L = L * 0.4;
    L.r = L.r < 1.413 ? pow( abs( L.r * 0.38317 ), 1.0 / 2.2 ) : 1.0 - exp( -L.r );
    L.g = L.g < 1.413 ? pow( abs( L.g * 0.38317 ), 1.0 / 2.2 ) : 1.0 - exp( -L.g );
    L.b = L.b < 1.413 ? pow( abs( L.b * 0.38317 ), 1.0 / 2.2 ) : 1.0 - exp( -L.b );
    return L;
}

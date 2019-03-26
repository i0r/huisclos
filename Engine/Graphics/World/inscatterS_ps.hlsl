#include "common_bruneton.hlsli"

/**
* Precomputed Atmospheric Scattering
* Copyright (c) 2008 INRIA
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holders nor the names of its
*    contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
* Author: Eric Bruneton
*/

// computes single scattering (line 3 in algorithm 4.1)

Texture2D deltaETex : register( t0 );
Texture3D deltaSRTex : register( t1 );
Texture3D deltaSMTex : register( t2 );
Texture2D transmittanceTex : register( t3 );

SamplerState transmittanceSample;

float3 transmittance( float r, float mu )
{
    float2 uv = getTransmittanceUV( r, mu );

    return transmittanceTex.Sample( transmittanceSample, uv ).xyz;
}

static const float dphi = M_PI / float( INSCATTER_SPHERICAL_INTEGRAL_SAMPLES );
static const float dtheta = M_PI / float( INSCATTER_SPHERICAL_INTEGRAL_SAMPLES );

// transmittance(=transparency) of atmosphere between x and x0
// assume segment x,x0 not intersecting ground
// d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x)
float3 transmittance( float r, float mu, float d )
{
    float3 result;
    float r1 = sqrt( r * r + d * d + 2.0 * r * mu * d );
    float mu1 = ( r * mu + d ) / r1;
    if ( mu > 0.0 ) {
        result = min( transmittance( r, mu ) / transmittance( r1, mu1 ), 1.0 );
    } else {
        result = min( transmittance( r1, -mu1 ) / transmittance( r, -mu ), 1.0 );
    }
    return result;
}

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
    return table.Sample( transmittanceSample, float3( ( uNu + uMuS ) / float( RES_NU ), uMu, uR ) ) * ( 1.0 - lerp ) +
        table.Sample( transmittanceSample, float3( ( uNu + uMuS + 1.0 ) / float( RES_NU ), uMu, uR ) ) * lerp;
}

float3 irradianceDeltaE( float r, float muS )
{
    float2 uv = getIrradianceUV( r, muS );
    return deltaETex.Sample( transmittanceSample, uv ).rgb;
}

void inscatter( float r, float mu, float muS, float nu, out float3 raymie )
{
    r = clamp( r, Rg, Rt );
    mu = clamp( mu, -1.0, 1.0 );
    muS = clamp( muS, -1.0, 1.0 );
    float var = sqrt( 1.0 - mu * mu ) * sqrt( 1.0 - muS * muS );
    nu = clamp( nu, muS * mu - var, muS * mu + var );

    float cthetamin = -sqrt( 1.0 - ( Rg / r ) * ( Rg / r ) );

    float3 v = float3( sqrt( 1.0 - mu * mu ), 0.0, mu );
    float sx = v.x == 0.0 ? 0.0 : ( nu - muS * mu ) / v.x;
    float3 s = float3( sx, sqrt( max( 0.0, 1.0 - sx * sx - muS * muS ) ), muS );

    raymie = float3( 0.0f, 0.0f, 0.0f );

    // integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7)
    for ( int itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta ) {
        float theta = ( float( itheta ) + 0.5 ) * dtheta;
        float ctheta = cos( theta );

        float greflectance = 0.0;
        float dground = 0.0;
        float3 gtransp = float3( 0.0f, 0.0f, 0.0f );
        if ( ctheta < cthetamin ) { // if ground visible in direction w
                                    // compute transparency gtransp between x and ground
            greflectance = AVERAGE_GROUND_REFLECTANCE / M_PI;
            dground = -r * ctheta - sqrt( r * r * ( ctheta * ctheta - 1.0 ) + Rg * Rg );
            gtransp = transmittance( Rg, -( r * ctheta + dground ) / Rg, dground );
        }

        for ( int iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi ) {
            float phi = ( float( iphi ) + 0.5 ) * dphi;
            float dw = dtheta * dphi * sin( theta );
            float3 w = float3( cos( phi ) * sin( theta ), sin( phi ) * sin( theta ), ctheta );

            float nu1 = dot( s, w );
            float nu2 = dot( v, w );
            float pr2 = phaseFunctionR( nu2 );
            float pm2 = phaseFunctionM( nu2 );

            // compute irradiance received at ground in direction w (if ground visible) =deltaE
            float3 gnormal = ( float3( 0.0, 0.0, r ) + dground * w ) / Rg;
            float3 girradiance = irradianceDeltaE( Rg, dot( gnormal, s ) );

            float3 raymie1; // light arriving at x from direction w

                          // first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE
            raymie1 = greflectance * girradiance * gtransp;

            // second term = inscattered light, =deltaS
            if ( first == 1.0 ) {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = phaseFunctionR( nu1 );
                float pm1 = phaseFunctionM( nu1 );
                float3 ray1 = texture4D( deltaSRTex, r, w.z, muS, nu1 ).rgb;
                float3 mie1 = texture4D( deltaSMTex, r, w.z, muS, nu1 ).rgb;
                raymie1 += ray1 * pr1 + mie1 * pm1;
            } else {
                raymie1 += texture4D( deltaSRTex, r, w.z, muS, nu1 ).rgb;
            }

            // light coming from direction w and scattered in direction v
            // = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction)
            // see Eq (7)
            raymie += raymie1 * ( betaR * exp( -( r - Rg ) / HR ) * pr2 + betaMSca * exp( -( r - Rg ) / HM ) * pm2 ) * dw;
        }
    }

    // output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1)
}

float4 main( psDataScreenQuad_t data ) : SV_TARGET
{
    float3 raymie;
    float mu, muS, nu;
    getMuMuSNu( data.position, r, dhdH, mu, muS, nu );
    inscatter( r, mu, muS, nu, raymie );

    return float4( raymie, 0.0 );
}

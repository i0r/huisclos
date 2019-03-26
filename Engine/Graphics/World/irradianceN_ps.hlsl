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

 // computes ground irradiance due to direct sunlight E[L0] (line 2 in algorithm 4.1)
Texture3D deltaSRTex : register( t0 );
Texture3D deltaSMTex : register( t1 );
Texture2D transmittanceTex : register( t2 );

SamplerState transmittanceSample;

float3 transmittance( float r, float mu )
{
    float2 uv = getTransmittanceUV( r, mu );

    return transmittanceTex.Sample( transmittanceSample, uv ).xyz;
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

static const float dphi = M_PI / float( IRRADIANCE_INTEGRAL_SAMPLES );
static const float dtheta = M_PI / float( IRRADIANCE_INTEGRAL_SAMPLES );

float4 main( psDataScreenQuad_t data ) : SV_TARGET
{
    float r, muS;
    getIrradianceRMuS( data.position, r, muS );
    float3 s = float3( max( sqrt( 1.0 - muS * muS ), 0.0 ), 0.0, muS );

    float3 result = float3( 0.0f, 0.0f, 0.0f );
    // integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15)
    for ( int iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi ) {
        float phi = ( float( iphi ) + 0.5 ) * dphi;
        for ( int itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta ) {
            float theta = ( float( itheta ) + 0.5 ) * dtheta;
            float dw = dtheta * dphi * sin( theta );
            float3 w = float3( cos( phi ) * sin( theta ), sin( phi ) * sin( theta ), cos( theta ) );
            float nu = dot( s, w );
            if ( first == 1.0 ) {
                // first iteration is special because Rayleigh and Mie were stored separately,
                // without the phase functions factors; they must be reintroduced here
                float pr1 = phaseFunctionR( nu );
                float pm1 = phaseFunctionM( nu );
                float3 ray1 = texture4D( deltaSRTex, r, w.z, muS, nu ).rgb;
                float3 mie1 = texture4D( deltaSMTex, r, w.z, muS, nu ).rgb;
                result += ( ray1 * pr1 + mie1 * pm1 ) * w.z * dw;
            } else {
                result += texture4D( deltaSRTex, r, w.z, muS, nu ).rgb * w.z * dw;
            }
        }
    }

    return float4( result, 0.0 );
}

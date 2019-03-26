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

 // computes higher order scattering (line 9 in algorithm 4.1)

Texture2D transmittanceTex : register( t0 );
Texture3D deltaJTex : register( t1 );

SamplerState transmittanceSample;

float3 transmittance( float r, float mu )
{
    float2 uv = getTransmittanceUV( r, mu );

    return transmittanceTex.Sample( transmittanceSample, uv ).xyz;
}

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

float3 integrand( float r, float mu, float muS, float nu, float t )
{
    float ri = sqrt( r * r + t * t + 2.0 * r * mu * t );
    float mui = ( r * mu + t ) / ri;
    float muSi = ( nu * t + muS * r ) / ri;
    return texture4D( deltaJTex, ri, mui, muSi, nu ).rgb * transmittance( r, mu, t );
}

float3 inscatter( float r, float mu, float muS, float nu )
{
    float3 raymie = float3( 0.0f, 0.0f, 0.0f );
    float dx = limit( r, mu ) / float( INSCATTER_INTEGRAL_SAMPLES );
    float xi = 0.0;
    float3 raymiei = integrand( r, mu, muS, nu, 0.0 );
    for ( int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i ) {
        float xj = float( i ) * dx;
        float3 raymiej = integrand( r, mu, muS, nu, xj );
        raymie += ( raymiei + raymiej ) / 2.0 * dx;
        xi = xj;
        raymiei = raymiej;
    }
    return raymie;
}

float4 main( psDataScreenQuad_t data ) : SV_TARGET
{
    float mu, muS, nu;
    getMuMuSNu( data.position, r, dhdH, mu, muS, nu );

    return float4( inscatter( r, mu, muS, nu ), 0.0f );
}

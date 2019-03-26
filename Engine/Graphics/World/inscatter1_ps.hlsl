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

Texture2D transmittanceTex : register( t0 );

SamplerState transmittanceSample;

float3 transmittance( float r, float mu )
{
    float2 uv = getTransmittanceUV( r, mu );

    return transmittanceTex.Sample( transmittanceSample, uv ).xyz;
}

float3 transmittance( float r, float mu, float3 v, float3 x0 )
{
    float3 result;
    float r1 = length( x0 );
    float mu1 = dot( x0, v ) / r;
    if ( mu > 0.0 ) {
        result = min( transmittance( r, mu ) / transmittance( r1, mu1 ), 1.0 );
    } else {
        result = min( transmittance( r1, -mu1 ) / transmittance( r, -mu ), 1.0 );
    }
    return result;
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

void integrand( float r, float mu, float muS, float nu, float t, out float3 ray, out float3 mie )
{
    ray = float3( 0.0f, 0.0f, 0.0f );
    mie = float3( 0.0f, 0.0f, 0.0f );
    float ri = sqrt( r * r + t * t + 2.0 * r * mu * t );
    float muSi = ( nu * t + muS * r ) / ri;
    ri = max( Rg, ri );
    if ( muSi >= -sqrt( 1.0 - Rg * Rg / ( ri * ri ) ) ) {
        float3 ti = transmittance( r, mu, t ) * transmittance( ri, muSi );
        ray = exp( -( ri - Rg ) / HR ) * ti;
        mie = exp( -( ri - Rg ) / HM ) * ti;
    }
}

void inscatter( float r, float mu, float muS, float nu, out float3 ray, out float3 mie )
{
    ray = float3( 0.0f, 0.0f, 0.0f );
    mie = float3( 0.0f, 0.0f, 0.0f );
    float dx = limit( r, mu ) / float( INSCATTER_INTEGRAL_SAMPLES );
    float xi = 0.0;
    float3 rayi;
    float3 miei;
    integrand( r, mu, muS, nu, 0.0, rayi, miei );
    for ( int i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i ) {
        float xj = float( i ) * dx;
        float3 rayj;
        float3 miej;
        integrand( r, mu, muS, nu, xj, rayj, miej );
        ray += ( rayi + rayj ) / 2.0 * dx;
        mie += ( miei + miej ) / 2.0 * dx;
        xi = xj;
        rayi = rayj;
        miei = miej;
    }
    ray *= betaR;
    mie *= betaMSca;
}

struct mrt_t
{
    float4 ray : SV_TARGET0;
    float4 mie : SV_TARGET1;
};

mrt_t main( psDataScreenQuad_t data )
{
    mrt_t o;

    float3 ray;
    float3 mie;
    float mu, muS, nu;
    getMuMuSNu( data.position, r, dhdH, mu, muS, nu );
    inscatter( r, mu, muS, nu, ray, mie );

    // store separately Rayleigh and Mie contributions, WITHOUT the phase function factor
    // (cf "Angular precision")
    o.ray = float4( ray, 1.0f );
    o.mie = float4( mie, 1.0f );

    return o;
}

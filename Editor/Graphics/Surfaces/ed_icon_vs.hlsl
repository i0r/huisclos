struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float3 uvAndDistance : TEXCOORD0;
};

cbuffer MatrixBuffer : register( b0 )
{
	float4 camPosition;
	matrix viewProjectionMatrix;
};

cbuffer IconBuffer : register( b6 )
{
	float4 positionAndRadius;
}

float4 ComputePosition( in float3 pos, in float size, in float2 vPos )
{
	float3 toEye = normalize( camPosition.xyz - pos );

	float3 up    = float3( 0.0f, 1.0f, 0.0f );

	float3 right = cross( toEye, up );
	up           = cross( toEye, right );

	pos += ( right * size * vPos.x ) + ( up * size * vPos.y );

	return mul( viewProjectionMatrix, float4( pos, 1.0f ) );
}

psDataScreenQuad_t main( uint vI : SV_VERTEXID )
{
	psDataScreenQuad_t o = ( psDataScreenQuad_t )0;

	o.uvAndDistance = float3( vI % 2, ( vI % 4 ) >> 1, 1.0f );
    o.uvAndDistance.z = distance( camPosition.xyz, positionAndRadius.xyz );
	o.position = ComputePosition( positionAndRadius.xyz, positionAndRadius.w, o.uvAndDistance.xy );

    if ( o.uvAndDistance.z < 4.0f ) {
		o.uvAndDistance.z /= 32.0f;
	} else if ( o.uvAndDistance.z > 96.0f ) {
		o.uvAndDistance.z = 0.0f;
	}

	return o;
}

struct psData_t
{
	float4 position : SV_POSITION;
    float4 ray : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

cbuffer MatrixBuffer : register( b0 )
{
	float4 camPosition;
	matrix viewProjectionMatrix;
	matrix inverseProjectionMatrix;
	matrix inverseViewMatrix;
};

psData_t main( uint vI : SV_VERTEXID )
{
    psData_t ps;

    float2 texcoord = float2( vI & 1, vI >> 1 );
    float3 pos = float3( ( texcoord.x - 0.5f ) * 2.0f, -( texcoord.y - 0.5f ) * 2.0f, 0.0f );

    ps.uv = float2( vI % 2, ( vI % 4 ) >> 1 );

    ps.position = float4( pos, 1.0f );

    float4 invPosition = mul( inverseProjectionMatrix, ps.position );
    ps.ray = mul( inverseViewMatrix, float4( invPosition.xyz, 0.0f ) );

    return ps;
}

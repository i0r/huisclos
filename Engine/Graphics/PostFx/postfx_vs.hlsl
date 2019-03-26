struct psDataScreenQuad_t
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

psDataScreenQuad_t main( uint vI : SV_VERTEXID )
{
	psDataScreenQuad_t output = ( psDataScreenQuad_t )0;

	float2 texcoord = float2( vI & 1, vI >> 1 );
	output.position = float4( ( texcoord.x - 0.5f ) * 2.0f, -( texcoord.y - 0.5f ) * 2.0f, 0.0f, 1.0f );
	output.uv = float2( vI % 2, ( vI % 4 ) >> 1 );

	return output;
}

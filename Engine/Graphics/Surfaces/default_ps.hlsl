struct psData_t
{
    float4 position : SV_POSITION;
};

float4 main( psData_t p ) : SV_TARGET
{
	return float4( 1.00f, 0.11f, 0.68f, 1.00f );
}

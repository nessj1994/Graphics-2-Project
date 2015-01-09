#pragma pack_matrix(row_major)

struct P_IN
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
};

float4 main( P_IN input ) : SV_TARGET
{
	return input.color;
}
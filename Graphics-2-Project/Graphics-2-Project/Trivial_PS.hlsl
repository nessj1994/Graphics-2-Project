#pragma pack_matrix(row_major)

struct P_IN
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;
};

float4 main(P_IN input) : SV_TARGET
{
	float3 lightdir = { 0, -1, 0 };
	float LightRatio = clamp(dot(-lightdir, input.normal), 0, 1);
	float4 lightcolor = { 1.0f, 1.0f, 1.0f, 1.0f };
	return saturate(input.color * LightRatio * lightcolor);
}
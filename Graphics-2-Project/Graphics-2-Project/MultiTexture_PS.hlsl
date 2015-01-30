#pragma pack_matrix(row_major)
Texture2D shaderTexture[2];
SamplerState sampleType;

struct P_IN
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;
};

float4 main(P_IN input) : SV_TARGET
{

	float4 textureColor1 = shaderTexture[0].Sample(sampleType, input.uv);
	float4 textureColor2 = shaderTexture[1].Sample(sampleType, input.uv);


		return saturate(textureColor1 * textureColor2);
}
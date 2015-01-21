
TextureCube shaderTexture;
SamplerState sampleType;

struct P_IN
{
	float3 posL : POSITION;
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float3 uv : UV;
};

float4 main(P_IN input) : SV_TARGET
{

		float4 textureColor = shaderTexture.Sample(sampleType, input.posL);

		return textureColor;
}
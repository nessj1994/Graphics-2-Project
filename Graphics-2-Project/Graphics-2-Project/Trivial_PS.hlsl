#pragma pack_matrix(row_major)
Texture2D shaderTexture;
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
	float3 lightdir = { 0, -1, 0 };
	float LightRatio = clamp(dot(-lightdir, input.normal), 0, 1);
	float4 lightcolor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float4 textureColor = shaderTexture.Sample(sampleType, input.uv);

		float3 surfacePos = { input.posH.x, input.posH.y, input.posH.z };
		float3 pointLightPos = { 0, 0, -1 };
		float3 pointLightDir = normalize(pointLightPos - surfacePos);
		float pointLightRatio = clamp(dot(pointLightDir, input.normal), 0, 1);
	float4 pointLightColor = { 1.0f, 0.1f, 0.1f, 1.0f };


	return saturate((textureColor * LightRatio * lightcolor) + (pointLightRatio * pointLightColor * textureColor));
}
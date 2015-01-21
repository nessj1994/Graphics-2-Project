#pragma pack_matrix(row_major)

struct V_IN
{
	float3 posL : POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;
};
struct V_OUT
{
	float3 posL : POSITION;
	float4 posH : SV_POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float3 uv : UV;
};
cbuffer OBJECT : register(b0)
{
	float4x4 worldMatrix;
}
cbuffer SCENE : register(b1)
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
}
V_OUT main(V_IN input)
{
	V_OUT output = (V_OUT)0;
	// ensures translation is preserved during matrix multiply  
	float4 localH = float4(input.posL, 1);
		// move local space vertex from vertex buffer into world space.
		localH = mul(localH, worldMatrix);

	// TODO: Move into view space, then projection space
	localH = mul(localH, viewMatrix);
	localH = mul(localH, projectionMatrix);

	output.posL = input.posL;

	output.posH = localH;

	return output; // send projected vertex to the rasterizer stage
}

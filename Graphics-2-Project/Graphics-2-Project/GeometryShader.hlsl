struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;

};

struct GSInput
{
	float4 pos : POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;
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

[maxvertexcount(4)]
void main(
	point float4 input[1] : SV_POSITION,
	inout TriangleStream< GSOutput > output
	)
{
	GSOutput vertices[4] = 
	{
		{ (GSOutput)0 },
		{ (GSOutput)0 },
		{ (GSOutput)0 },
		{ (GSOutput)0 }
	};

	vertices[0].pos = float4(-100, 0, 100, 0);
	vertices[0].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	vertices[0].normal = float3(0, 1.0f, 0);
	vertices[0].uv = float2 (0, 0);


	vertices[1].pos = float4(100, 0, 100, 0);
	vertices[1].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	vertices[1].normal = float3(0, 1.0f, 0);
	vertices[1].uv = float2 (0, 0);


	vertices[2].pos = float4(-100, 0, -100, 0);
	vertices[2].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	vertices[2].normal = float3(0, 1.0f, 0);
	vertices[2].uv = float2 (0, 0);


	vertices[3].pos = float4(100, 0, -100, 0);
	vertices[3].color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	vertices[3].normal = float3(0, 1.0f, 0);
	vertices[3].uv = float2 (0, 0);

	for(unsigned int i = 0; i < 4; i++)
	{
		vertices[i].pos = mul(vertices[i].pos, viewMatrix);
		vertices[i].pos = mul(vertices[i].pos, projectionMatrix);
		vertices[i].normal = mul(vertices[i].normal, worldMatrix);
	}

	output.Append(vertices[0]);
	output.Append(vertices[1]);
	output.Append(vertices[2]);
	output.Append(vertices[3]);




}
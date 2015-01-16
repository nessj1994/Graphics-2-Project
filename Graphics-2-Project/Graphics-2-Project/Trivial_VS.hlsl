#pragma pack_matrix(row_major)

//struct INPUT_VERTEX
//{
//	float4 coordinate : POSITION;
//	float4 color : COLOR;
//};
//
//struct OUTPUT_VERTEX
//{
//	float4 colorOut : COLOR;
//	float4 projectedCoordinate : SV_POSITION;
//};
//
//// TODO: PART 3 STEP 2a
//cbuffer THIS_IS_VRAM : register( b0 )
//{
//	float4 constantColor;
//	float2 constantOffset;
//	float2 padding;
//};
//
//OUTPUT_VERTEX main( INPUT_VERTEX fromVertexBuffer )
//{
//	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;
//	sendToRasterizer.projectedCoordinate.w = 1;
//	
//	sendToRasterizer.projectedCoordinate.xy = fromVertexBuffer.coordinate.xy;
//		
//	// TODO : PART 4 STEP 4
//	/*sendToRasterizer.projectedCoordinate.xy += constantOffset;*/
//	
//	// TODO : PART 3 STEP 7
//	sendToRasterizer.colorOut = constantColor;
//	// END PART 3
//
//	return sendToRasterizer;
//}




struct V_IN
{
	float3 posL : POSITION;
	float4 color : COLOR;
	float3 normal : NORMAL;
	float2 uv : UV;
};
struct V_OUT
{
	float4 posH : SV_POSITION;
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
	output.normal = mul(input.normal, worldMatrix);
	output.normal = normalize(output.normal);
	output.posH = localH;

	output.color = input.color;
	
	return output; // send projected vertex to the rasterizer stage
}

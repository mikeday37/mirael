static float2 positions[3] = float2[](
	float2(0.0, -0.5),
	float2(0.5, 0.5),
	float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
	float3(1.0, 0.0, 0.0),
	float3(0.0, 1.0, 0.0),
	float3(0.0, 0.0, 1.0)
);

struct VertexData {
	float3 color;
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexData vertMain(uint vid : SV_VertexID) {
	VertexData data;
	data.sv_position = float4(positions[vid], 0.0, 1.0);
	data.color = colors[vid];
	return data;
}

[shader("fragment")]
float4 fragMain(VertexData data) {
	float3 color = data.color;
	return float4(color, 1.0);
}

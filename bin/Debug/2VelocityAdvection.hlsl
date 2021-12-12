#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	64
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	64
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
Texture3D<float4> velocityTexture				: register(t0);
RWTexture3D<float4> velocityFinal				: register(u0);


SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uint packUnorm4x8(float4 value)
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	uint w = uint(saturate(value.w) * 255.0f);

	return x | (y << 8u) | (z << 16u) | (w << 24u);
}

uniform uint2 texResolution;

uniform float timeSinceLast;
uniform float reciprocalDeltaX;
uniform float velocityDissipationConstant;
uniform float inkDissipationConstant;

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if(gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x && 
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y)
	{
		int3 idx3 = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);
		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		float4 velocity = velocityTexture.Load(idx4);

		float4 idxBackInTime = (idx4 - (timeSinceLast * reciprocalDeltaX * velocity));

		float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		velocityFinal[idx3] = v;
	}
}
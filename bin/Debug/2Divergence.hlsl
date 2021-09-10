#if 0
	***	threads_per_group_x	8
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	8
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	32
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	32
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
RWTexture3D<float3> divergenceTexture : register(u0);
Texture2D<float3> velocityRead : register(t0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float halfDeltaX;

[numthreads(8, 8, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, 0);

		float3 a = velocityRead.SampleLevel(TextureSampler, float2(idx.x - 1, idx.y), 0);
		float3 b = velocityRead.SampleLevel(TextureSampler, float2(idx.x + 1, idx.y), 0);
		float3 c = velocityRead.SampleLevel(TextureSampler, float2(idx.x, idx.y + 1), 0);
		float3 d = velocityRead.SampleLevel(TextureSampler, float2(idx.x, idx.y - 1), 0);

		divergenceTexture[idx].xyz = float3(((a.x - b.x) + (c.y - d.y)) * halfDeltaX, 0.0, 0.0);
			
			//float3(((a.x - b.x) + (c.z - d.z)) * halfDeltaX, 0, 0);
	}
}
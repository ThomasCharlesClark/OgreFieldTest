#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	512
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	512
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
RWTexture3D<float3> divergenceTexture : register(u0);
Texture3D<float3> velocityRead : register(t0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float halfDeltaX;

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		float3 a = velocityRead.SampleLevel(TextureSampler, float3(idx.x - 1, idx.y,	 idx.z) / width, 0);
		float3 b = velocityRead.SampleLevel(TextureSampler, float3(idx.x + 1, idx.y,	 idx.z) / width, 0);
		float3 c = velocityRead.SampleLevel(TextureSampler, float3(idx.x,	  idx.y - 1, idx.z) / width, 0);
		float3 d = velocityRead.SampleLevel(TextureSampler, float3(idx.x,	  idx.y + 1, idx.z) / width, 0);

		float3 div = float3(
			((a.x - b.x) + (c.y - d.y)) * halfDeltaX,
			0,
			0);
			/*((a.x - b.x) + (c.y - d.y)) * halfDeltaX,
			((a.x - b.x) + (c.y - d.y)) * halfDeltaX);*/

		divergenceTexture[idx] = div;
	}
}
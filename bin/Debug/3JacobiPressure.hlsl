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
RWTexture3D<float3> pressureTexture : register(u0);
Texture2D<float3> divergenceRead : register(t0);

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
	int3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, 0);

	float alpha = -halfDeltaX * halfDeltaX;
	float rBeta = 0.25;
	float beta = divergenceRead.Load(idx);

	float3 a = pressureTexture.Load(int3(idx.x - 1, idx.z, 0));
	float3 b = pressureTexture.Load(int3(idx.x + 1, idx.z, 0));
	float3 c = pressureTexture.Load(int3(idx.x, idx.z - 1, 0));
	float3 d = pressureTexture.Load(int3(idx.x, idx.z + 1, 0));

	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		for (int i = 0; i < 20; ++i) 
		{
			pressureTexture[idx].xyz = float3((a.x + b.x + c.x + d.x + alpha * beta) * rBeta, 0, 0);
		}
	}
}
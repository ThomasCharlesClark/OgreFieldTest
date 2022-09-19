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
Texture3D<float> vorticityTexture		: register(t0);
RWTexture3D<float3> velocityTexture		: register(u0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float halfDeltaX;
uniform float vorticityConfinementScale;
uniform float timeSinceLast;

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		float a = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x - 1, idx.y, idx.z) / width, 0);
		float b = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x + 1, idx.y, idx.z) / width, 0);
		float c = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y - 1, idx.z) / width, 0);
		float d = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y + 1, idx.z) / width, 0);

		float3 force = float3(abs(c) - abs(d), 0.0f, abs(b) - abs(a)) * halfDeltaX;

		force = normalize(force);

		force *= vorticityConfinementScale * vorticityTexture[idx] * float3(1, 0, -1);

		velocityTexture[idx] += force * timeSinceLast;
	}
}
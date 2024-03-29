#if 0
	***	threads_per_group_x	4
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	4
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	256
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	256
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
RWTexture3D<float4> velocityTexture : register(u0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float halfDeltaX;

[numthreads(4, 4, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1) {

		int3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float4 a = velocityTexture.Load(float4(idx.x - 1, idx.y, idx.z, 0));
		float4 b = velocityTexture.Load(float4(idx.x + 1, idx.y, idx.z, 0));
		float4 c = velocityTexture.Load(float4(idx.x, idx.y - 1, idx.z, 0));
		float4 d = velocityTexture.Load(float4(idx.x, idx.y + 1, idx.z, 0));

		float alpha = halfDeltaX * halfDeltaX / (/*0.998 ? * */ timeSinceLast);
		float rBeta = (1 / 4 + alpha);
		float4 beta = velocityTexture.Load(float4(idx.xyz, 0));

		float4 vVel = (a + b + c + d + alpha * beta) * rBeta;

		velocityTexture[idx] = vVel;
	}
}
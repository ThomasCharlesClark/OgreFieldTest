#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	8
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	8
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
RWTexture3D<float> inkTexFinal			: register(u0);
Texture3D<float> inkTexture				: register(t0);
Texture3D<float4> velocityTexture		: register(t1);

SamplerState TextureSampler
{
	Filter = ANISOTROPIC;
	AddressU = Clamp;
	AddressV = Clamp;
};

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
	/*if (gl_GlobalInvocationID.x >= 0 &&
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y >= 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1)
	{*/
		int3 idx3 = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		float width = texResolution.x;

		//float4 velocity = velocityTexture.Load(idx4);
		float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx4 / width, 0);

		//velocity.x = 0;

		//float3 idxBackInTime = (idx3 - velocity);
		//float3 idxBackInTime = (idx3 - (timeSinceLast * reciprocalDeltaX * velocity));
		float3 idxBackInTime = (idx3 - (timeSinceLast * velocity));

		float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//float i = inkTexture.Load(int4(idxBackInTime, 0));
		float i = inkTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		
		//inkTexFinal[idx3] = normalize(v.x);
		inkTexFinal[idx3] = i;
	//}
}
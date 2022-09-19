#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
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
RWTexture3D<float4> inkTextureFinal		: register(u0);
Texture3D<float4> velocityTexture		: register(t0);
Texture3D<float4> inkTexture			: register(t1);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

float4 unpackUnorm4x8(uint value)
{
	float4 retVal;
	retVal.x = float(value & 0xFF);
	retVal.y = float((value >> 8u) & 0xFF);
	retVal.z = float((value >> 16u) & 0xFF);
	retVal.w = float((value >> 24u) & 0xFF);

	return retVal * 0.0039215687f;
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
	if (gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx3 = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx3 / width, 0);

		//float4 velocity = velocityTexture.Load(int4(idx3, 0));

		//float3 idxBackInTime = (idx3 - 1 / (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		float3 idxBackInTime = (idx3 - (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		//float3 idxBackInTime = (idx3 - (reciprocalDeltaX * velocity.xyz));

		//float3 idxBackInTime = (idx3 - (velocity.xyz));

		// at this point in time, inkTexture should contain the previous
		// state of affairs
		//float4 i = inkTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		float4 i = inkTexture.Load(int4(idxBackInTime, 0));

		float4 i2 = inkTextureFinal.Load(int4(idxBackInTime, 0));

		//inkTexture[idxBackInTime] = float4(0, 0, 0, 1.0);

		inkTextureFinal[idx3] = i; // float4(i.xyz, 1.0);

		//inkTextureFinal[idxBackInTime] = float4(0, 0, 0, 1.0);
	}
}
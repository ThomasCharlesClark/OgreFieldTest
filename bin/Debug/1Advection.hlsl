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
RWTexture3D<float4> velocityFinal				: register(u0);
RWTexture3D<float4> inkFinal					: register(u1);

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
	if (gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1)
	{
		int3 idx = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);
		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		float4 velocity = velocityFinal.Load(idx4) * velocityDissipationConstant;

		float4 ink = inkFinal.Load(idx4);

		//velocity.y = velocity.z;

		//velocity.z = 0;

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity));
		
		float3 idxBackInTimeUV = idxBackInTime / width;

		float id = inkDissipationConstant;

		idx = idx - (velocity.xyz * timeSinceLast);

		float3 a = velocityFinal.Load(float4(idx.x - 1, idx.y, idx.z, 0)).xyz;
		float3 b = velocityFinal.Load(float4(idx.x + 1, idx.y, idx.z, 0)).xyz;
		float3 c = velocityFinal.Load(float4(idx.x, idx.y + 1, idx.z, 0)).xyz;
		float3 d = velocityFinal.Load(float4(idx.x, idx.y - 1, idx.z, 0)).xyz;

		float3 e = lerp(a, c, 0.5);
		float3 f = lerp(b, d, 0.5);

		float3 newVel = lerp(e, f, 0.5);

		velocityFinal[idxBackInTime] = float4(newVel.xyz, 0);

		float4 ai = inkFinal.Load(float4(idx.x - 1, idx.y, idx.z, 0));
		float4 bi = inkFinal.Load(float4(idx.x + 1, idx.y, idx.z, 0));
		float4 ci = inkFinal.Load(float4(idx.x, idx.y - 1, idx.z, 0));
		float4 di = inkFinal.Load(float4(idx.x, idx.y + 1, idx.z, 0));

		float4 ei = lerp(ai, ci, 0.5);
		float4 fi = lerp(bi, di, 0.5);

		float4 newInk = lerp(ei, fi, 0.5);

		inkFinal[idxBackInTime] = newInk;
	}
}
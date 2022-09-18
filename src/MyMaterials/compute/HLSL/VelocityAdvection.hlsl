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

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
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

		float4 velocity = velocityFinal.Load(idx4);

		//float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx4 / width, 0);

		velocity.y = velocity.z;
		velocity.z = 0;

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity));
		
		float3 idxBackInTimeUV = idxBackInTime / width;
		//float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTimeUV, 0);

		//float4 v = velocityFinal.Load(idxBackInTime);

		float id = inkDissipationConstant;

		idx = idx - (velocity.xyz * timeSinceLast);

		//float3 a = velocityTexture.SampleLevel(TextureSampler, float3(idx.x - 1, idx.y, idx.z) / width, 0);
		//float3 b = velocityTexture.SampleLevel(TextureSampler, float3(idx.x + 1, idx.y, idx.z) / width, 0);
		//float3 c = velocityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y + 1, idx.z) / width, 0);
		//float3 d = velocityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y - 1, idx.z) / width, 0);

		float3 a = velocityFinal.Load(float4(idx.x - 1, idx.y, idx.z, 0)).xyz;
		float3 b = velocityFinal.Load(float4(idx.x + 1, idx.y, idx.z, 0)).xyz;
		float3 c = velocityFinal.Load(float4(idx.x, idx.y + 1, idx.z, 0)).xyz;
		float3 d = velocityFinal.Load(float4(idx.x, idx.y - 1, idx.z, 0)).xyz;

		float3 e = lerp(a, c, 0.5);
		float3 f = lerp(b, d, 0.5);

		float3 newVel = lerp(e, f, 0.5);

		velocityFinal[idxBackInTime] = float4(newVel.xyz, 0);
	}
}
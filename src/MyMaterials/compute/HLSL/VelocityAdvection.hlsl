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
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		int3 idx3 = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);
		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		//float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx3 / width, 0);

		float4 velocity = velocityTexture.Load(idx4);

		float4 idxBackInTime = (idx4 - (timeSinceLast * reciprocalDeltaX * velocity));

		float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//float4 v = velocityTexture.Load(idxBackInTime);

		velocityFinal[idx3] = v;
	}
}
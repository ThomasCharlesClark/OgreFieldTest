RWTexture3D<float4> velocityWrite			: register(u0); // primaryVelocityTexture
Texture3D<float4> velocityRead				: register(t0); // secondaryVelocityTexture

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
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		float4 velocity = velocityRead.SampleLevel(TextureSampler, idx / width, 0);

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		//float4 v = float4(float3(velocityRead.SampleLevel(TextureSampler, idxBackInTime / width, 0).xyz) * velocityDissipationConstant, 0);

		float4 v = float4(float3(velocityRead.SampleLevel(TextureSampler, idxBackInTime / width, 0).xyz), 0);

		velocityWrite[idx] = v;
	}
}
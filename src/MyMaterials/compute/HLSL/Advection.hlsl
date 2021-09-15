RWTexture3D<float4> velocityWrite	: register(u0);
RWTexture3D<float4> inkWrite		: register(u1);
Texture3D<float4> velocityRead		: register(t0);
Texture3D<float4> inkRead			: register(t1);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

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

		float width = texResolution.x;

		float4 velocity = velocityRead.SampleLevel(TextureSampler, idx / width, 1.0);
		float4 ink = velocityRead.SampleLevel(TextureSampler, idx / width, 1.0);

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity.xyz)) / width;

		float4 v = float4(float3(velocityRead.SampleLevel(TextureSampler, idxBackInTime, 1.0).xyz) * velocityDissipationConstant, 1.0);
		float4 i = float4(float3(inkRead.SampleLevel(TextureSampler, idxBackInTime, 1.0).xyz), 1.0);

		velocityWrite[idx] = v;
		inkWrite[idx] = i * inkDissipationConstant;
	}
}
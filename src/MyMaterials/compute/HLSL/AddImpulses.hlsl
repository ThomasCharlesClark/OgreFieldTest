struct Particle
{
	float4 colour;
	float3 velocity;
	float pressure;
	float3 pressureGradient;
};

RWTexture3D<float4> velocityTexture : register(u0);
RWTexture3D<float4> inkTexture : register(u1);
RWStructuredBuffer<Particle> otherBuffer : register(u2);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float reciprocalDeltaX;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	float tsl = timeSinceLast * 2;

	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		velocityTexture[idx] += float4(otherBuffer[rwIdx].velocity, 1.0);
		inkTexture[idx] += otherBuffer[rwIdx].colour;
	}
}
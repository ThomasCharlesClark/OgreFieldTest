struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
};

RWStructuredBuffer<Particle> inputUavBuffer		: register(u0); // inputUavBuffer
RWTexture3D<float4> velocityTexture				: register(u1); // velocityTexture
RWTexture3D<float> inkTexture					: register(u2); // inkTexture
RWTexture3D<float> inkTextureSampler			: register(u3); // inkTextureSampler
RWTexture3D<float4> temporaryVelocity			: register(u4); // velocityTexture


SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;

[numthreads(@value(threads_per_group_x), @value(threads_per_group_y), @value(threads_per_group_z))]
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
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		int4 idx4 = int4(gl_GlobalInvocationID.xyz, 0);

		float4 velocity = float4(inputUavBuffer[rwIdx].velocity, 0.0);

		float width = texResolution.x;

		velocityTexture[gl_GlobalInvocationID] += velocity;
		inkTexture[gl_GlobalInvocationID] += inputUavBuffer[rwIdx].ink;
	}
}
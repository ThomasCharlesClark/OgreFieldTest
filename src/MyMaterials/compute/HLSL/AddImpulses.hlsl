struct Particle
{
	float4 colour;
	float3 velocity;
	float pressure;
	float3 pressureGradient;
};

RWTexture3D<float4> velocityWrite				: register(u0);	// primaryVelocityTexture
RWTexture3D<float4> inkWrite					: register(u1); // primaryInkTexture
RWStructuredBuffer<Particle> handInputBuffer	: register(u2); // inputUavBuffer (leapMotion input)

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		velocityWrite[gl_GlobalInvocationID] = float4(handInputBuffer[rwIdx].velocity, 1.0);
		inkWrite[gl_GlobalInvocationID] = handInputBuffer[rwIdx].colour;
	}
}
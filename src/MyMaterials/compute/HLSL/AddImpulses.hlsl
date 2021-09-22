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
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		velocityWrite[idx] = float4(handInputBuffer[rwIdx].velocity, 1.0);

		//inkWrite[gl_LocalInvocationID] = handInputBuffer[rwIdx].colour;
		//inkWrite[gl_LocalInvocationID] += handInputBuffer[rwIdx].colour;

		inkWrite[gl_GlobalInvocationID] += handInputBuffer[rwIdx].colour;
	}
}
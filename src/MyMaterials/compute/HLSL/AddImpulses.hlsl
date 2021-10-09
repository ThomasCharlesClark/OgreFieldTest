struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
};

RWTexture3D<float4> velocityWrite				: register(u0);	// primaryVelocityTexture
RWTexture3D<float4> inkWrite					: register(u1); // primaryInkTexture
RWStructuredBuffer<Particle> handInputBuffer	: register(u2); // inputUavBuffer (leapMotion input)
RWTexture3D<float> inkTemp						: register(u3); // tempInkTexture (temporary uav)

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

		float4 velocity = float4(handInputBuffer[rwIdx].velocity, 1.0);

		velocityWrite[gl_GlobalInvocationID] = velocity;
		// being additive here might be wrong - velocity doesn't need to be additive; why should ink?
		inkWrite[gl_GlobalInvocationID] += handInputBuffer[rwIdx].colour;
		inkTemp[gl_GlobalInvocationID] += handInputBuffer[rwIdx].ink;
		
		//handInputBuffer[rwIdx].colour = float4(0.0450033244f, 0.1421513113f, 0.4302441212f, 1.0f);
		//handInputBuffer[rwIdx].colour = float4(0, 0, 0, 1.0);
		//handInputBuffer[rwIdx].ink = 0.0;
	}
}
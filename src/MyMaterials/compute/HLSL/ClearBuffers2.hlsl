struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWStructuredBuffer<Particle> handInputBuffer	: register(u0); // inputUavBuffer (leapMotion input)
RWTexture3D<float4> inkPrimary					: register(u1); // primaryInkTexture

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

		//handInputBuffer[rwIdx].ink = 0.0;
		//handInputBuffer[rwIdx].colour = float4(0, 0, 0, 1);

		inkPrimary[gl_GlobalInvocationID] = float4(inkPrimary[gl_GlobalInvocationID].xyz * 0.28, 1.0);

		//inkPrimary[gl_GlobalInvocationID] = float4(0, 0, 0, 1);
	}
}
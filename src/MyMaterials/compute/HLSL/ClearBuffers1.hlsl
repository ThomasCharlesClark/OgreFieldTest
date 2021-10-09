struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
};

RWStructuredBuffer<Particle> handInputBuffer	: register(u0); // inputUavBuffer (leapMotion input)
RWTexture3D<float4> inkSecondary				: register(u1); // secondaryInkTexture

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

		//handInputBuffer[rwIdx].ink = 0.0;
		//handInputBuffer[rwIdx].colour = float4(0, 0, 0, 1.0);

		inkSecondary[gl_GlobalInvocationID] = float4(inkSecondary[gl_GlobalInvocationID].xyz * 0.28, 1.0);
		//inkSecondary[gl_GlobalInvocationID] = inkSecondary[gl_GlobalInvocationID] * 0.28;

		inkSecondary[gl_GlobalInvocationID] = float4(0, 0, 0, 1.0);
	}
}
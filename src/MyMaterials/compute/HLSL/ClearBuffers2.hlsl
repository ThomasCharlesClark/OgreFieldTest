struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWTexture3D<float> inkTexture					: register(u0);
RWTexture3D<float> inkTexFinal				: register(u1);
RWTexture3D<float4> velocityTexture				: register(u2);
RWTexture3D<float4> velocityFinal				: register(u3);

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
		
		inkTexture[gl_GlobalInvocationID] *= 0.992;
		inkTexFinal[gl_GlobalInvocationID] *= 0.992;

		//inkTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		//inkTexFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 0);

		velocityFinal[gl_GlobalInvocationID] *= 0.996;
		velocityTexture[gl_GlobalInvocationID] *= 0.996;

		//velocityFinal[gl_GlobalInvocationID] *= 1.0002;
		//velocityTexture[gl_GlobalInvocationID] *= 1.0002;

		//velocityFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		//velocityTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 0);

	}
}
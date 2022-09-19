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
RWTexture3D<float4> divergenceTexture				: register(u4);
RWTexture3D<float4> pressureTexture				: register(u5);

uniform uint2 texResolution;
uniform float timeSinceLast;
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
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		
		float4 zero = float4(0, 0, 0, 0);

		//inkTexture[gl_GlobalInvocationID] *= 0.992;
		//inkTexFinal[gl_GlobalInvocationID] *= 0.02;

		//inkTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		//inkTexFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		 
		//velocityFinal[gl_GlobalInvocationID] *= 0.04;
		//velocityTexture[gl_GlobalInvocationID] *= 0.996;

		inkTexFinal[gl_GlobalInvocationID] *= inkDissipationConstant;

		//divergenceTexture[gl_GlobalInvocationID] *= 0.96;

		divergenceTexture[gl_GlobalInvocationID] = zero;
		pressureTexture[gl_GlobalInvocationID] = zero;

		//velocityFinal[gl_GlobalInvocationID] *= 1.0002;
		//velocityTexture[gl_GlobalInvocationID] *= 1.0002;

		//velocityFinal[gl_GlobalInvocationID] *= velocityDissipationConstant;
		//velocityFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 0);

	}
}
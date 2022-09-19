struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWTexture3D<float> inkTexture					: register(u0);
RWTexture3D<float4> velocityTexture				: register(u1);
RWTexture3D<float4> divergenceTexture			: register(u2);
RWTexture3D<float4> pressureTexture				: register(u3);

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float velocityDissipationConstant; 
uniform float inkDissipationConstant;
uniform float pressureDissipationConstant;

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

		//velocityTexture[gl_GlobalInvocationID] *= velocityDissipationConstant;
		
		//inkTexture[gl_GlobalInvocationID] *= inkDissipationConstant;
		
		//divergenceTexture[gl_GlobalInvocationID] *= velocityDissipationConstant;

		//pressureTexture[gl_GlobalInvocationID] *= pressureDissipationConstant;

		//pressureTexture[gl_GlobalInvocationID] = zero;
	}
}
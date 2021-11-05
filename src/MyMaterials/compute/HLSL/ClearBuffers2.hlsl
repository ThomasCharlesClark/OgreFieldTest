struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWStructuredBuffer<Particle> handInputBuffer	: register(u0);
RWTexture3D<float4> inkTexture					: register(u1);
RWTexture3D<float4> velocityFinal				: register(u2); // velocityFinal

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
		//handInputBuffer[rwIdx].colour *= 0.98;// float4(0, 0, 0, 1);
		//handInputBuffer[rwIdx].velocity *= 0.98;// float3(0, 0, 0);

		//inkTextureFinal[gl_GlobalInvocationID] = float4(inkTextureFinal[gl_GlobalInvocationID].xyz * 0.97, 1.0);

		inkTexture[gl_GlobalInvocationID] *= 0.998; // float4(0, 0, 0, 1);
		//inkTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 1);

		velocityFinal[gl_GlobalInvocationID] *= 0.98;

		//velocityFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 1);
	}
}
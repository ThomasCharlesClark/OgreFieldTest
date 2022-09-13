RWTexture3D<float3> velocityTexture			: register(u0);
RWTexture3D<float3> pressureTexture			: register(u1);
RWTexture3D<float> inkTexture				: register(u2);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float reciprocalDeltaX;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x == 0) {
		float3 neighbourIdx = float3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.x == texResolution.x - 1) {

		float3 neighbourIdx = float3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.y == 0) {
		float3 neighbourIdx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y + 1, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.y == texResolution.y - 1) {

		float3 neighbourIdx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y - 1, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 *   velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
}
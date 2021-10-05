struct Particle 
{
	float4 colour;
	float3 velocity;
	float pressure;
	float3 pressureGradient;
};

RWStructuredBuffer<uint> pixelBuffer : register(u0);
RWTexture3D<float4> velocityRead : register(u1);
RWTexture3D<float4> inkRead : register(u2);

uniform uint2 texResolution;

uniform float timeSinceLast;

uint packUnorm4x8( float4 value )
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	uint w = uint(saturate(value.w) * 255.0f);
	
	return x | (y << 8u) | (z << 16u ) | (w << 24u);
}

float4 unpackUnorm4x8(uint value)
{
	float4 retVal;
	retVal.x = float(value & 0xFF);
	retVal.y = float((value >> 8u) & 0xFF);
	retVal.z = float((value >> 16u) & 0xFF);
	retVal.w = float((value >> 24u) & 0xFF);
	 
	return retVal * 0.0039215687f;
}

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		//float4 i = inkRead.Load(int4(gl_GlobalInvocationID, 1));
		float4 i = inkRead.Load(int4(gl_GlobalInvocationID, 1));

		float4 v = velocityRead.Load(int4(gl_GlobalInvocationID, 1));
		
		//pixelBuffer[idx] = packUnorm4x8(i);

		//pixelBuffer[idx] = packUnorm4x8(float4(i.xyz, 1.0f));

		pixelBuffer[idx] = packUnorm4x8(float4(v.xyz + i.xyz, 1.0f));
	}
}
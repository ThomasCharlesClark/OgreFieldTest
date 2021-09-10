#if 0
	***	threads_per_group_x	8
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	8
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	32
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	32
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
struct Particle 
{
	float4 colour;
	float3 velocity;
	float pressure;
	float3 pressureGradient;
};

RWStructuredBuffer<uint> pixelBuffer : register(u0);
RWStructuredBuffer<Particle> otherBuffer : register(u1);
RWTexture3D<float3> velocityTexture : register(u2);
//RWTexture3D<float3> pressureTexture : register(u3);
RWTexture3D<float3> pressureGradientTexture : register(u4);
//RWTexture3D<float3> divergenceTexture : register(u5);
RWTexture3D<float3> inkTexture : register(u6);

uniform uint2 texResolution;

uniform float timeSinceLast;

uint packUnorm4x8( float4 value )
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	//uint z = uint(value.z);
	uint w = uint(saturate(value.w) * 255.0f);
	
	return x | (y << 8u) | (z << 16u ) | (w << 24u);
}

[numthreads(8, 8, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		/*uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		pixelBuffer[idx] = packUnorm4x8(float4(float2(gl_LocalInvocationID.xy) / 8.0f, otherBuffer[0].colour.x,
			1.0f));*/

		//	//1.0f - ((1.0f / texResolution.x) * gl_GlobalInvocationID.x)));
		//	//(1.0f / texResolution.x) * gl_GlobalInvocationID.x ));

		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float2 coord = float2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);

		float3 v = velocityTexture.Load(int3(coord, 0));

		coord -= (v * timeSinceLast).xy;

		uint idxBackInTime = coord.y * texResolution.x + coord.x;

		float3 i = inkTexture.Load(int3(coord, 0));
		//float3 p = pressureTexture.Load(int3(coord, 0));
		//float3 div = divergenceTexture.Load(int3(coord, 0));
		float3 vel = velocityTexture.Load(int3(coord, 0));
		float4 c = otherBuffer[idxBackInTime].colour;

		//i += div;
		//i += p;

		//c.g = v.r;

		/*
			Right, this ugly pink result is FUCKING HUGELY IMPORTANT AND AWESOME.

			This means that I have loaded a whole bunch of textures onto the GPU, as well as one actual Structured UAV Buffer
			I have set two of these textures (velocityTexture and inkTexture) with some random BS data just for the sake of 
			testing.

			Below I am reading colour value.r from velocityTexture.r

			but when I specified the random BS data to fill the velocityTexture with, I set the R component to be 0.0!!!

			and yet redness has occurred!

			this is because of the ultimately supercool factorino that I have MY OWN COMPUTE JOB! IT IS COMPUTING!

			the Advection compute job is currently just overwriting velocityTexture.r with inkTexture.r

			but it's PROOF that it's DOING SOMETHING ADSFGHJFSDGKSDFGLKLDFAGSJLDFKJGKDF!!!
		*/

		/*c.r = v.r;
		c.b += v.g;*/


		//float3 vBit = velocityTexture.Load(int3(coord, 0));

		//pixelBuffer[idx] = packUnorm4x8(float4(vBit, 1.0f));


		pixelBuffer[idx] = packUnorm4x8(float4(i, 1.0f));
	}
}

/*
	So what's next? I need the ability to sample the otherBuffer... at some position which is not the
	current global invocation x,y
*/

float3 PerformNormalMapping(float3 normal, float3 tangent,
	float3 bitangent, float3 normalMapNormal)
{
	float3 redistributedNormalMap = 2 * normalMapNormal - 1.0f;
	float3x3 tbn = float3x3(tangent, bitangent, normal);
	return normalize(mul(redistributedNormalMap, tbn));
}

float2 ParallaxMapping(float2 currentTexCoords, float heightScale,
	Texture2D<float4> displacementTexture, float3 viewDir,
	float3 normal, float3 tangent, float3 bitangent,
	SamplerState wrapSampler)
{
	float3x3 tbn = transpose(float3x3(tangent, bitangent, normal));
	float3 tangentSpaceViewDir = normalize(mul(viewDir, tbn));

	int minSampleCount = 4;
	int maxSampleCount = 8;

	float2 maxParallaxOffset = -tangentSpaceViewDir.xy * heightScale / tangentSpaceViewDir.z;
	int sampleCount = (int)lerp(maxSampleCount, minSampleCount, dot(-viewDir, normal));
	float zStep = 1.0f / (float)sampleCount;
	float2 texStep = maxParallaxOffset * zStep;

	int sampleIndex = 0;
	float2 currTexOffset = 0;
	float2 prevTexOffset = 0;
	float2 finalTexOffset = 0;
	float currRayZ = 1.0f - zStep;
	float prevRayZ = 1.0f;
	float currHeight = 0.0f;
	float prevHeight = 0.0f;

	while (sampleIndex < sampleCount + 1)
	{
		currHeight = displacementTexture.Sample(wrapSampler,
			currentTexCoords + currTexOffset).r;

		if (currHeight > currRayZ)
		{
			float t = (prevHeight - prevRayZ) /
				(prevHeight - currHeight + currRayZ - prevRayZ);
			finalTexOffset = prevTexOffset + t * texStep;
			break;
		}
		else
		{
			++sampleIndex;
			prevTexOffset = currTexOffset;
			prevRayZ = currRayZ;
			prevHeight = currHeight;
			currTexOffset += texStep;

			currRayZ -= zStep;
		}
	}

	return currentTexCoords + finalTexOffset;
}

float2 ParallaxMapping(float2 currentTexCoords, float heightScale,
	Texture2D<float4> displacementTexture, float3 viewDir,
	float3 normal, float3 tangent, float3 bitangent,
	SamplerState wrapSampler, float offset)
{
	float3x3 tbn = transpose(float3x3(tangent, bitangent, normal));
	float3 tangentSpaceViewDir = normalize(mul(viewDir, tbn));

	int minSampleCount = 8;
	int maxSampleCount = 64;

	float2 maxParallaxOffset = -tangentSpaceViewDir.xy * heightScale / tangentSpaceViewDir.z;
	int sampleCount = (int)lerp(maxSampleCount, minSampleCount, dot(-viewDir, normal));
	float zStep = 1.0f / (float)sampleCount;
	float2 texStep = maxParallaxOffset * zStep;

	int sampleIndex = 0;
	float2 currTexOffset = 0;
	float2 prevTexOffset = 0;
	float2 finalTexOffset = 0;
	float currRayZ = 1.0f - zStep;
	float prevRayZ = 1.0f;
	float currHeight = 0.0f;
	float prevHeight = 0.0f;

	while (sampleIndex < sampleCount + 1)
	{
		currHeight = displacementTexture.Sample(wrapSampler,
			currentTexCoords + currTexOffset).r;
		currHeight *= offset;

		if (currHeight > currRayZ)
		{
			float t = (prevHeight - prevRayZ) /
				(prevHeight - currHeight + currRayZ - prevRayZ);
			finalTexOffset = prevTexOffset + t * texStep;
			break;
		}
		else
		{
			++sampleIndex;
			prevTexOffset = currTexOffset;
			prevRayZ = currRayZ;
			prevHeight = currHeight;
			currTexOffset += texStep;

			currRayZ -= zStep;
		}
	}

	return currentTexCoords + finalTexOffset;
}

float2 CalculateSphereUV(float3 sphereVector)
{
	float theta = atan2(sphereVector.x, sphereVector.z);
	float phi = acos(sphereVector.y);
	float rawU = theta / (2 * 3.14f);

	float u = 1 - (rawU + 0.5f);
	float v = phi / 3.14f;

	return float2(u, v);
}
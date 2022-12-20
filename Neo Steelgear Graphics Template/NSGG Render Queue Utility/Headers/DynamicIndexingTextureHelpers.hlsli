template<typename T>
Texture2D<T> GetTexture2D(unsigned int heapIndex, bool nonUniformIndex)
{
	return GetDynamicallyIndexedResource<Texture2D<T>>(heapIndex, nonUniformIndex);
}

template<typename T>
T GetTexture2DSample(SamplerState textureSampler, float2 textureCoords,
	unsigned int heapIndex, bool nonUniformIndex)
{
	Texture2D<T> toSample = GetTexture2D<T>(heapIndex, nonUniformIndex);
	return toSample.Sample(textureSampler, textureCoords);
}

template<typename T>
T GetTexture2DValue(uint2 textureCoords, unsigned int heapIndex,
	bool nonUniformIndex)
{
	return GetTexture2D<T>(heapIndex, nonUniformIndex)[textureCoords];
}

// RWTexture2D

template<typename T>
RWTexture2D<T> GetRWTexture2D(unsigned int heapIndex, bool nonUniformIndex)
{
	return GetDynamicallyIndexedResource<RWTexture2D<T>>(heapIndex, nonUniformIndex);
}

template<typename T>
T GetRWTexture2DSample(SamplerState textureSampler, float2 textureCoords,
	unsigned int heapIndex, bool nonUniformIndex)
{
	RWTexture2D<T> toSample = GetRWTexture2D<T>(heapIndex, nonUniformIndex);
	return toSample.Sample(textureSampler, textureCoords);
}

template<typename T>
T GetRWTexture2DValue(uint2 textureCoords, unsigned int heapIndex,
	bool nonUniformIndex)
{
	return GetRWTexture2D<T>(heapIndex, nonUniformIndex)[textureCoords];
}

template<typename T>
void SetRWTexture2DValue(T value, uint2 textureCoords, unsigned int heapIndex,
	bool nonUniformIndex)
{
	GetRWTexture2D<T>(heapIndex, nonUniformIndex)[textureCoords] = value;
}
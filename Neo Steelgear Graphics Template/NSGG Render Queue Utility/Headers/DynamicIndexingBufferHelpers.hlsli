#include "DynamicIndexingHelpers.hlsli"

template<typename T>
StructuredBuffer<T> GetStructuredBuffer(unsigned int heapIndex, 
	bool nonUniformIndex) 
{
	return GetDynamicallyIndexedResource<StructuredBuffer<T> >(heapIndex, 
		nonUniformIndex);
}

template<typename T>
T GetStructuredBufferElement(unsigned int elementIndex,
	unsigned int heapIndex, bool nonUniformIndex)
{
	return GetStructuredBuffer<T>(heapIndex, nonUniformIndex)[elementIndex];
}

// RWStructuredBuffer

template<typename T>
RWStructuredBuffer<T> GetRWStructuredBuffer(unsigned int heapIndex,
	bool nonUniformIndex)
{
	return GetDynamicallyIndexedResource<RWStructuredBuffer<T> >(heapIndex,
		nonUniformIndex);
}

template<typename T>
T GetRWStructuredBufferElement(unsigned int elementIndex,
	unsigned int heapIndex, bool nonUniformIndex)
{
	return GetRWStructuredBuffer<T>(heapIndex, nonUniformIndex)[elementIndex];
}

template<typename T>
void SetRWStructuredBufferElement(unsigned int elementIndex, T value,
	unsigned int heapIndex, bool nonUniformIndex)
{
	GetRWStructuredBuffer<T>(heapIndex, nonUniformIndex)[elementIndex] = value;
}
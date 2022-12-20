template<typename T>
T GetDynamicallyIndexedResource(unsigned int heapIndex, bool nonUniformIndex)
{
	if (nonUniformIndex == true)
	{
		T toReturn = ResourceDescriptorHeap[NonUniformResourceIndex(heapIndex)];
		return toReturn;
	}
	else
	{
		T toReturn = ResourceDescriptorHeap[heapIndex];
		return toReturn;
	}
}
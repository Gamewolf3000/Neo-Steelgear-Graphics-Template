#pragma once

#include <d3d12.h>
#include <dxgi.h>
#include <vector>
#include <utility>

#include <HeapAllocatorGPU.h>

#include "LocalResourceDesc.h"
#include "ResourceIdentifiers.h"

struct LocalResourceHandle
{
	ID3D12Resource* resource = nullptr;
	size_t offset = size_t(-1);
	size_t size = 0;
};

struct LocalAllocatorMemoryInfo
{
	size_t initialSize = 0;
	size_t expansionSize = 0;
};

class InnerLocalAllocator
{
private:

	struct BufferEntry
	{
		size_t offset = size_t(-1);
		size_t size = 0;
	};

	ID3D12Device* device = nullptr;
	LocalAllocatorMemoryInfo memoryInfo;
	HeapAllocatorGPU* allocator = nullptr;
	HeapChunk heapChunk;
	ID3D12Resource* resource = nullptr;
	std::vector<BufferEntry> buffers;
	size_t currentSize = 0;
	size_t currentOffset = 0;
	unsigned char* mappedPtr = nullptr;

	void AllocateResource();
	void AllocateHeapChunk(size_t minimumSize);

public:
	InnerLocalAllocator() = default;
	virtual ~InnerLocalAllocator();
	InnerLocalAllocator(const InnerLocalAllocator& other) = delete;
	InnerLocalAllocator& operator=(const InnerLocalAllocator& other) = delete;
	InnerLocalAllocator(InnerLocalAllocator&& other) noexcept;
	InnerLocalAllocator& operator=(InnerLocalAllocator&& other) noexcept;

	void Initialize(ID3D12Device* deviceToUse, 
		const LocalAllocatorMemoryInfo& allocatorMemoryInfo,
		HeapAllocatorGPU* allocatorToUse);
	void Reset();

	void SetMinimumFrameDataSize(size_t minimumSizeNeeded);

	LocalResourceIndex AllocateBuffer(const LocalResourceDesc& desc);

	LocalResourceHandle GetHandle(const LocalResourceIndex& index) const;
	size_t GetCurrentSize() const;
	D3D12_RESOURCE_BARRIER GetInitializationBarrier();

	void UpdateData(void* dataPtr, size_t dataSize);
};
#pragma once

#include <FrameObject.h>

#include "InnerLocalAllocator.h"

template<FrameType Frames>
class LocalResourceAllocator : FrameBased<Frames>
{
private:
	FrameObject<InnerLocalAllocator, Frames> allocators;
	std::vector<unsigned char> data;

public:
	LocalResourceAllocator() = default;
	~LocalResourceAllocator() = default;
	LocalResourceAllocator(const LocalResourceAllocator& other) = delete;
	LocalResourceAllocator& operator=(const LocalResourceAllocator& other) = delete;
	LocalResourceAllocator(LocalResourceAllocator&& other) noexcept = default;
	LocalResourceAllocator& operator=(LocalResourceAllocator&& other) noexcept = default;

	void Initialize(ID3D12Device* deviceToUse, const LocalAllocatorMemoryInfo& memoryInfo,
		HeapAllocatorGPU* allocatorToUse);
	void SetMinimumFrameDataSize(size_t minimumSizeNeeded);

	LocalResourceIndex CreateLocalResource(const LocalResourceDesc& desc);
	void SetLocalResourceData(const LocalResourceIndex& index, const void* dataPtr);

	LocalResourceHandle GetLocalResourceHandle(const LocalResourceIndex& index) const;
	D3D12_RESOURCE_BARRIER GetInitializationBarrier();

	void UploadData();

	void SwapFrame() override;
};

template<FrameType Frames>
void LocalResourceAllocator<Frames>::Initialize(ID3D12Device* deviceToUse,
	const LocalAllocatorMemoryInfo& memoryInfo, HeapAllocatorGPU* allocatorToUse)
{
	allocators.Initialize< InnerLocalAllocator, ID3D12Device*,
		const LocalAllocatorMemoryInfo&, HeapAllocatorGPU*>(
			&InnerLocalAllocator::Initialize, deviceToUse, memoryInfo, allocatorToUse);
	data.resize(allocators.Active().GetCurrentSize());
}

template<FrameType Frames>
void LocalResourceAllocator<Frames>::SetMinimumFrameDataSize(size_t minimumSizeNeeded)
{
	allocators.Active().SetMinimumFrameDataSize(minimumSizeNeeded);
	data.resize(allocators.Active().GetCurrentSize());
}

template<FrameType Frames>
LocalResourceIndex LocalResourceAllocator<Frames>::CreateLocalResource(
	const LocalResourceDesc& desc)
{
	return allocators.Active().AllocateBuffer(desc);
}

template<FrameType Frames>
void LocalResourceAllocator<Frames>::SetLocalResourceData(
	const LocalResourceIndex& index, const void* dataPtr)
{
	LocalResourceHandle handle = allocators.Active().GetHandle(index);
	memcpy(&data[handle.offset], dataPtr, handle.size);
}

template<FrameType Frames>
LocalResourceHandle LocalResourceAllocator<Frames>::GetLocalResourceHandle(
	const LocalResourceIndex& index) const
{
	return allocators.Active().GetHandle(index);
}

template<FrameType Frames>
D3D12_RESOURCE_BARRIER LocalResourceAllocator<Frames>::GetInitializationBarrier()
{
	return allocators.Active().GetInitializationBarrier();
}

template<FrameType Frames>
void LocalResourceAllocator<Frames>::UploadData()
{
	allocators.Active().UpdateData(data.data(), data.size());
}

template<FrameType Frames>
void LocalResourceAllocator<Frames>::SwapFrame()
{
	allocators.SwapFrame();
	allocators.Active().Reset();
}
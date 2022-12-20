#pragma once

#include <optional>

#include <HeapHelper.h>
#include <HeapAllocatorGPU.h>
#include <D3DPtr.h>
#include <DescriptorAllocator.h>

#include "TransientResourceDesc.h"
#include "ResourceIdentifiers.h"

struct TransientResourceHandle
{
	ID3D12Resource* resource = nullptr;
};

struct TransientAllocatorMemoryInfo
{
	size_t initialSize = 0;
	size_t expansionSize = 0;
	size_t nrOfStartingSlotsShaderBindable = 100;
	size_t nrOfStartingSlotsRTV = 20;
	size_t nrOfStartingSlotsDSV = 20;
};

class TransientResourceAllocator
{
private:
	struct TransientResourceIdentifier
	{
		size_t chunkIndex = size_t(-1);
		size_t internalIndex = size_t(-1);
	};

	struct AllocatedResource
	{
		D3DPtr<ID3D12Resource> resource;
		bool hasRTV = false;
		std::optional<D3D12_CLEAR_VALUE> optimalClearValue = std::nullopt;
	};

	struct MemoryChunk
	{
		HeapChunk heapChunk;
		std::vector<AllocatedResource> resources;
		size_t currentOffset = 0;
	};

	ID3D12Device* device = nullptr;

	TransientAllocatorMemoryInfo memoryInfo;
	HeapAllocatorGPU* allocator = nullptr;
	std::vector<MemoryChunk> memoryChunks;
	std::vector<TransientResourceIdentifier> identifiers;

	DescriptorAllocator shaderBindableDescriptors;
	DescriptorAllocator rtvDescriptors;
	DescriptorAllocator dsvDescriptors;

	ID3D12Resource* AllocateResource(const TransientResourceDesc& desc, 
		ID3D12Heap* heap, size_t heapOffset, D3D12_RESOURCE_STATES initialState);
	void AllocateHeapChunk(size_t minimumSize);
	size_t GetAvailableMemoryChunk(size_t allocationSize);

public:
	TransientResourceAllocator() = default;
	~TransientResourceAllocator() = default;
	TransientResourceAllocator(const TransientResourceAllocator& other) = delete;
	TransientResourceAllocator& operator=(const TransientResourceAllocator& other) = delete;
	TransientResourceAllocator(TransientResourceAllocator&& other) noexcept = default;
	TransientResourceAllocator& operator=(TransientResourceAllocator&& other) noexcept = default;

	void Initialize(ID3D12Device* deviceToUse, 
		const TransientAllocatorMemoryInfo& allocatorMemoryInfo, 
		HeapAllocatorGPU* allocatorToUse);
	void Clear();
	
	TransientResourceIndex CreateTransientResource(const TransientResourceDesc& desc,
		D3D12_RESOURCE_STATES initialState);

	TransientResourceViewIndex CreateSRV(const TransientResourceIndex& index,
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> desc = std::nullopt);
	TransientResourceViewIndex CreateUAV(const TransientResourceIndex& index,
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> desc = std::nullopt);
	TransientResourceViewIndex CreateRTV(const TransientResourceIndex& index,
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> desc = std::nullopt);
	TransientResourceViewIndex CreateDSV(const TransientResourceIndex& index,
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> desc = std::nullopt);

	TransientResourceHandle GetTransientResourceHandle(const TransientResourceIndex& index) const;
	size_t GetShanderBindableCount() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderBindableHandle(const TransientResourceViewIndex& index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(const TransientResourceViewIndex& index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(const TransientResourceViewIndex& index) const;

	void AddInitializationBarriers(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo) const;
	void DiscardRenderTargets(ID3D12GraphicsCommandList* list);
	void ClearDepthStencils(ID3D12GraphicsCommandList* list);
};
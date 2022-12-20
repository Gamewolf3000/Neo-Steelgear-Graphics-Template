#pragma once

#include <unordered_map>
#include <stdexcept>
#include <vector>

#include <d3d12.h>

#include <FrameBased.h>
#include <ResourceComponent.h>
#include <D3DPtr.h>

#include "CategoryIdentifiers.h"

template<FrameType Frames>
class ManagedDescriptorHeap : public FrameBased<Frames>
{
private:
	struct ComponentOffset
	{
		size_t cbvOffset = size_t(-1);
		size_t srvOffset = size_t(-1);
		size_t uavOffset = size_t(-1);
	};

	std::unordered_map<CategoryIdentifier, ComponentOffset> componentOffsets;
	size_t globalDescriptorsOffset = 0;

	ID3D12Device* device = nullptr;
	D3DPtr<ID3D12DescriptorHeap> cpuHeap;
	D3DPtr<ID3D12DescriptorHeap> gpuHeap;
	unsigned int descriptorsPerFrame = 0;
	size_t currentOffset = 0;
	unsigned int descriptorSize = 0;

	void ThrowIfFailed(HRESULT hr, const std::exception& exception);

	void CreateDescriptorHeaps(unsigned int nrOfDescriptors);
	void StoreDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle,
		UINT nrOfComponents);

	struct ReplacedDescriptorHeap
	{
		ID3D12DescriptorHeap* heap = nullptr;
		FrameType framesNeededAlive = Frames;
	};

	std::vector<ReplacedDescriptorHeap> replacedDescriptors;

public:
	ManagedDescriptorHeap() = default;
	~ManagedDescriptorHeap() = default;
	ManagedDescriptorHeap(const ManagedDescriptorHeap& other) = delete;
	ManagedDescriptorHeap& operator=(const ManagedDescriptorHeap& other) = delete;
	ManagedDescriptorHeap(ManagedDescriptorHeap&& other) noexcept = default;
	ManagedDescriptorHeap& operator=(ManagedDescriptorHeap&& other) noexcept = default;

	void Initialize(ID3D12Device* deviceToUse,
		unsigned int startDescriptorsPerFrame);

	void AddCategoryDescriptors(const CategoryIdentifier& identifier,
		const ResourceComponent& component);
	size_t GetCategoryHeapOffset(const CategoryIdentifier& identifier,
		ViewType viewType) const;

	void AddGlobalDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE startHandle,
		size_t nrOfDescriptors);
	size_t GetGlobalOffset() const;

	void UploadCurrentFrameHeap();
	ID3D12DescriptorHeap* GetShaderVisibleHeap() const;

	void SwapFrame() override;
};

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::ThrowIfFailed(
	HRESULT hr, const std::exception& exception)
{
	if (FAILED(hr))
		throw exception;
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::CreateDescriptorHeaps(
	unsigned int nrOfDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = nrOfDescriptors;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&cpuHeap));
	ThrowIfFailed(hr, std::runtime_error("Failed to create cpu descriptor heap"));

	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = nrOfDescriptors * Frames;
	hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gpuHeap));
	ThrowIfFailed(hr, std::runtime_error("Failed to create gpu descriptor heap"));
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::StoreDescriptors(
	D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle, UINT nrOfComponents)
{
	if (descriptorsPerFrame - currentOffset < nrOfComponents)
	{
		D3DPtr<ID3D12DescriptorHeap> temp = std::move(cpuHeap); // We need to copy already stored descriptors
		replacedDescriptors.push_back({ std::move(gpuHeap), Frames }); // Store for deletion when safe
		descriptorsPerFrame *= 2;
		CreateDescriptorHeaps(descriptorsPerFrame);
		device->CopyDescriptorsSimple(currentOffset, cpuHeap->GetCPUDescriptorHandleForHeapStart(),
			temp->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	auto destinationHandle = cpuHeap->GetCPUDescriptorHandleForHeapStart();
	destinationHandle.ptr += currentOffset * descriptorSize;
	device->CopyDescriptorsSimple(nrOfComponents, destinationHandle,
		sourceHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	currentOffset += nrOfComponents;
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::Initialize(
	ID3D12Device* deviceToUse, unsigned int startDescriptorsPerFrame)
{
	replacedDescriptors.reserve(5); // Should stop unnecessary dynamic expansions during reasonable runtime operations
	device = deviceToUse;
	descriptorsPerFrame = startDescriptorsPerFrame;
	descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CreateDescriptorHeaps(startDescriptorsPerFrame);
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::AddCategoryDescriptors(
	const CategoryIdentifier& identifier, const ResourceComponent& component)
{
	ComponentOffset toStore;
	UINT nrOfComponents = static_cast<UINT>(component.NrOfDescriptors());
	size_t heapStartCurrentFrame = descriptorsPerFrame * this->activeFrame;

	if (component.HasDescriptorsOfType(ViewType::CBV))
	{
		toStore.cbvOffset = currentOffset + heapStartCurrentFrame;
		StoreDescriptors(component.GetDescriptorHeapCBV(), nrOfComponents);
	}

	if (component.HasDescriptorsOfType(ViewType::SRV))
	{
		toStore.srvOffset = currentOffset + heapStartCurrentFrame;
		StoreDescriptors(component.GetDescriptorHeapSRV(), nrOfComponents);
	}

	if (component.HasDescriptorsOfType(ViewType::UAV))
	{
		toStore.uavOffset = currentOffset + heapStartCurrentFrame;
		StoreDescriptors(component.GetDescriptorHeapUAV(), nrOfComponents);
	}

	componentOffsets[identifier] = toStore;
}

template<FrameType Frames>
inline size_t ManagedDescriptorHeap<Frames>::GetCategoryHeapOffset(
	const CategoryIdentifier& identifier, ViewType viewType) const
{
	const auto& offsets = componentOffsets.at(identifier);

	switch (viewType)
	{
	case ViewType::CBV:
		return offsets.cbvOffset;
	case ViewType::SRV:
		return offsets.srvOffset;
	case ViewType::UAV:
		return offsets.uavOffset;
	default:
		throw std::runtime_error("Attempting to get heap offset of incorrect type");
	}
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::AddGlobalDescriptors(
	D3D12_CPU_DESCRIPTOR_HANDLE startHandle, size_t nrOfDescriptors)
{
	size_t heapStartCurrentFrame = descriptorsPerFrame * this->activeFrame;
	globalDescriptorsOffset = currentOffset + heapStartCurrentFrame;
	StoreDescriptors(startHandle, nrOfDescriptors);
}

template<FrameType Frames>
inline size_t ManagedDescriptorHeap<Frames>::GetGlobalOffset() const
{
	return globalDescriptorsOffset;
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::UploadCurrentFrameHeap()
{
	auto destination = gpuHeap->GetCPUDescriptorHandleForHeapStart();
	destination.ptr += this->activeFrame * descriptorsPerFrame * descriptorSize;
	auto source = cpuHeap->GetCPUDescriptorHandleForHeapStart();
	device->CopyDescriptorsSimple(descriptorsPerFrame, destination, source,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

template<FrameType Frames>
inline ID3D12DescriptorHeap* 
ManagedDescriptorHeap<Frames>::GetShaderVisibleHeap() const
{
	return gpuHeap;
}

template<FrameType Frames>
inline void ManagedDescriptorHeap<Frames>::SwapFrame()
{
	FrameBased<Frames>::SwapFrame();
	currentOffset = 0;
	globalDescriptorsOffset = 0;

	for (size_t i = 0; i < replacedDescriptors.size(); ++i)
	{
		--replacedDescriptors[i].framesNeededAlive;
		if (replacedDescriptors[i].framesNeededAlive == 0)
		{
			std::swap(replacedDescriptors[i], replacedDescriptors.back());
			--i;
			replacedDescriptors.pop_back();
		}
	}
}

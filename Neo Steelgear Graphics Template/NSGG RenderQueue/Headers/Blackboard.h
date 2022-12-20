#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <cstdint>

#include <d3d12.h>

#include <FrameBased.h>
#include <FrameObject.h>
#include <MultiHeapAllocatorGPU.h>

#include "ResourceIdentifiers.h"
#include "LocalResourceAllocator.h"
#include "TransientResourceAllocator.h"

template<FrameType Frames>
class Blackboard : FrameBased<Frames>
{
private:
    MultiHeapAllocatorGPU allocator;
    LocalResourceAllocator<Frames> localAllocator;
    FrameObject<TransientResourceAllocator, Frames> transientAllocators;

public:
    Blackboard() = default;
    ~Blackboard() = default;
    Blackboard(const Blackboard& other) = delete;
    Blackboard& operator=(const Blackboard& other) = delete;
    Blackboard(Blackboard&& other) = default;
    Blackboard& operator=(Blackboard&& other) = default;

    void Initialize(ID3D12Device* deviceToUse, 
        const LocalAllocatorMemoryInfo& localAllocatorMemoryInfo,
        const TransientAllocatorMemoryInfo& transientAllocatorMemoryInfo);

    TransientResourceIndex CreateTransientResource(
        const TransientResourceDesc& desc, D3D12_RESOURCE_STATES initialState);
    LocalResourceIndex CreateLocalResource(const LocalResourceDesc& desc);

    ViewIdentifier CreateSRV(const TransientResourceIndex& index,
        const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>& desc = std::nullopt);
    ViewIdentifier CreateUAV(const TransientResourceIndex& index,
        const std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>& desc = std::nullopt);
    ViewIdentifier CreateRTV(const TransientResourceIndex& index,
        const std::optional<D3D12_RENDER_TARGET_VIEW_DESC>& desc = std::nullopt);
    ViewIdentifier CreateDSV(const TransientResourceIndex& index,
        const std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>& desc = std::nullopt);

    void SetLocalFrameMemoryRequirement(size_t memoryNeededForFrame);
    void SetLocalResourceData(const LocalResourceIndex& index, const void* data);
    void UploadLocalData();

    TransientResourceHandle GetTransientResourceHandle(const TransientResourceIndex& index) const;
    LocalResourceHandle GetLocalResource(const LocalResourceIndex& index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetTransientResourceRTV(const ViewIdentifier& identifier) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetTransientResourceDSV(const ViewIdentifier& identifier) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetTransientShaderBindableHandle() const;
    size_t GetNrTransientShaderBindables() const;

    void GetInitializeBarriers(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo);
    void DiscardAndClearResources(ID3D12GraphicsCommandList* list);
    void SwapFrame() override;
};

template<FrameType Frames>
inline void Blackboard<Frames>::Initialize(ID3D12Device* deviceToUse,
    const LocalAllocatorMemoryInfo& localAllocatorMemoryInfo,
    const TransientAllocatorMemoryInfo& transientAllocatorMemoryInfo)
{
    allocator.Initialize(deviceToUse);
    localAllocator.Initialize(deviceToUse, localAllocatorMemoryInfo, &allocator);
    transientAllocators.Initialize<TransientResourceAllocator, ID3D12Device*,
        const TransientAllocatorMemoryInfo&, HeapAllocatorGPU*>(&TransientResourceAllocator::Initialize,
            deviceToUse, transientAllocatorMemoryInfo, static_cast<HeapAllocatorGPU*>(&allocator));
}

template<FrameType Frames>
TransientResourceIndex Blackboard<Frames>::CreateTransientResource(
    const TransientResourceDesc& desc, D3D12_RESOURCE_STATES initialState)
{
    return transientAllocators.Active().CreateTransientResource(desc, initialState);
}

template<FrameType Frames>
LocalResourceIndex Blackboard<Frames>::CreateLocalResource(const LocalResourceDesc& desc)
{
    return localAllocator.CreateLocalResource(desc);
}

template<FrameType Frames>
ViewIdentifier Blackboard<Frames>::CreateSRV(const TransientResourceIndex& index,
    const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC>& desc)
{
    ViewIdentifier toReturn;
    toReturn.internalIndex = transientAllocators.Active().CreateSRV(index, desc);
    toReturn.type = FrameViewType::SHADER_BINDABLE;

    return toReturn;
}

template<FrameType Frames>
ViewIdentifier Blackboard<Frames>::CreateUAV(const TransientResourceIndex& index,
    const std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC>& desc)
{
    ViewIdentifier toReturn;
    toReturn.internalIndex = transientAllocators.Active().CreateUAV(index, desc);
    toReturn.type = FrameViewType::SHADER_BINDABLE;

    return toReturn;
}

template<FrameType Frames>
ViewIdentifier Blackboard<Frames>::CreateRTV(const TransientResourceIndex& index,
    const std::optional<D3D12_RENDER_TARGET_VIEW_DESC>& desc)
{
    ViewIdentifier toReturn;
    toReturn.internalIndex = transientAllocators.Active().CreateRTV(index, desc);
    toReturn.type = FrameViewType::RTV;

    return toReturn;
}

template<FrameType Frames>
ViewIdentifier Blackboard<Frames>::CreateDSV(const TransientResourceIndex& index,
    const std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>& desc)
{
    ViewIdentifier toReturn;
    toReturn.internalIndex = transientAllocators.Active().CreateDSV(index, desc);
    toReturn.type = FrameViewType::DSV;

    return toReturn;
}

template<FrameType Frames>
void Blackboard<Frames>::SetLocalFrameMemoryRequirement(size_t memoryNeededForFrame)
{
    localAllocator.SetMinimumFrameDataSize(memoryNeededForFrame);
}

template<FrameType Frames>
void Blackboard<Frames>::SetLocalResourceData(const LocalResourceIndex& index, const void* data)
{
    localAllocator.SetLocalResourceData(index, data);
}

template<FrameType Frames>
inline void Blackboard<Frames>::UploadLocalData()
{
    localAllocator.UploadData();
}

template<FrameType Frames>
TransientResourceHandle Blackboard<Frames>::GetTransientResourceHandle(
    const TransientResourceIndex& index) const
{
    return transientAllocators.Active().GetTransientResourceHandle(index);
}

template<FrameType Frames>
LocalResourceHandle Blackboard<Frames>::GetLocalResource(const LocalResourceIndex& index) const
{
    return localAllocator.GetLocalResourceHandle(index);
}

template<FrameType Frames>
D3D12_CPU_DESCRIPTOR_HANDLE Blackboard<Frames>::GetTransientResourceRTV(
    const ViewIdentifier& identifier) const
{
    return transientAllocators.Active().GetRTV(identifier.internalIndex);
}

template<FrameType Frames>
D3D12_CPU_DESCRIPTOR_HANDLE Blackboard<Frames>::GetTransientResourceDSV(
    const ViewIdentifier& identifier) const
{
    return transientAllocators.Active().GetDSV(identifier.internalIndex);
}

template<FrameType Frames>
inline D3D12_CPU_DESCRIPTOR_HANDLE Blackboard<Frames>::GetTransientShaderBindableHandle() const
{
    return transientAllocators.Active().GetShaderBindableHandle(0);
}

template<FrameType Frames>
inline size_t Blackboard<Frames>::GetNrTransientShaderBindables() const
{
    return transientAllocators.Active().GetShanderBindableCount();
}

template<FrameType Frames>
inline void Blackboard<Frames>::GetInitializeBarriers(
    std::vector<D3D12_RESOURCE_BARRIER>& toAddTo)
{
    transientAllocators.Active().AddInitializationBarriers(toAddTo);
    toAddTo.push_back(localAllocator.GetInitializationBarrier());
}

template<FrameType Frames>
inline void Blackboard<Frames>::DiscardAndClearResources(ID3D12GraphicsCommandList* list)
{
    transientAllocators.Active().DiscardRenderTargets(list);
    transientAllocators.Active().ClearDepthStencils(list);
}

template<FrameType Frames>
inline void Blackboard<Frames>::SwapFrame()
{
    localAllocator.SwapFrame();
    transientAllocators.SwapFrame();
    transientAllocators.Active().Clear();
}
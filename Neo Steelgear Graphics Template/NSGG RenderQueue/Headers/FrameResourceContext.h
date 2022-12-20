#pragma once

#include <optional>

#include <FrameBased.h>

#include "ManagedDescriptorHeap.h"
#include "ManagedResourceCategories.h"
#include "Blackboard.h"
#include "ResourceIdentifiers.h"
#include "CategoryIdentifiers.h"

template<FrameType Frames>
class FrameResourceContext
{
private:
	ManagedDescriptorHeap<Frames>* descriptorHeap = nullptr;
	ManagedResourceCategories<Frames>* resourceCategories = nullptr;
	Blackboard<Frames>* blackboard = nullptr;

public:
	FrameResourceContext() = default;
	~FrameResourceContext() = default;
	FrameResourceContext(const FrameResourceContext& other) = delete;
	FrameResourceContext& operator=(const FrameResourceContext& other) = delete;
	FrameResourceContext(FrameResourceContext&& other) noexcept = default;
	FrameResourceContext& operator=(FrameResourceContext&& other) noexcept = default;

	void Initialize(ManagedDescriptorHeap<Frames>* descriptorHeap,
		ManagedResourceCategories<Frames>* resourceCategories,
		Blackboard<Frames>* blackboard);

	void TransitionCategoryResources(const CategoryIdentifier& identifier,
		std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
		D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

	void SetLocalResourceData(const LocalResourceIndex& index, const void* data);

	TransientResourceHandle GetTransientResource(const TransientResourceIndex& index) const;
	LocalResourceHandle GetLocalResource(const LocalResourceIndex& index) const;
	CategoryResourceHandle GetCategoryResource(const CategoryResourceIdentifier& identifier) const;
	size_t GetCategoryDescriptorStart(const CategoryIdentifier& identifier, ViewType viewType) const;
	size_t GetCategoryResourceDescriptor(const CategoryResourceIdentifier& identifier, ViewType viewType) const;
	size_t GetTransientDescriptorOffset(const ViewIdentifier& viewIdentifier) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetTransientResourceRTV(const ViewIdentifier& viewIdentifier) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetTransientResourceDSV(const ViewIdentifier& viewIdentifier) const;
};

template<FrameType Frames>
void FrameResourceContext<Frames>::Initialize(
	ManagedDescriptorHeap<Frames>* descriptorHeapToUse,
	ManagedResourceCategories<Frames>* resourceCategoriesToUse,
	Blackboard<Frames>* blackboardToUse)
{
	descriptorHeap = descriptorHeapToUse;
	resourceCategories = resourceCategoriesToUse;
	blackboard = blackboardToUse;
}

template<FrameType Frames>
void FrameResourceContext<Frames>::TransitionCategoryResources(
	const CategoryIdentifier& identifier, std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
	D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	resourceCategories->TransitionCategoryState(identifier, toAddTo, stateAfter, stateBefore);
}

template<FrameType Frames>
void FrameResourceContext<Frames>::SetLocalResourceData(
	const LocalResourceIndex& index, const void* data)
{
	blackboard->SetLocalResourceData(index, data);
}

template<FrameType Frames>
TransientResourceHandle FrameResourceContext<Frames>::GetTransientResource(
	const TransientResourceIndex& index) const
{
	return blackboard->GetTransientResourceHandle(index);
}

template<FrameType Frames>
LocalResourceHandle FrameResourceContext<Frames>::GetLocalResource(
	const LocalResourceIndex& index) const
{
	return blackboard->GetLocalResource(index);
}

template<FrameType Frames>
CategoryResourceHandle FrameResourceContext<Frames>::GetCategoryResource(
	const CategoryResourceIdentifier& identifier) const
{
	return resourceCategories->GetResourceHandle(identifier);
}

template<FrameType Frames>
size_t FrameResourceContext<Frames>::GetCategoryDescriptorStart(
	const CategoryIdentifier& identifier, ViewType viewType) const
{
	return descriptorHeap->GetCategoryHeapOffset(identifier, viewType);
}

template<FrameType Frames>
inline size_t FrameResourceContext<Frames>::GetCategoryResourceDescriptor(
	const CategoryResourceIdentifier& identifier, ViewType viewType) const
{
	size_t toReturn = GetCategoryDescriptorStart(
		identifier.categoryIdentifier, viewType);
	toReturn += identifier.internalIndex.descriptorIndex;

	return toReturn;
}

template<FrameType Frames>
size_t FrameResourceContext<Frames>::GetTransientDescriptorOffset(
	const ViewIdentifier& viewIdentifier) const
{
	size_t toReturn = descriptorHeap->GetGlobalOffset();
	toReturn += viewIdentifier.internalIndex;

	return toReturn;
}

template<FrameType Frames>
D3D12_CPU_DESCRIPTOR_HANDLE FrameResourceContext<Frames>::GetTransientResourceRTV(
	const ViewIdentifier& viewIdentifier) const
{
	return blackboard->GetTransientResourceRTV(viewIdentifier);
}

template<FrameType Frames>
D3D12_CPU_DESCRIPTOR_HANDLE FrameResourceContext<Frames>::GetTransientResourceDSV(
	const ViewIdentifier& viewIdentifier) const
{
	return blackboard->GetTransientResourceDSV(viewIdentifier);
}
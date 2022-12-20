#pragma once

#include <vector>
#include <stdexcept>

#include <d3d12.h>

#include "FrameResourceIdentifier.h"
#include "FrameResourceContext.h"

class FrameResourceBarrier
{
private:
	D3D12_RESOURCE_BARRIER_TYPE type;

	union
	{
		struct Transition
		{
			FrameResourceIdentifier identifier =
				FrameResourceIdentifier(TransientResourceIndex(-1));
			D3D12_RESOURCE_STATES stateBefore;
			D3D12_RESOURCE_STATES stateAfter;
		} transition;

		struct Aliasing
		{
			FrameResourceIdentifier identifierBefore =
				FrameResourceIdentifier(TransientResourceIndex(-1));
			FrameResourceIdentifier identifierAfter =
				FrameResourceIdentifier(TransientResourceIndex(-1));
		} aliasing = Aliasing();

		struct UAV
		{
			FrameResourceIdentifier identifier =
				FrameResourceIdentifier(TransientResourceIndex(-1));
		} uav;

	} data;

	template<FrameType Frames>
	void AddBarriersTransition(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
		FrameResourceContext<Frames>& context) const;

	template<FrameType Frames>
	void AddBarriersAliasing(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
		FrameResourceContext<Frames>& context) const;

	template<FrameType Frames>
	void AddBarriersUAV(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
		FrameResourceContext<Frames>& context) const;

public:
	FrameResourceBarrier() = default;
	~FrameResourceBarrier() = default;
	FrameResourceBarrier(const FrameResourceBarrier& other) = delete;
	FrameResourceBarrier& operator=(const FrameResourceBarrier& other) = delete;
	FrameResourceBarrier(FrameResourceBarrier&& other) noexcept = default;
	FrameResourceBarrier& operator=(FrameResourceBarrier&& other) noexcept = default;

	void InitializeAsTransition(const FrameResourceIdentifier& identifier,
		D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
	void InitializeAsAliasing(const FrameResourceIdentifier& identifierBefore,
		const FrameResourceIdentifier& identifierAfter);
	void InitializeAsUAV(const FrameResourceIdentifier& identifier);

	void MergeTransitionAfterState(D3D12_RESOURCE_STATES stateToMerge);

	template<FrameType Frames>
	void AddBarriers(std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
		FrameResourceContext<Frames>& context) const;
};

template<FrameType Frames>
void FrameResourceBarrier::AddBarriersTransition(
	std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
	FrameResourceContext<Frames>& context) const
{
	if (data.transition.identifier.origin == FrameResourceOrigin::TRANSIENT)
	{
		D3D12_RESOURCE_BARRIER toAdd;
		toAdd.Type = type;
		toAdd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		auto handle = context.GetTransientResource(
			data.transition.identifier.identifier.transient);
		toAdd.Transition.pResource = handle.resource;
		toAdd.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		toAdd.Transition.StateBefore = data.transition.stateBefore;
		toAdd.Transition.StateAfter = data.transition.stateAfter;
		toAddTo.push_back(toAdd);
	}
	else
	{
		context.TransitionCategoryResources(
			data.transition.identifier.identifier.category, toAddTo,
			data.transition.stateBefore, data.transition.stateAfter);
	}
}

template<FrameType Frames>
void FrameResourceBarrier::AddBarriersAliasing(
	std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
	FrameResourceContext<Frames>& context) const
{
	if (data.uav.identifier.origin == FrameResourceOrigin::CATEGORY)
	{
		throw std::runtime_error("Category aliasing barriers are not supported!");
	}

	D3D12_RESOURCE_BARRIER toAdd;
	toAdd.Type = type;
	toAdd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	auto beforeHandle = context.GetTransientResource(
		data.aliasing.identifierBefore.identifier.transient);
	auto afterHandle = context.GetTransientResource(
		data.aliasing.identifierAfter.identifier.transient);
	toAdd.Aliasing.pResourceBefore = beforeHandle.resource;
	toAdd.Aliasing.pResourceAfter = afterHandle.resource;
	toAddTo.push_back(toAdd);
}

template<FrameType Frames>
void FrameResourceBarrier::AddBarriersUAV(
	std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
	FrameResourceContext<Frames>& context) const
{
	if (data.uav.identifier.origin == FrameResourceOrigin::TRANSIENT)
	{
		D3D12_RESOURCE_BARRIER toAdd;
		toAdd.Type = type;
		toAdd.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		auto handle = context.GetTransientResource(
			data.uav.identifier.identifier.transient);
		toAdd.UAV.pResource = handle.resource;
		toAddTo.push_back(toAdd);
	}
	else
	{
		throw std::runtime_error("Category UAV barriers are not yet supported!");
	}
}

template<FrameType Frames>
void FrameResourceBarrier::AddBarriers(
	std::vector<D3D12_RESOURCE_BARRIER>& toAddTo,
	FrameResourceContext<Frames>& context) const
{
	switch (type)
	{
	case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
		AddBarriersTransition(toAddTo, context);
		break;
	case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
		AddBarriersAliasing(toAddTo, context);
		break;
	case D3D12_RESOURCE_BARRIER_TYPE_UAV:
		AddBarriersUAV(toAddTo, context);
		break;
	default:
		throw std::runtime_error("Unknown barrier type in frame resource barrier");
		break;
	}
}
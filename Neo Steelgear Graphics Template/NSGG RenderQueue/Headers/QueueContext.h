#pragma once

#include <vector>
#include <unordered_map>

#include <d3d12.h>

#include <FrameBased.h>

#include "RenderQueue.h"
#include "ResourceIdentifiers.h"
#include "FrameResource.h"
#include "QueueJob.h"
#include "EnqueuedJob.h"
#include "CategoryIdentifiers.h"

template<FrameType Frames>
class QueueContext
{
private:
	RenderQueue<Frames>* renderQueue = nullptr;

	struct QueueResource
	{
		FrameResource resource;
		size_t jobIndexOfLastStateChange = size_t(-1);
		size_t barrierIndexOfLastBarrier = size_t(-1);
		size_t jobIndexOfLastAccess = size_t(-1);

		QueueResource(const FrameResourceIdentifier& identifier) :
			resource(identifier)
		{

		}
	};

	std::vector<QueueResource> transientResources;
	std::unordered_map<CategoryIdentifier, QueueResource> componentResources;

	std::vector<EnqueuedJob<Frames>> jobs;

	void HandleRequest(QueueResource& resource, D3D12_RESOURCE_STATES neededState);

	void AddPostExecutionCategoryBarriers();

public:
	QueueContext() = default;
	~QueueContext() = default;
	QueueContext(const QueueContext& other) = delete;
	QueueContext& operator=(const QueueContext& other) = delete;
	QueueContext(QueueContext&& other) noexcept = default;
	QueueContext& operator=(QueueContext&& other) noexcept = default;

	void Initialize(RenderQueue<Frames>* renderQueueToUse);

	TransientResourceIndex CreateTransientResource(D3D12_RESOURCE_STATES initialState);

	void RequestTransientResource(const TransientResourceIndex& index,
		D3D12_RESOURCE_STATES neededState);
	void RequestCategoryResource(const CategoryIdentifier& identifier,
		D3D12_RESOURCE_STATES neededState);

	void AddJobToQueue(QueueJob<Frames>* job);
	void FinalizeQueue(TransientResourceIndex endTextureIndex);
	void ClearQueue();
};

template<FrameType Frames>
void QueueContext<Frames>::HandleRequest(
	QueueResource& resource, D3D12_RESOURCE_STATES neededState)
{
	std::optional<FrameResourceBarrier> neededBarrier =
		resource.resource.UpdateState(neededState);

	if (neededBarrier.has_value())
	{
		resource.barrierIndexOfLastBarrier = jobs.back().AddBarrier(std::move(neededBarrier.value()));
		resource.jobIndexOfLastStateChange = jobs.size() - 1;
	}
	else if (resource.jobIndexOfLastStateChange != size_t(-1))
	{
		auto& lastJob = jobs[resource.jobIndexOfLastStateChange];
		FrameResourceBarrier& lastBarrier = 
			lastJob.GetBarrier(resource.barrierIndexOfLastBarrier);
		lastBarrier.MergeTransitionAfterState(neededState);
	}

	resource.jobIndexOfLastAccess = jobs.size() - 1;
}

template<FrameType Frames>
inline void QueueContext<Frames>::AddPostExecutionCategoryBarriers()
{
	for (const auto& mapPair : componentResources)
	{
		// If a category resource was transitioned manually, 
		// or if a texture was promoted to a write state, 
		// we need to transition it back to common.
		// Transition to common may seem weird, 
		// but that way it can promote as needed at the start, AND, 
		// another queue setup can assume a category resource starts as common,
		// which should be the initial case in all cases for category resources.

		bool transitionNeeded =
			mapPair.second.jobIndexOfLastStateChange != size_t(-1);
		transitionNeeded |= mapPair.second.resource.IsInWriteState() &&
			mapPair.first.type != CategoryType::BUFFER;

		if (transitionNeeded)
		{
			FrameResourceBarrier toAdd;
			toAdd.InitializeAsTransition(mapPair.first,
				mapPair.second.resource.GetCurrentState(),
				D3D12_RESOURCE_STATE_COMMON);
			renderQueue->postExecutionBarriers.push_back(std::move(toAdd));
		}
	}
}

template<FrameType Frames>
inline void QueueContext<Frames>::Initialize(
	RenderQueue<Frames>* renderQueueToUse)
{
	renderQueue = renderQueueToUse;
}

template<FrameType Frames>
inline TransientResourceIndex QueueContext<Frames>::CreateTransientResource(
	D3D12_RESOURCE_STATES initialState)
{
	FrameResourceIdentifier identifier(transientResources.size());

	QueueResource queueResource(identifier);
	queueResource.resource.UpdateState(initialState);
	transientResources.push_back(std::move(queueResource));

	return transientResources.size() - 1;
}

template<FrameType Frames>
inline void QueueContext<Frames>::RequestTransientResource(
	const TransientResourceIndex& index, D3D12_RESOURCE_STATES neededState)
{
	HandleRequest(transientResources[index], neededState);
}

template<FrameType Frames>
inline void QueueContext<Frames>::RequestCategoryResource(
	const CategoryIdentifier& identifier, D3D12_RESOURCE_STATES neededState)
{
	if (componentResources.find(identifier) == componentResources.end())
	{
		componentResources.emplace(identifier, identifier);
	}

	HandleRequest(componentResources.at(identifier), neededState);
}

template<FrameType Frames>
inline void QueueContext<Frames>::AddJobToQueue(QueueJob<Frames>* job)
{
	EnqueuedJob<Frames> toAdd;
	toAdd.Initialize(job);
	jobs.push_back(std::move(toAdd));

	job->SetupQueue(*this);
}

template<FrameType Frames>
inline void QueueContext<Frames>::FinalizeQueue(
	TransientResourceIndex endTextureIndex)
{
	renderQueue->transientResources.clear();
	renderQueue->transientResources.reserve(transientResources.size());

	for (size_t i = 0; i < transientResources.size(); ++i)
	{
		renderQueue->transientResources.push_back({ transientResources[i].resource.GetInitialState() });
	}

	renderQueue->jobs = std::move(jobs);
	renderQueue->endTextureIndex = endTextureIndex;

	std::optional<FrameResourceBarrier> endTextureTransition =
		transientResources[endTextureIndex].resource.UpdateState(
			D3D12_RESOURCE_STATE_COPY_SOURCE);

	if (endTextureTransition.has_value() == true)
	{
		renderQueue->postExecutionBarriers.push_back(
			std::move(endTextureTransition.value()));
	}

	AddPostExecutionCategoryBarriers();
}

template<FrameType Frames>
inline void QueueContext<Frames>::ClearQueue()
{
	transientResources.clear();
	componentResources.clear();
	jobs.clear();

	renderQueue->transientResources.clear();
	renderQueue->jobs.clear();
}

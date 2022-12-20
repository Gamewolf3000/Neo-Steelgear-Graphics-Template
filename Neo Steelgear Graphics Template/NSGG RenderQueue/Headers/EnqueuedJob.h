#pragma once

#include <vector>

#include "QueueJob.h"
#include "FrameResourceBarrier.h"
#include "FrameResourceContext.h"

template<FrameType Frames>
class EnqueuedJob
{
private:
	QueueJob<Frames>* job;
	std::vector<FrameResourceBarrier> barriers;

public:
	EnqueuedJob() = default;
	~EnqueuedJob() = default;
	EnqueuedJob(const EnqueuedJob& other) = delete;
	EnqueuedJob& operator=(const EnqueuedJob& other) = delete;
	EnqueuedJob(EnqueuedJob&& other) noexcept = default;
	EnqueuedJob& operator=(EnqueuedJob&& other) noexcept = default;

	void Initialize(QueueJob<Frames>* jobToStore);

	size_t AddBarrier(FrameResourceBarrier&& barrier);
	FrameResourceBarrier& GetBarrier(size_t index);

	void ProcessJob(ID3D12GraphicsCommandList* list, 
		std::vector<D3D12_RESOURCE_BARRIER>& barrierVector,
		FrameResourceContext<Frames>& context);

	QueueJob<Frames>* GetQueueJob();
};

template<FrameType Frames>
inline void EnqueuedJob<Frames>::Initialize(QueueJob<Frames>* jobToStore)
{
	job = jobToStore;
}

template<FrameType Frames>
size_t EnqueuedJob<Frames>::AddBarrier(FrameResourceBarrier&& barrier)
{
	barriers.push_back(std::move(barrier));
	return barriers.size() - 1;
}

template<FrameType Frames>
inline FrameResourceBarrier& EnqueuedJob<Frames>::GetBarrier(size_t index)
{
	return barriers[index];
}

template<FrameType Frames>
inline QueueJob<Frames>* EnqueuedJob<Frames>::GetQueueJob()
{
	return job;
}

template<FrameType Frames>
inline void EnqueuedJob<Frames>::ProcessJob(ID3D12GraphicsCommandList* list,
	std::vector<D3D12_RESOURCE_BARRIER>& barrierVector,
	FrameResourceContext<Frames>& context)
{
	barrierVector.clear();
	barrierVector.reserve(barriers.size());
	
	for (const auto& barrier : barriers)
	{
		barrier.AddBarriers(barrierVector, context);
	}

	if (barrierVector.size() != 0)
	{
		list->ResourceBarrier(barrierVector.size(), barrierVector.data());
	}

	job->ExecuteFrame(list, context);
}
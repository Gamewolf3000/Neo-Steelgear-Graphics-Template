#pragma once

#include <vector>
#include <cstdint>

#include <entt.hpp>
#include <FrameBased.h>

#include "TransientResourceDesc.h"
#include "EnqueuedJob.h"
#include "FramePreparationContext.h"
#include "FrameSetupContext.h"
#include "Blackboard.h"
#include "RenderQueueTimerCPU.h"
#include "RenderQueueTimerGPU.h"
#include "ImguiContext.h"

template<FrameType Frames>
class RenderQueue
{
private:
	friend class QueueContext<Frames>;

	struct TransientResource
	{
		D3D12_RESOURCE_STATES initialState;
	};

	std::vector<TransientResource> transientResources;

	std::vector<EnqueuedJob<Frames>> jobs;
	std::vector<FrameResourceBarrier> postExecutionBarriers;

	TransientResourceIndex endTextureIndex = TransientResourceIndex(-1);

	FrameSetupContext setupContext;

	void PrepareBatch(size_t startJobIndex, size_t nrOfJobsToProcess,
		const entt::registry& frameRegistry,
		const FramePreparationContext<Frames>& context,
		size_t batchIndex, RenderQueueTimerCPU& cpuTimer);
	void ExecuteBatch(size_t startJobIndex, size_t nrOfJobsToProcess,
		ID3D12GraphicsCommandList* list, FrameResourceContext<Frames>& context,
		size_t batchIndex, RenderQueueTimerCPU& cpuTimer,
		RenderQueueTimerGPU<Frames>& gpuTimer);

public:
	RenderQueue() = default;
	~RenderQueue() = default;
	RenderQueue(const RenderQueue& other) = delete;
	RenderQueue& operator=(const RenderQueue& other) = delete;
	RenderQueue(RenderQueue&& other) noexcept = default;
	RenderQueue& operator=(RenderQueue&& other) noexcept = default;

	void PrepareFrame(const entt::registry& frameRegistry,
		std::uint8_t nrOfPartitions,
		const FramePreparationContext<Frames>& context,
		RenderQueueTimerCPU& cpuTimer);

	void SetResourceInfo(
		const std::vector<std::pair<TransientResourceIndex, TransientResourceDesc>>& globalDescs);
	void SetupTransientResources(Blackboard<Frames>& blackboard);

	void ExecuteJobs(const std::vector<ID3D12GraphicsCommandList*> lists,
		FrameResourceContext<Frames>& context, RenderQueueTimerCPU& cpuTimer,
		RenderQueueTimerGPU<Frames>& gpuTimer);

	void PerformImguiOperations(const FrameTimesCPU& cpuTimes,
		const FrameTimesGPU& gpuTimes, ImguiContext& imguiContext);

	size_t GetNrOfJobs() const;
	FrameSetupContext& GetFrameSetupContext();
	TransientResourceIndex GetEndTextureIndex() const;
	const std::vector<FrameResourceBarrier>& GetPostExecutionBarriers() const;
};

template<FrameType Frames>
void RenderQueue<Frames>::PrepareBatch(
	size_t startJobIndex, size_t nrOfJobsToProcess,
	const entt::registry& frameRegistry,
	const FramePreparationContext<Frames>& context, size_t batchIndex,
	RenderQueueTimerCPU& cpuTimer)
{
	auto batchStartPoint = cpuTimer.GetCurrentTimePoint();
	for (size_t i = 0; i < nrOfJobsToProcess; ++i)
	{
		auto jobStartPoint = cpuTimer.GetCurrentTimePoint();
		jobs[i + startJobIndex].GetQueueJob()->PrepareFrame(
			frameRegistry, context);
		cpuTimer.MarkJobPreparation(i + startJobIndex, jobStartPoint);
	}
	cpuTimer.MarkBatchPreparation(batchIndex, batchStartPoint);
}

template<FrameType Frames>
void RenderQueue<Frames>::ExecuteBatch(
	size_t startJobIndex, size_t nrOfJobsToProcess,
	ID3D12GraphicsCommandList* list, FrameResourceContext<Frames>& context,
	size_t batchIndex, RenderQueueTimerCPU& cpuTimer,
	RenderQueueTimerGPU<Frames>& gpuTimer)
{
	std::vector<D3D12_RESOURCE_BARRIER> barrierVector;

	auto batchStartPoint = cpuTimer.GetCurrentTimePoint();
	gpuTimer.MarkBatchStart(list, batchIndex);
	for (size_t i = 0; i < nrOfJobsToProcess; ++i)
	{
		auto jobStartPoint = cpuTimer.GetCurrentTimePoint();
		gpuTimer.MarkJobStart(list, i + startJobIndex);
		jobs[i + startJobIndex].ProcessJob(list, barrierVector, context);
		gpuTimer.MarkJobEnd(list, i + startJobIndex);
		cpuTimer.MarkJobExecution(i + startJobIndex, jobStartPoint);
	}
	gpuTimer.MarkBatchEnd(list, batchIndex);
	cpuTimer.MarkBatchExecution(batchIndex, batchStartPoint);
}

template<FrameType Frames>
void RenderQueue<Frames>::PrepareFrame(
	const entt::registry& frameRegistry, std::uint8_t nrOfPartitions,
	const FramePreparationContext<Frames>& context,
	RenderQueueTimerCPU& cpuTimer)
{
	PassCost totalPreparationCost = 0;
	for (auto& job : jobs)
	{
		job.GetQueueJob()->CalculateFrameCosts(frameRegistry);
		totalPreparationCost += job.GetQueueJob()->GetPreparationCost();
	}

	PassCost costPerBatch = totalPreparationCost / nrOfPartitions;

	size_t currentBatchCost = 0;
	size_t currentBatchStartIndex = 0;
	size_t currentBatchJobAmount = 0;
	size_t batchIndex = 0;

	for (size_t i = 0; i < jobs.size(); ++i)
	{
		currentBatchCost += jobs[i].GetQueueJob()->GetPreparationCost();

		if (currentBatchCost >= costPerBatch)
		{
			PrepareBatch(currentBatchStartIndex, currentBatchJobAmount + 1,
				frameRegistry, context, batchIndex++, cpuTimer);
			currentBatchCost = 0;
			currentBatchStartIndex = i + 1;
			currentBatchJobAmount = 0;
		}
		else
		{
			++currentBatchJobAmount;
		}
	}

	if (currentBatchStartIndex < jobs.size()) // The last batch was not executed in the loop
	{
		PrepareBatch(currentBatchStartIndex, currentBatchJobAmount,
			frameRegistry, context, batchIndex, cpuTimer);
	}
}

template<FrameType Frames>
void RenderQueue<Frames>::SetResourceInfo(
	const std::vector<std::pair<TransientResourceIndex, TransientResourceDesc>>& globalDescs)
{
	setupContext.Reset(transientResources.size());

	for (const auto& pair : globalDescs)
	{
		setupContext.SetTransientResourceDesc(pair.first, pair.second);
	}

	for (auto& job : jobs)
	{
		job.GetQueueJob()->SetResourceInfo(setupContext);
	}
}

template<FrameType Frames>
void RenderQueue<Frames>::SetupTransientResources(
	Blackboard<Frames>& blackboard)
{
	for (size_t i = 0; i < transientResources.size(); ++i)
	{
		blackboard.CreateTransientResource(
			setupContext.transientResourceDescs[i],
			transientResources[i].initialState);
	}

	setupContext.CreateTransientDescriptors(blackboard);
	blackboard.SetLocalFrameMemoryRequirement(setupContext.totalLocalMemoryNeeded);

	for (const auto& localDesc : setupContext.localResourceDescs)
	{
		blackboard.CreateLocalResource(localDesc);
	}
}

template<FrameType Frames>
void RenderQueue<Frames>::ExecuteJobs(
	const std::vector<ID3D12GraphicsCommandList*> lists,
	FrameResourceContext<Frames>& context, RenderQueueTimerCPU& cpuTimer,
	RenderQueueTimerGPU<Frames>& gpuTimer)
{
	PassCost totalExecutionCost = 0;
	for (auto& job : jobs)
	{
		totalExecutionCost += job.GetQueueJob()->GetExecutionCost();
	}

	PassCost costPerBatch = totalExecutionCost / lists.size();

	size_t currentBatchCost = 0;
	size_t currentBatchStartIndex = 0;
	size_t currentBatchJobAmount = 0;
	size_t currentListIndex = 0;

	for (size_t i = 0; i < jobs.size(); ++i)
	{
		currentBatchCost += jobs[i].GetQueueJob()->GetExecutionCost();

		if (currentBatchCost >= costPerBatch)
		{
			size_t batchIndex = currentListIndex;
			ExecuteBatch(currentBatchStartIndex, currentBatchJobAmount + 1,
				lists[currentListIndex++], context, batchIndex, cpuTimer,
				gpuTimer);
			currentBatchCost = 0;
			currentBatchStartIndex = i + 1;
			currentBatchJobAmount = 0;
		}
		else
		{
			++currentBatchJobAmount;
		}
	}

	if (currentBatchStartIndex < jobs.size()) // The last batch was not executed in the loop
	{
		ExecuteBatch(currentBatchStartIndex, currentBatchJobAmount,
			lists[currentListIndex], context, currentListIndex, cpuTimer,
			gpuTimer);
	}
}

template<FrameType Frames>
void RenderQueue<Frames>::PerformImguiOperations(const FrameTimesCPU& cpuTimes,
	const FrameTimesGPU& gpuTimes, ImguiContext& imguiContext)
{
	if (ImGui::BeginTabBar("Render queue tab bar"))
	{
		if (ImGui::BeginTabItem("Batch preparation information"))
		{
			for (size_t i = 0; i < cpuTimes.batchPreparationTimes.size(); ++i)
			{
				imguiContext.AddText("Preparation batch ", i, ':',
					cpuTimes.batchPreparationTimes[i]);
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Batch execution information"))
		{
			for (size_t i = 0; i < cpuTimes.batchExecutionTimes.size(); ++i)
			{
				imguiContext.AddText("Execution batch ", i, " CPU time: ",
					cpuTimes.batchExecutionTimes[i]);
				imguiContext.AddText("Execution batch ", i, " GPU time: ",
					gpuTimes.batchTimes[i]);
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Job information"))
		{
			imguiContext.AddText("Nr of jobs: ", jobs.size());

			if (ImGui::BeginTabBar("Queue job tab bar"))
			{
				for (size_t i = 0; i < jobs.size(); ++i)
				{
					std::string tabName = "Queue job " + std::to_string(i);
					if (ImGui::BeginTabItem(tabName.c_str()))
					{
						imguiContext.AddText("Preparation time: ",
							cpuTimes.jobPreparationTimes[i]);
						imguiContext.AddText("CPU execution time: ",
							cpuTimes.jobExecutionTimes[i]);
						imguiContext.AddText("GPU execution time: ",
							gpuTimes.jobTimes[i]);

						jobs[i].GetQueueJob()->PerformImguiOperations(
							imguiContext);

						ImGui::EndTabItem();
					}
				}

				ImGui::EndTabBar();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

template<FrameType Frames>
size_t RenderQueue<Frames>::GetNrOfJobs() const
{
	return jobs.size();
}

template<FrameType Frames>
inline FrameSetupContext& RenderQueue<Frames>::GetFrameSetupContext()
{
	return setupContext;
}

template<FrameType Frames>
TransientResourceIndex RenderQueue<Frames>::GetEndTextureIndex() const
{
	return endTextureIndex;
}

template<FrameType Frames>
const std::vector<FrameResourceBarrier>&
RenderQueue<Frames>::GetPostExecutionBarriers() const
{
	return postExecutionBarriers;
}

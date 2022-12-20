#pragma once

#include <vector>
#include <array>
#include <utility>
#include <chrono>
#include <stdexcept>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <FrameObject.h>
#include <D3DPtr.h>

typedef UINT64 TimeTypeGPU;

struct FrameTimesGPU
{
	double totalRenderTime = 0.0;

	double copyTime = 0.0;
	double discardAndClearTime = 0.0;

	std::vector<double> batchTimes;
	std::vector<double> jobTimes;

	double postQueueTime = 0.0;

	FrameTimesGPU& operator=(const FrameTimesGPU& other)
	{
		auto vecLambda = [](std::vector<double>& toSet,
			const std::vector<double>& toSetFrom)
		{
			if (toSet.size() != toSetFrom.size())
			{
				toSet.resize(toSetFrom.size());
			}

			for (size_t i = 0; i < toSet.size(); ++i)
			{
				toSet[i] = toSetFrom[i];
			}
		};

		totalRenderTime = other.totalRenderTime;

		copyTime = other.copyTime;
		discardAndClearTime = other.discardAndClearTime;

		vecLambda(batchTimes, other.batchTimes);
		vecLambda(jobTimes, other.jobTimes);

		postQueueTime = other.postQueueTime;

		return *this;
	}

	void operator+=(const FrameTimesGPU& other)
	{
		auto vecLambda = [](std::vector<double>& toAddTo,
			const std::vector<double>& toAddFrom)
		{
			if (toAddTo.size() != toAddFrom.size())
			{
				toAddTo.resize(toAddFrom.size());
			}

			for (size_t i = 0; i < toAddTo.size(); ++i)
			{
				toAddTo[i] += toAddFrom[i];
			}
		};

		totalRenderTime += other.totalRenderTime;

		copyTime += other.copyTime;
		discardAndClearTime += other.discardAndClearTime;

		vecLambda(batchTimes, other.batchTimes);
		vecLambda(jobTimes, other.jobTimes);

		postQueueTime += other.postQueueTime;
	}

	void operator/=(size_t divisor)
	{
		auto vecLambda = [](std::vector<double>& dividends, size_t divisor)
		{
			for (double& dividend : dividends)
			{
				dividend /= divisor;
			}
		};

		totalRenderTime /= divisor;

		copyTime /= divisor;
		discardAndClearTime /= divisor;

		vecLambda(batchTimes, divisor);
		vecLambda(jobTimes, divisor);

		postQueueTime /= divisor;
	}
};

template<FrameType Frames>
class RenderQueueTimerGPU
{
private:
	struct PerFrameResources
	{
		D3DPtr<ID3D12QueryHeap> directQueryHeap;
		D3DPtr<ID3D12Resource> directResultsBuffer;

		D3DPtr<ID3D12QueryHeap> copyQueryHeap;
		D3DPtr<ID3D12Resource> copyResultsBuffer;
	};
	FrameObject<PerFrameResources, Frames> perFrameResources;

	FrameType resolveCounter = Frames + 1;
	FrameTimesGPU lastCalculatedFrameTimes;
	FrameTimesGPU currentFrameTimes;
	double frequency = 1.0f;
	double elapsedGlobalTime = 0.0f;
	size_t elapsedFrames = 0;
	std::chrono::time_point<std::chrono::steady_clock> lastFrameStart =
		std::chrono::steady_clock::now();
	bool isActive = true;
	size_t nrOfBatchesCurrently = 0;
	size_t nrOfJobsCurrently = 0;
	TimeTypeGPU directFrequency = 0;
	TimeTypeGPU copyFrequency = 0;
	TimeTypeGPU presentFrequency = 0;

	std::vector<std::pair<FrameObject<PerFrameResources, Frames>, FrameType>> oldResources;

	D3DPtr<ID3D12QueryHeap> CreateQueryHeap(ID3D12Device* device,
		D3D12_QUERY_HEAP_TYPE type, UINT count);
	D3DPtr<ID3D12Resource> CreateResultBuffer(ID3D12Device* device, UINT count);

	void EndQuery(ID3D12GraphicsCommandList* list, ID3D12QueryHeap* heap,
		UINT index);

	double CalculateTime(const TimeTypeGPU* data, UINT startIndex, UINT endIndex,
		TimeTypeGPU frequency);
	double CalculateTime(const TimeTypeGPU* data, UINT startIndex, UINT endIndex,
		TimeTypeGPU startFrequency, TimeTypeGPU endFrequency);

	void ResetFrameTimes(FrameTimesGPU& toReset);

public:
	RenderQueueTimerGPU() = default;
	~RenderQueueTimerGPU() = default;
	RenderQueueTimerGPU(const RenderQueueTimerGPU& other) = delete;
	RenderQueueTimerGPU& operator=(const RenderQueueTimerGPU& other) = delete;
	RenderQueueTimerGPU(RenderQueueTimerGPU&& other) noexcept = default;
	RenderQueueTimerGPU& operator=(RenderQueueTimerGPU&& other) noexcept = default;

	void SetActive(bool active);
	void Reset(ID3D12Device* device, UINT nrOfBatches, UINT nrOfJobs,
		ID3D12CommandQueue* directQueue, ID3D12CommandQueue* copyQueue,
		ID3D12CommandQueue* presentQueue);

	void MarkFrameStart(ID3D12GraphicsCommandList* list);
	void MarkFrameEnd(ID3D12GraphicsCommandList* list);

	void MarkCopyStart(ID3D12GraphicsCommandList* list);
	void MarkCopyEnd(ID3D12GraphicsCommandList* list);

	void MarkDiscardAndClearStart(ID3D12GraphicsCommandList* list);
	void MarkDiscardAndClearEnd(ID3D12GraphicsCommandList* list);

	void MarkBatchStart(ID3D12GraphicsCommandList* list, UINT batchIndex);
	void MarkBatchEnd(ID3D12GraphicsCommandList* list, UINT batchIndex);

	void MarkJobStart(ID3D12GraphicsCommandList* list, UINT jobIndex);
	void MarkJobEnd(ID3D12GraphicsCommandList* list, UINT jobIndex);

	void MarkPostQueueStart(ID3D12GraphicsCommandList* list);
	void MarkPostQueueEnd(ID3D12GraphicsCommandList* list);

	void ResolveQueries(ID3D12GraphicsCommandList* directList,
		ID3D12GraphicsCommandList* copyList);
	const FrameTimesGPU& GetPreviousFrameIterationTimes();
};

template<FrameType Frames>
D3DPtr<ID3D12QueryHeap> RenderQueueTimerGPU<Frames>::CreateQueryHeap(
	ID3D12Device* device, D3D12_QUERY_HEAP_TYPE type, UINT count)
{
	D3D12_QUERY_HEAP_DESC heapDesc;
	heapDesc.Type = type;
	heapDesc.Count = count;
	heapDesc.NodeMask = 0;

	ID3D12QueryHeap* toReturn;
	HRESULT hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&toReturn));

	if (FAILED(hr))
	{
		throw std::runtime_error("Error, failed to create GPU timer query heap");
	}

	return toReturn;
}

template<FrameType Frames>
D3DPtr<ID3D12Resource> RenderQueueTimerGPU<Frames>::CreateResultBuffer(
	ID3D12Device* device, UINT count)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = count * sizeof(TimeTypeGPU);
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* toReturn;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr, IID_PPV_ARGS(&toReturn));

	if (FAILED(hr))
	{
		throw std::runtime_error("Error, failed to create GPU timer result buffer");
	}

	return toReturn;
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::EndQuery(ID3D12GraphicsCommandList* list,
	ID3D12QueryHeap* heap, UINT index)
{
	if (isActive == false)
	{
		return;
	}

	list->EndQuery(heap, D3D12_QUERY_TYPE_TIMESTAMP, index);
}

template<FrameType Frames>
double RenderQueueTimerGPU<Frames>::CalculateTime(const TimeTypeGPU* data,
	UINT startIndex, UINT endIndex, TimeTypeGPU frequency)
{
	TimeTypeGPU startTime = data[startIndex];
	TimeTypeGPU endTime = data[endIndex];

	return (endTime - startTime) / static_cast<double>(frequency);
}

template<FrameType Frames>
double RenderQueueTimerGPU<Frames>::CalculateTime(const TimeTypeGPU* data,
	UINT startIndex, UINT endIndex, TimeTypeGPU startFrequency,
	TimeTypeGPU endFrequency)
{
	TimeTypeGPU startTime = data[startIndex];
	TimeTypeGPU endTime = data[endIndex];

	return (endTime / static_cast<double>(endFrequency)) -
		(startTime / static_cast<double>(startFrequency));
}

template<FrameType Frames>
inline void RenderQueueTimerGPU<Frames>::ResetFrameTimes(FrameTimesGPU& toReset)
{
	toReset.totalRenderTime = 0.0;

	toReset.copyTime = 0.0;
	toReset.discardAndClearTime = 0.0;

	memset(toReset.batchTimes.data(), 0, 
		sizeof(double) * toReset.batchTimes.size());
	memset(toReset.jobTimes.data(), 0,
		sizeof(double) * toReset.jobTimes.size());

	toReset.postQueueTime = 0.0;
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::SetActive(bool active)
{
	isActive = active;
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::Reset(ID3D12Device* device, UINT nrOfBatches,
	UINT nrOfJobs, ID3D12CommandQueue* directQueue, ID3D12CommandQueue* copyQueue,
	ID3D12CommandQueue* presentQueue)
{
	std::chrono::time_point<std::chrono::steady_clock> currentFrameStart = 
		std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsedTime = 
		(currentFrameStart - lastFrameStart);
	elapsedGlobalTime += elapsedTime.count();
	lastFrameStart = currentFrameStart;

	if (nrOfBatches == nrOfBatchesCurrently && nrOfJobs == nrOfJobsCurrently)
	{
		perFrameResources.SwapFrame();

		if (resolveCounter > 0)
		{
			--resolveCounter;
		}

		if (oldResources.size() > 0)
		{
			for (auto& pair : oldResources)
			{
				--pair.second;
			}

			if (oldResources[0].second == 0)
			{
				oldResources.erase(oldResources.begin(), oldResources.begin() + 1);
			}
		}

		return;
	}

	elapsedGlobalTime = 0.0f;
	elapsedFrames = 0;
	oldResources.push_back(std::make_pair(std::move(perFrameResources), Frames));
	UINT directQueryCount = 6 + 2 * (nrOfBatches + nrOfJobs);

	auto initializationLambda = [&](PerFrameResources& perFrameResource)
	{
		perFrameResource.directQueryHeap = CreateQueryHeap(device,
			D3D12_QUERY_HEAP_TYPE_TIMESTAMP, directQueryCount);
		perFrameResource.directResultsBuffer = CreateResultBuffer(device,
			directQueryCount);

		UINT copyQueryCount = 2;
		perFrameResource.copyQueryHeap = CreateQueryHeap(device,
			D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP, copyQueryCount);
		perFrameResource.copyResultsBuffer = CreateResultBuffer(device,
			copyQueryCount);
	};

	perFrameResources.Initialize(initializationLambda);

	currentFrameTimes.batchTimes.resize(nrOfBatches);
	currentFrameTimes.jobTimes.resize(nrOfJobs);

	nrOfBatchesCurrently = nrOfBatches;
	nrOfJobsCurrently = nrOfJobs;
	directQueue->GetTimestampFrequency(&directFrequency);
	copyQueue->GetTimestampFrequency(&copyFrequency);
	presentQueue->GetTimestampFrequency(&presentFrequency);
	resolveCounter = Frames + 1;
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkFrameStart(ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 0);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkFrameEnd(ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 1);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkCopyStart(ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().copyQueryHeap, 0);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkCopyEnd(ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().copyQueryHeap, 1);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkDiscardAndClearStart(
	ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkDiscardAndClearEnd(
	ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 3);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkBatchStart(ID3D12GraphicsCommandList* list,
	UINT batchIndex)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 4 + batchIndex * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkBatchEnd(ID3D12GraphicsCommandList* list,
	UINT batchIndex)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 5 + batchIndex * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkJobStart(ID3D12GraphicsCommandList* list,
	UINT jobIndex)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 4 +
		nrOfBatchesCurrently * 2 + jobIndex * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkJobEnd(ID3D12GraphicsCommandList* list,
	UINT jobIndex)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 5 +
		nrOfBatchesCurrently * 2 + jobIndex * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkPostQueueStart(
	ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 4 +
		nrOfBatchesCurrently * 2 + nrOfJobsCurrently * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::MarkPostQueueEnd(
	ID3D12GraphicsCommandList* list)
{
	EndQuery(list, perFrameResources.Active().directQueryHeap, 5 +
		nrOfBatchesCurrently * 2 + nrOfJobsCurrently * 2);
}

template<FrameType Frames>
void RenderQueueTimerGPU<Frames>::ResolveQueries(
	ID3D12GraphicsCommandList* directList, ID3D12GraphicsCommandList* copyList)
{
	if (isActive == false || resolveCounter != 0)
	{
		return;
	}

	UINT directQueryCount = 6 + 2 * (nrOfBatchesCurrently + nrOfJobsCurrently);
	directList->ResolveQueryData(perFrameResources.Active().directQueryHeap,
		D3D12_QUERY_TYPE_TIMESTAMP, 0, directQueryCount,
		perFrameResources.Active().directResultsBuffer, 0);

	UINT copyQueryCount = 2;
	copyList->ResolveQueryData(perFrameResources.Active().copyQueryHeap,
		D3D12_QUERY_TYPE_TIMESTAMP, 0, copyQueryCount,
		perFrameResources.Active().copyResultsBuffer, 0);
}

template<FrameType Frames>
inline const FrameTimesGPU&
RenderQueueTimerGPU<Frames>::GetPreviousFrameIterationTimes()
{
	if (isActive == false)
	{
		return lastCalculatedFrameTimes;
	}

	ID3D12Resource* directBuffer = perFrameResources.Active().directResultsBuffer;
	ID3D12Resource* copyBuffer = perFrameResources.Active().copyResultsBuffer;

	TimeTypeGPU* directBufferData = nullptr;
	directBuffer->Map(0, nullptr, reinterpret_cast<void**>(&directBufferData));
	TimeTypeGPU* copyBufferData = nullptr;
	copyBuffer->Map(0, nullptr, reinterpret_cast<void**>(&copyBufferData));

	currentFrameTimes.totalRenderTime += CalculateTime(directBufferData, 0, 1, directFrequency);
	currentFrameTimes.copyTime += CalculateTime(copyBufferData, 0, 1, copyFrequency);
	currentFrameTimes.discardAndClearTime += CalculateTime(directBufferData, 2, 3,
		directFrequency);

	for (size_t batchIndex = 0; batchIndex < nrOfBatchesCurrently; ++batchIndex)
	{
		currentFrameTimes.batchTimes[batchIndex] += CalculateTime(directBufferData,
			4 + batchIndex * 2, 5 + batchIndex * 2, directFrequency);
	}

	size_t jobsStartBaseIndex = 4 + nrOfBatchesCurrently * 2;
	size_t jobsEndBaseIndex = jobsStartBaseIndex + 1;

	for (size_t jobIndex = 0; jobIndex < nrOfJobsCurrently; ++jobIndex)
	{
		currentFrameTimes.jobTimes[jobIndex] += CalculateTime(directBufferData,
			jobsStartBaseIndex + jobIndex * 2, jobsEndBaseIndex + jobIndex * 2,
			directFrequency);
	}

	size_t postQueueStart = jobsStartBaseIndex + nrOfJobsCurrently * 2;

	currentFrameTimes.postQueueTime += CalculateTime(directBufferData, postQueueStart,
		postQueueStart + 1, directFrequency, presentFrequency);

	D3D12_RANGE noWriteRange = { 0, 0 };
	directBuffer->Unmap(0, &noWriteRange);
	copyBuffer->Unmap(0, &noWriteRange);

	++elapsedFrames;

	if (elapsedGlobalTime >= frequency)
	{
		elapsedGlobalTime -= frequency;
		currentFrameTimes /= elapsedFrames;
		lastCalculatedFrameTimes = currentFrameTimes;
		ResetFrameTimes(currentFrameTimes);
		elapsedFrames = 0;
	}

	return lastCalculatedFrameTimes;
}
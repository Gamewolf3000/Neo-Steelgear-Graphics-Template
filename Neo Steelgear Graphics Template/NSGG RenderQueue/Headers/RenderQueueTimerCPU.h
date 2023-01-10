#pragma once

#include <vector>
#include <chrono>

struct FrameTimesCPU
{
	double totalFrameTime;

	double preRenderTime;
	double renderTime;

	double totalPreparationTime;
	std::vector<double> batchPreparationTimes;
	std::vector<double> jobPreparationTimes;

	double setupTime;
	double initializationAndUpdateTime;
	double discardAndClearTime;

	double totalExecutionTime;
	std::vector<double> batchExecutionTimes;
	std::vector<double> jobExecutionTimes;
	
	double postQueueTime;
	double imguiTime;

	FrameTimesCPU& operator=(const FrameTimesCPU& other)
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

		totalFrameTime = other.totalFrameTime;

		preRenderTime = other.preRenderTime;
		renderTime = other.renderTime;

		totalPreparationTime = other.totalPreparationTime;
		vecLambda(batchPreparationTimes, other.batchPreparationTimes);
		vecLambda(jobPreparationTimes, other.jobPreparationTimes);

		setupTime = other.setupTime;
		initializationAndUpdateTime = other.initializationAndUpdateTime;
		discardAndClearTime = other.discardAndClearTime;

		totalExecutionTime = other.totalExecutionTime;
		vecLambda(batchExecutionTimes, other.batchExecutionTimes);
		vecLambda(jobExecutionTimes, other.jobExecutionTimes);

		postQueueTime = other.postQueueTime;
		imguiTime = other.imguiTime;

		return *this;
	}

	void operator+=(const FrameTimesCPU& other)
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

		totalFrameTime += other.totalFrameTime;

		preRenderTime += other.preRenderTime;
		renderTime += other.renderTime;

		totalPreparationTime += other.totalPreparationTime;
		vecLambda(batchPreparationTimes, other.batchPreparationTimes);
		vecLambda(jobPreparationTimes, other.jobPreparationTimes);

		setupTime += other.setupTime;
		initializationAndUpdateTime += other.initializationAndUpdateTime;
		discardAndClearTime += other.discardAndClearTime;

		totalExecutionTime += other.totalExecutionTime;
		vecLambda(batchExecutionTimes, other.batchExecutionTimes);
		vecLambda(jobExecutionTimes, other.jobExecutionTimes);

		postQueueTime += other.postQueueTime;
		imguiTime += other.imguiTime;
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

		totalFrameTime /= divisor;

		preRenderTime /= divisor;
		renderTime /= divisor;

		totalPreparationTime /= divisor;
		vecLambda(batchPreparationTimes, divisor);
		vecLambda(jobPreparationTimes, divisor);

		setupTime /= divisor;
		initializationAndUpdateTime /= divisor;
		discardAndClearTime /= divisor;

		totalExecutionTime /= divisor;
		vecLambda(batchExecutionTimes, divisor);
		vecLambda(jobExecutionTimes, divisor);

		postQueueTime /= divisor;
		imguiTime /= divisor;
	}
};

typedef std::chrono::time_point<std::chrono::steady_clock> RenderQueueTimePoint;

class RenderQueueTimerCPU
{
private:
	RenderQueueTimePoint frameStart;
	FrameTimesCPU lastCalculatedFrameTimes;
	FrameTimesCPU currentFrameTimes;
	double frequency = 1.0f;
	double elapsedGlobalTime = 0.0f;
	size_t elapsedFrames = 0;
	bool isActive = true;

	double GetElapsedTime(const RenderQueueTimePoint& startPoint);
	void ResetFrameTimes(FrameTimesCPU& toReset);

public:
	RenderQueueTimerCPU() = default;
	~RenderQueueTimerCPU() = default;
	RenderQueueTimerCPU(const RenderQueueTimerCPU& other) = delete;
	RenderQueueTimerCPU& operator=(const RenderQueueTimerCPU& other) = delete;
	RenderQueueTimerCPU(RenderQueueTimerCPU&& other) noexcept = default;
	RenderQueueTimerCPU& operator=(RenderQueueTimerCPU&& other) noexcept = default;

	void SetActive(bool active);
	void Reset();
	void SetJobInfo(size_t nrOfBatches, size_t nrOfJobs);
	RenderQueueTimePoint GetCurrentTimePoint();

	RenderQueueTimePoint MarkPreRender();
	void MarkBatchPreparation(size_t batchIndex, const RenderQueueTimePoint& startPoint);
	void MarkJobPreparation(size_t jobIndex, const RenderQueueTimePoint& startPoint);
	void MarkPreparation(const RenderQueueTimePoint& startPoint);

	void MarkSetup(const RenderQueueTimePoint& startPoint);
	void MarkInitializationAndUpdate(const RenderQueueTimePoint& startPoint);
	void MarkDiscardAndClear(const RenderQueueTimePoint& startPoint);

	void MarkBatchExecution(size_t batchIndex, const RenderQueueTimePoint& startPoint);
	void MarkJobExecution(size_t jobIndex, const RenderQueueTimePoint& startPoint);
	void MarkExecution(const RenderQueueTimePoint& startPoint);

	void MarkPostQueue(const RenderQueueTimePoint& startPoint);
	void MarkImgui(const RenderQueueTimePoint& startPoint);

	void FinishFrame(RenderQueueTimePoint renderStartPoint);

	const FrameTimesCPU& GetFrameTimes();
};
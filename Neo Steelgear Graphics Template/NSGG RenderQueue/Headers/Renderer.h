#pragma once

#include <dxgidebug.h>

#include <FrameObject.h>
#include <D3DPtr.h>

#include "Dear ImGui\imgui.h"

#include "Blackboard.h"
#include "ManagedCommandAllocator.h"
#include "ManagedDevice.h"
#include "FramePreparationContext.h"
#include "FrameResourceContext.h"
#include "ManagedDescriptorHeap.h"
#include "ManagedFence.h"
#include "ManagedResourceCategories.h"
#include "QueueContext.h"
#include "RenderQueue.h"
#include "RenderWindow.h"
#include "RenderQueueTimerCPU.h"
#include "RenderQueueTimerGPU.h"
#include "ImguiContext.h"

struct DebugSettings
{
	bool useDebugLayer = true;
	bool useGPUValidation = true;
};

struct BlackboardSettings
{
	LocalAllocatorMemoryInfo localAllocatorMemoryInfo;
	TransientAllocatorMemoryInfo transientAllocatorMemoryInfo;
};

struct DescriptorHeapSettings
{
	size_t startDescriptorsPerFrame = 1000;
};

struct InformationSettings
{
	bool performTimingsCPU = true;
	bool performTimingsGPU = true;
	bool renderImgui = true;
};

//struct ThreadingSettings
//{
//	WorkQueue* workQueueToUse = nullptr;
//};

struct RenderSettings
{
	DebugSettings debug;
	DeviceRequirements device;
	WindowSettings window;
	BlackboardSettings blackboard;
	DescriptorHeapSettings descriptorHeap;
	ResourceCategoriesSettings resourceCategories;
	//ThreadingSettings threading;
	InformationSettings information;
};

template<FrameType Frames>
class Renderer : FrameBased<Frames>
{
private:
	D3DPtr<IDXGIFactory2> factory;
	ManagedDevice device;
	ManagedDescriptorHeap<Frames> descriptorHeap;
	RenderWindow<Frames> window;
	ManagedResourceCategories<Frames> resourceCategories;

	Blackboard<Frames> blackboard;
	RenderQueue<Frames> renderQueue;
	QueueContext<Frames> queueContext;
	FramePreparationContext<Frames> preparationContext;
	FrameResourceContext<Frames> resourceContext;
	std::vector<std::pair<TransientResourceIndex, TransientResourceDesc>> globalTransientDescs;

	D3DPtr<ID3D12CommandQueue> copyQueue;
	D3DPtr<ID3D12CommandQueue> directQueue;
	D3DPtr<ID3D12CommandQueue> presentQueue;

	FrameObject<ManagedFence, Frames> endOfFrameFence;
	FrameObject<ManagedFence, Frames> updateFence;
	FrameObject<ManagedFence, Frames> jobsDoneFence;

	FrameObject<ManagedCommandAllocator, Frames> updateAllocator;
	FrameObject<ManagedCommandAllocator, Frames> mainAllocator;

	RenderQueueTimerCPU cpuTimer;
	RenderQueueTimerGPU<Frames> gpuTimer;
	ImguiContext imguiContext;
	FrameTimesCPU latestTimesCPU;
	FrameTimesGPU latestTimesGPU;
	bool renderImgui = true;

	void ThrowIfFailed(HRESULT hr, const std::exception& exception);

	void HandleDebugSettings(const DebugSettings& settings);

	void CreateCommandQueues();

	void CopyFrameToBackbuffer(ID3D12GraphicsCommandList* list);
	void RenderTimesCPU();
	void RenderTimesGPU();

	void PrepareAndSetupFrame(const entt::registry& registry);
	void InitializeAndUpdateCategoryResources();
	void DiscardAndClearTransientResources();
	void ExecuteRenderQueueJobs();
	void PrepareBackbuffer();
	void RenderImgui();

public:
	Renderer() = default;
	~Renderer();
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(Renderer&& other) = delete;
	Renderer& operator=(Renderer&& other) = delete;

	void Initialize(const RenderSettings& settings);
	void FlushQueue();

	ID3D12Device* Device();
	ManagedResourceCategories<Frames>& ResourceCategories();
	QueueContext<Frames>& QueueContext();
	RenderWindow<Frames>& Window();

	void ToggleFullscreen();

	void WaitForAvailableFrame();
	void SetGlobalFrameResourceDesc(const TransientResourceIndex& index,
		const TransientResourceDesc& desc);
	void Render(const entt::registry& registry);

	const FrameTimesCPU& GetLastFrameTimes();
	const FrameTimesGPU& GetLastCycleFrameTimes();
};

template<FrameType Frames>
inline void Renderer<Frames>::ThrowIfFailed(HRESULT hr, const std::exception& exception)
{
	if (FAILED(hr))
		throw exception;
}

template<FrameType Frames>
inline void Renderer<Frames>::HandleDebugSettings(const DebugSettings& settings)
{
	if (!settings.useDebugLayer && !settings.useGPUValidation)
		return;

	ID3D12Debug1* debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)),
		std::runtime_error("Could not get debug interface"));

	if (settings.useDebugLayer)
	{
		debugController->EnableDebugLayer();
	}
	if (settings.useGPUValidation)
	{
		debugController->SetEnableGPUBasedValidation(true);
	}

	IDXGIInfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&infoQueue))))
	{
		infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

		DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
		{
			80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
		};
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		infoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
	}

	debugController->Release();
}

template<FrameType Frames>
inline void Renderer<Frames>::CreateCommandQueues()
{
	D3D12_COMMAND_QUEUE_DESC desc;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	HRESULT hr = device.GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&copyQueue));
	ThrowIfFailed(hr, std::runtime_error("Could not create copy queue"));

	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = device.GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&directQueue));
	ThrowIfFailed(hr, std::runtime_error("Could not create direct queue"));

	hr = device.GetDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&presentQueue));
	ThrowIfFailed(hr, std::runtime_error("Could not create present queue"));
}

template<FrameType Frames>
inline void Renderer<Frames>::CopyFrameToBackbuffer(ID3D12GraphicsCommandList* list)
{
	static std::vector<D3D12_RESOURCE_BARRIER> postExecutionBarriers;
	postExecutionBarriers.clear();

	postExecutionBarriers.push_back(
		window.GetSwapChain().TransitionBackbuffer(D3D12_RESOURCE_STATE_COPY_DEST));

	for (const FrameResourceBarrier& barrier : renderQueue.GetPostExecutionBarriers())
	{
		barrier.AddBarriers(postExecutionBarriers, resourceContext);
	}

	if (postExecutionBarriers.size() != 0)
	{
		list->ResourceBarrier(postExecutionBarriers.size(),
			postExecutionBarriers.data());
		postExecutionBarriers.clear();
	}

	if (renderQueue.GetEndTextureIndex() != TransientResourceIndex(-1))
	{
		list->CopyResource(window.GetSwapChain().GetCurrentBackbuffer(),
			resourceContext.GetTransientResource(
				renderQueue.GetEndTextureIndex()).resource);
	}
}

template<FrameType Frames>
inline void Renderer<Frames>::RenderTimesCPU()
{
	imguiContext.AddText("CPU times");
	imguiContext.AddText("Total: ", latestTimesCPU.totalFrameTime);
	imguiContext.AddText("Pre render: ", latestTimesCPU.preRenderTime);
	imguiContext.AddText("Render: ", latestTimesCPU.renderTime);
	imguiContext.AddText("Preparation: ",
		latestTimesCPU.totalPreparationTime);
	imguiContext.AddText("Setup time: ", latestTimesCPU.setupTime);
	imguiContext.AddText("Initialization and update: ",
		latestTimesCPU.initializationAndUpdateTime);
	imguiContext.AddText("Discard and clear: ",
		latestTimesCPU.discardAndClearTime);
	imguiContext.AddText("Execution: ", latestTimesCPU.preRenderTime);
	imguiContext.AddText("Post queue: ", latestTimesCPU.postQueueTime);
	imguiContext.AddText("Imgui: ", latestTimesCPU.imguiTime);
}

template<FrameType Frames>
inline void Renderer<Frames>::RenderTimesGPU()
{
	imguiContext.AddText("\nGPU times");
	imguiContext.AddText("Total: ", latestTimesGPU.totalRenderTime);
	imguiContext.AddText("Copy: ", latestTimesGPU.copyTime);
	imguiContext.AddText("Discard and clear: ",
		latestTimesGPU.discardAndClearTime);
	imguiContext.AddText("Post queue: ", latestTimesGPU.postQueueTime);
}

template<FrameType Frames>
inline void Renderer<Frames>::PrepareAndSetupFrame(const entt::registry& registry)
{
	auto preparationStartPoint = cpuTimer.GetCurrentTimePoint();
	resourceCategories.UpdateDescriptorHeap(descriptorHeap);
	renderQueue.PrepareFrame(registry, 1, preparationContext, cpuTimer); // 1 for now, later when multithreading it should be a setting
	cpuTimer.MarkPreparation(preparationStartPoint);

	auto setupStartPoint = cpuTimer.GetCurrentTimePoint();
	renderQueue.SetResourceInfo(globalTransientDescs);
	renderQueue.SetupTransientResources(blackboard);
	descriptorHeap.AddGlobalDescriptors(
		blackboard.GetTransientShaderBindableHandle(),
		blackboard.GetNrTransientShaderBindables());
	cpuTimer.MarkSetup(setupStartPoint);
}

template<FrameType Frames>
inline void Renderer<Frames>::InitializeAndUpdateCategoryResources()
{
	auto initAndUpdateStartPoint = cpuTimer.GetCurrentTimePoint();
	static std::vector<D3D12_RESOURCE_BARRIER> initBarriers;
	initBarriers.clear();

	blackboard.GetInitializeBarriers(initBarriers);
	gpuTimer.MarkCopyStart(updateAllocator.Active().ActiveList());
	updateAllocator.Active().ActiveList()->ResourceBarrier(initBarriers.size(),
		initBarriers.data());
	resourceCategories.ActivateNewCategories(updateAllocator.Active().ActiveList());
	resourceCategories.UpdateCategories(updateAllocator.Active().ActiveList());
	gpuTimer.MarkCopyEnd(updateAllocator.Active().ActiveList());

	updateAllocator.Active().FinishActiveList();
	updateAllocator.Active().ExecuteCommands(copyQueue);
	updateFence.Active().Signal(copyQueue);
	updateFence.Active().WaitGPU(directQueue);
	cpuTimer.MarkInitializationAndUpdate(initAndUpdateStartPoint);
}

template<FrameType Frames>
inline void Renderer<Frames>::DiscardAndClearTransientResources()
{
	auto discardAndClearStartPoint = cpuTimer.GetCurrentTimePoint();
	gpuTimer.MarkDiscardAndClearStart(mainAllocator.Active().ActiveList());
	blackboard.DiscardAndClearResources(mainAllocator.Active().ActiveList());
	gpuTimer.MarkDiscardAndClearEnd(mainAllocator.Active().ActiveList());
	mainAllocator.Active().FinishActiveList(true);
	mainAllocator.Active().ExecuteCommands(directQueue);
	cpuTimer.MarkDiscardAndClear(discardAndClearStartPoint);
}

template<FrameType Frames>
inline void Renderer<Frames>::ExecuteRenderQueueJobs()
{
	auto executionStartPoint = cpuTimer.GetCurrentTimePoint();
	std::vector<ID3D12GraphicsCommandList*> temp;
	temp.push_back(mainAllocator.Active().ActiveList());
	auto bindableDescriptorHeap = descriptorHeap.GetShaderVisibleHeap();

	for (ID3D12GraphicsCommandList* list : temp)
	{
		list->SetDescriptorHeaps(1, &bindableDescriptorHeap);
	}

	renderQueue.ExecuteJobs(temp, this->resourceContext, cpuTimer, gpuTimer);
	blackboard.UploadLocalData();
	mainAllocator.Active().FinishActiveList(true);
	mainAllocator.Active().ExecuteCommands(directQueue);
	jobsDoneFence.Active().Signal(directQueue);
	jobsDoneFence.Active().WaitGPU(presentQueue);
	cpuTimer.MarkExecution(executionStartPoint);
}

template<FrameType Frames>
inline void Renderer<Frames>::PrepareBackbuffer()
{
	auto postQueueStartPoint = cpuTimer.GetCurrentTimePoint();
	gpuTimer.MarkPostQueueStart(mainAllocator.Active().ActiveList());
	CopyFrameToBackbuffer(mainAllocator.Active().ActiveList());
	gpuTimer.MarkPostQueueEnd(mainAllocator.Active().ActiveList());
	cpuTimer.MarkPostQueue(postQueueStartPoint);
}

template<FrameType Frames>
inline void Renderer<Frames>::RenderImgui()
{
	auto imguiStartPoint = cpuTimer.GetCurrentTimePoint();

	if (renderImgui == true)
	{
		imguiContext.StartImguiFrame();
		ImGui::Begin("Information");

		if (ImGui::BeginTabBar("Main tab bar"))
		{
			if (ImGui::BeginTabItem("General information"))
			{
				RenderTimesCPU();
				RenderTimesGPU();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Render queue information"))
			{
				renderQueue.PerformImguiOperations(latestTimesCPU,
					latestTimesGPU, imguiContext);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
		auto transitionRenderTarget = window.GetSwapChain().TransitionBackbuffer(
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		mainAllocator.Active().ActiveList()->ResourceBarrier(1, &transitionRenderTarget);
		auto rtv = window.GetSwapChain().GetCurrentBackbufferRTV();
		mainAllocator.Active().ActiveList()->OMSetRenderTargets(1, &rtv, true, nullptr);
		imguiContext.FinishImguiFrame(mainAllocator.Active().ActiveList());
	}

	cpuTimer.MarkImgui(imguiStartPoint);

	auto transitionPresent = window.GetSwapChain().TransitionBackbuffer(
		D3D12_RESOURCE_STATE_PRESENT);
	mainAllocator.Active().ActiveList()->ResourceBarrier(1, &transitionPresent);
}

template<FrameType Frames>
inline Renderer<Frames>::~Renderer()
{
	FlushQueue();
}

template<FrameType Frames>
inline void Renderer<Frames>::Initialize(const RenderSettings& settings)
{
	HandleDebugSettings(settings.debug);

	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	ThrowIfFailed(hr, std::runtime_error("Could not create dxgi factory"));
	device.Initialize(factory, settings.device);
	CreateCommandQueues();
	window.Initialize(device.GetDevice(), presentQueue, factory, settings.window);

	endOfFrameFence.Initialize(&ManagedFence::Initialize, device.GetDevice(), size_t(0));
	updateFence.Initialize(&ManagedFence::Initialize, device.GetDevice(), size_t(0));
	jobsDoneFence.Initialize(&ManagedFence::Initialize, device.GetDevice(), size_t(0));
	updateAllocator.Initialize(&ManagedCommandAllocator::Initialize,
		device.GetDevice(), D3D12_COMMAND_LIST_TYPE_COPY);
	mainAllocator.Initialize(&ManagedCommandAllocator::Initialize,
		device.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);

	blackboard.Initialize(device.GetDevice(), settings.blackboard.localAllocatorMemoryInfo,
		settings.blackboard.transientAllocatorMemoryInfo);

	descriptorHeap.Initialize(device.GetDevice(),
		settings.descriptorHeap.startDescriptorsPerFrame);
	resourceCategories.Initialize(device.GetDevice(),
		settings.resourceCategories);
	//workQueue = settings.threading.workQueueToUse;

	cpuTimer.SetActive(settings.information.performTimingsCPU);
	gpuTimer.SetActive(settings.information.performTimingsGPU);
	renderImgui = settings.information.renderImgui;

	queueContext.Initialize(&renderQueue);
	preparationContext.Initialize(&descriptorHeap);
	resourceContext.Initialize(&descriptorHeap, &resourceCategories, &blackboard);
	imguiContext.Initialize(window.GetWindowHandle(), device.GetDevice());
}

template<FrameType Frames>
inline void Renderer<Frames>::FlushQueue()
{
	while (!endOfFrameFence.Active().Completed())
	{
		// Spinwait
	}
}

template<FrameType Frames>
inline ID3D12Device* Renderer<Frames>::Device()
{
	return device.GetDevice();
}

template<FrameType Frames>
inline ManagedResourceCategories<Frames>& Renderer<Frames>::ResourceCategories()
{
	return resourceCategories;
}

template<FrameType Frames>
inline QueueContext<Frames>& Renderer<Frames>::QueueContext()
{
	return queueContext;
}

template<FrameType Frames>
inline RenderWindow<Frames>& Renderer<Frames>::Window()
{
	return window;
}

template<FrameType Frames>
inline void Renderer<Frames>::ToggleFullscreen()
{
	FlushQueue(); // Wait for all commands currently active to finish
	window.ToggleFullscreen();
}

template<FrameType Frames>
inline void Renderer<Frames>::WaitForAvailableFrame()
{
	DWORD waitResult = WaitForSingleObjectEx(
		window.GetSwapChain().GetWaitHandle(), INFINITE, true);

	if (waitResult != WAIT_OBJECT_0)
		throw std::runtime_error("Waiting for swap chain failed");

	while (!endOfFrameFence.Next().Completed())
	{
		// Spinwait, not even sure if this is necessary as we wait for the swapchain
	}

	latestTimesCPU = cpuTimer.GetFrameTimes();
	cpuTimer.Reset(1, renderQueue.GetNrOfJobs()); // CHANGE THE 1 LATER TO BE BASED ON MULTI THREADING SETTINGS
	gpuTimer.Reset(device.GetDevice(), 1, renderQueue.GetNrOfJobs(), directQueue,
		copyQueue, presentQueue); // CHANGE THE 1 LATER TO BE BASED ON MULTI THREADING SETTINGS
	latestTimesGPU = gpuTimer.GetPreviousFrameIterationTimes();

	window.SwapFrame();

	endOfFrameFence.SwapFrame();
	updateFence.SwapFrame();
	jobsDoneFence.SwapFrame();
	updateAllocator.SwapFrame();
	mainAllocator.SwapFrame();

	descriptorHeap.SwapFrame();
	resourceCategories.SwapFrame();
	blackboard.SwapFrame();
	globalTransientDescs.clear();

	mainAllocator.Active().Reset();
	updateAllocator.Active().Reset();
	gpuTimer.ResolveQueries(mainAllocator.Active().ActiveList(),
		updateAllocator.Active().ActiveList());
}

template<FrameType Frames>
inline void Renderer<Frames>::SetGlobalFrameResourceDesc(
	const TransientResourceIndex& index, const TransientResourceDesc& desc)
{
	globalTransientDescs.push_back(std::make_pair(index, desc));
}

template<FrameType Frames>
inline void Renderer<Frames>::Render(const entt::registry& registry)
{
	auto renderStartPoint = cpuTimer.MarkPreRender();
	gpuTimer.MarkFrameStart(mainAllocator.Active().ActiveList());

	PrepareAndSetupFrame(registry);
	InitializeAndUpdateCategoryResources();
	DiscardAndClearTransientResources();
	descriptorHeap.UploadCurrentFrameHeap();
	ExecuteRenderQueueJobs();
	PrepareBackbuffer();
	RenderImgui();

	gpuTimer.MarkFrameEnd(mainAllocator.Active().ActiveList());
	mainAllocator.Active().FinishActiveList();
	mainAllocator.Active().ExecuteCommands(presentQueue);
	window.GetSwapChain().Present();
	endOfFrameFence.Active().Signal(presentQueue);
	cpuTimer.FinishFrame(renderStartPoint);
}

template<FrameType Frames>
inline const FrameTimesCPU& Renderer<Frames>::GetLastFrameTimes()
{
	return cpuTimer.GetFrameTimes();
}

template<FrameType Frames>
inline const FrameTimesGPU& Renderer<Frames>::GetLastCycleFrameTimes()
{
	return gpuTimer.GetPreviousFrameIterationTimes();
}

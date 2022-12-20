#pragma once

#include <stdexcept>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <D3DPtr.h>
#include <FrameObject.h>

struct SwapChainFrame
{
	D3DPtr<ID3D12Resource> backbuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_PRESENT;
};

template<FrameType Frames>
class ManagedSwapChain : public FrameObject<SwapChainFrame, Frames>
{
private:
	ID3D12Device* device;
	D3DPtr<IDXGISwapChain3> swapChain;
	D3DPtr<ID3D12DescriptorHeap> rtvHeap;
	HANDLE backbufferWaitHandle = nullptr;
	HWND windowHandle = nullptr;
	unsigned int rtvSize = 0;
	const UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	void CreateDescriptorHeap();
	void CreateRTVs();

public:
	ManagedSwapChain() = default;
	~ManagedSwapChain();

	void Initialize(ID3D12Device* deviceToUse, ID3D12CommandQueue* queue,
		IDXGIFactory2* factory, HWND handleToWindow, bool fullscreen);

	HANDLE GetWaitHandle();
	void ResizeBackbuffers(unsigned int newWidth, unsigned int newHeight);
	UINT GetBackbufferWidth();
	UINT GetBackbufferHeight();
	DXGI_FORMAT GetBackbufferFormat();
	unsigned int GetCurrentBackbufferIndex();
	ID3D12Resource* GetCurrentBackbuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackbufferRTV();
	HWND GetWindowHandle();

	void ToggleFullscreen();
	D3D12_RESOURCE_BARRIER TransitionBackbuffer(D3D12_RESOURCE_STATES newState);

	void ClearBackbuffer(ID3D12GraphicsCommandList* commandList);
	void DiscardBackbuffer(ID3D12GraphicsCommandList* commandList);
	void Present();

	void SwapFrame() override;
};

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::CreateDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = Frames;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap));
	if (FAILED(hr))
		throw std::runtime_error("Could not create descriptor heap for backbuffers");

	rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::CreateRTVs()
{
	auto rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (std::uint8_t i = 0; i < Frames; ++i)
	{
		D3DPtr<ID3D12Resource> currentBuffer;
		HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&currentBuffer));
		if (FAILED(hr))
			throw std::runtime_error("Could not fetch backbuffer");

		device->CreateRenderTargetView(currentBuffer, nullptr, rtvHandle);
		this->frameObjects[i].backbuffer = std::move(currentBuffer);
		this->frameObjects[i].rtvHandle = rtvHandle;
		rtvHandle.ptr += rtvSize;
	}
}

template<FrameType Frames>
inline ManagedSwapChain<Frames>::~ManagedSwapChain()
{
	swapChain->SetFullscreenState(FALSE, NULL); // An application is not allowed to quit in fullscreen
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::Initialize(ID3D12Device* deviceToUse,
	ID3D12CommandQueue* queue, IDXGIFactory2* factory, HWND handleToWindow,
	bool fullscreen)
{
	device = deviceToUse;
	windowHandle = handleToWindow;

	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.Width = 0;
	desc.Height = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = Frames;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = swapChainFlags;

	D3DPtr<IDXGISwapChain1> temp;
	HRESULT hr = factory->CreateSwapChainForHwnd(queue, windowHandle, &desc,
		nullptr, nullptr, &temp);
	if (FAILED(hr))
		throw std::runtime_error("Could not create swap chain");

	hr = temp->QueryInterface(__uuidof(IDXGISwapChain3),
		reinterpret_cast<void**>(&swapChain));
	if (FAILED(hr))
	{
		throw std::runtime_error("Could not query swap chain 3");
	}

	hr = swapChain->SetMaximumFrameLatency(Frames);
	if (FAILED(hr))
	{
		throw std::runtime_error("Could not set max frame latency");
	}

	backbufferWaitHandle = swapChain->GetFrameLatencyWaitableObject();

	CreateDescriptorHeap();
	CreateRTVs();
}

template<FrameType Frames>
inline HANDLE ManagedSwapChain<Frames>::GetWaitHandle()
{
	return backbufferWaitHandle;
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::ResizeBackbuffers(unsigned int newWidth,
	unsigned int newHeight)
{
	for (auto& swapChainFrame : this->frameObjects)
		swapChainFrame.backbuffer = D3DPtr<ID3D12Resource>();

	swapChain->ResizeBuffers(Frames, newWidth, newHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM, swapChainFlags);

	CreateRTVs();
	this->activeFrame = GetCurrentBackbufferIndex();
	OutputDebugStringA(std::to_string(this->activeFrame).c_str());
}

template<FrameType Frames>
inline UINT ManagedSwapChain<Frames>::GetBackbufferWidth()
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	swapChain->GetDesc1(&desc);
	return desc.Width;
}

template<FrameType Frames>
inline UINT ManagedSwapChain<Frames>::GetBackbufferHeight()
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	swapChain->GetDesc1(&desc);
	return desc.Height;
}

template<FrameType Frames>
inline DXGI_FORMAT ManagedSwapChain<Frames>::GetBackbufferFormat()
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	swapChain->GetDesc1(&desc);
	return desc.Format;
}

template<FrameType Frames>
inline unsigned int ManagedSwapChain<Frames>::GetCurrentBackbufferIndex()
{
	return swapChain->GetCurrentBackBufferIndex();
}

template<FrameType Frames>
inline ID3D12Resource* ManagedSwapChain<Frames>::GetCurrentBackbuffer()
{
	return this->Active().backbuffer;
}

template<FrameType Frames>
inline D3D12_CPU_DESCRIPTOR_HANDLE
ManagedSwapChain<Frames>::GetCurrentBackbufferRTV()
{
	return this->Active().rtvHandle;
}

template<FrameType Frames>
inline HWND ManagedSwapChain<Frames>::GetWindowHandle()
{
	return windowHandle;
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::ToggleFullscreen()
{
	BOOL fullscreenState;
	HRESULT hr = swapChain->GetFullscreenState(&fullscreenState, nullptr);
	if (FAILED(hr))
	{
		throw std::runtime_error("Could not get swap chain fullscreen state");
	}

	hr = swapChain->SetFullscreenState(!fullscreenState, nullptr);
	if (FAILED(hr))
	{
		throw std::runtime_error("Could not toggle fullscreen for swap chain");
	}
}

template<FrameType Frames>
inline D3D12_RESOURCE_BARRIER ManagedSwapChain<Frames>::TransitionBackbuffer(
	D3D12_RESOURCE_STATES newState)
{
	D3D12_RESOURCE_BARRIER toReturn;
	toReturn.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toReturn.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toReturn.Transition.pResource = this->Active().backbuffer;
	toReturn.Transition.StateBefore = this->Active().currentState;
	toReturn.Transition.StateAfter = newState;
	toReturn.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	this->Active().currentState = newState;
	return toReturn;
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::ClearBackbuffer(
	ID3D12GraphicsCommandList* commandList)
{
	float clearColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(this->Active().rtvHandle, clearColour,
		0, nullptr);
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::DiscardBackbuffer(
	ID3D12GraphicsCommandList* commandList)
{
	commandList->DiscardResource(this->Active().backbuffer, nullptr);
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::Present()
{
	if (FAILED(swapChain->Present(1, 0)))
	{
		throw std::runtime_error("Could not present backbuffer");
	}
}

template<FrameType Frames>
inline void ManagedSwapChain<Frames>::SwapFrame()
{
	this->activeFrame = GetCurrentBackbufferIndex();
}

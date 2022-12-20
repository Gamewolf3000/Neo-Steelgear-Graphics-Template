#pragma once

#include <utility>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <Windows.h>

#include "Dear ImGui\imgui_impl_dx12.h"

#include "ManagedSwapChain.h"

typedef std::function<std::pair<bool, LRESULT>(HWND, UINT, WPARAM, LPARAM)> WindowMessageCallback;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct WindowSettings
{
    std::wstring className = L"";
    std::wstring windowName = L"";
    HINSTANCE instance = nullptr;
    unsigned int windowWidth = 0;
    unsigned int windowHeight = 0;
    bool windowed = true;
};

template<FrameType Frames>
class RenderWindow : public FrameBased<Frames>
{
private:
    typedef std::unordered_map<UINT, std::vector<WindowMessageCallback>> CallbackMap;
    CallbackMap callbackMap;
    HWND windowHandle;
    ManagedSwapChain<Frames> swapChain;
    bool windowed;

    static LRESULT CALLBACK InternalWindowProc(HWND hWnd, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
            return true;

        const CallbackMap* callbackMap =
            reinterpret_cast<const CallbackMap*>(GetWindowLongPtr(hWnd, 0));

        if (callbackMap != nullptr)
        {
            auto it = callbackMap->find(message);

            if (it != callbackMap->end())
            {
                for (const auto& function : (*it).second)
                {
                    std::pair<bool, UINT> funcReturn = function(hWnd, message, wParam, lParam);

                    if (funcReturn.first == true)
                    {
                        return funcReturn.second;
                    }
                }
            }
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    void InitializeWindow(const WindowSettings& settings);

public:
    RenderWindow() = default;
    ~RenderWindow();

    void Initialize(ID3D12Device* device, ID3D12CommandQueue* presentQueue,
        IDXGIFactory2* factory, const WindowSettings& settings);

    void AddWindowCallback(UINT messageType,
        const WindowMessageCallback& callback);
    void ToggleFullscreen();

    ManagedSwapChain<Frames>& GetSwapChain();
    const HWND GetWindowHandle();

    void SwapFrame() override;
};

template<FrameType Frames>
inline void RenderWindow<Frames>::InitializeWindow(const WindowSettings& settings)
{
    WNDCLASS wc = { };
    wc.lpfnWndProc = RenderWindow<Frames>::InternalWindowProc;
    wc.hInstance = settings.instance;
    wc.lpszClassName = settings.className.c_str();
    wc.cbWndExtra = sizeof(CallbackMap*);
    RegisterClass(&wc);

    DWORD windowStyle = settings.windowed ? WS_OVERLAPPEDWINDOW :
        WS_EX_TOPMOST | WS_POPUP;
    RECT rt = { 0, 0, static_cast<LONG>(settings.windowWidth),
        static_cast<LONG>(settings.windowHeight) };
    AdjustWindowRect(&rt, windowStyle, FALSE);

    windowHandle = CreateWindowEx(0, settings.className.c_str(),
        settings.windowName.c_str(), windowStyle, 0, 0,
        rt.right - rt.left, rt.bottom - rt.top, nullptr, nullptr,
        settings.instance, nullptr);

    if (windowHandle == nullptr)
    {
        DWORD errorCode = GetLastError();
        throw std::runtime_error("Could not create window, last error: " +
            std::to_string(errorCode));
    }

    LONG_PTR callbackMapAddress = reinterpret_cast<LONG_PTR>(&callbackMap);
    SetWindowLongPtr(windowHandle, 0, callbackMapAddress);
    ShowWindow(windowHandle, settings.windowed ? SW_SHOWNORMAL : SW_SHOWMAXIMIZED);
}

template<FrameType Frames>
inline RenderWindow<Frames>::~RenderWindow()
{
    SetWindowLongPtr(windowHandle, 0, 0);
}

template<FrameType Frames>
inline void RenderWindow<Frames>::Initialize(ID3D12Device* device,
    ID3D12CommandQueue* presentQueue, IDXGIFactory2* factory,
    const WindowSettings& settings)
{
    InitializeWindow(settings);
    swapChain.Initialize(device, presentQueue, factory, windowHandle,
        !settings.windowed);
    windowed = settings.windowed;
}

template<FrameType Frames>
inline void RenderWindow<Frames>::AddWindowCallback(UINT messageType,
    const WindowMessageCallback& callback)
{
    callbackMap[messageType].push_back(callback);
}

template<FrameType Frames>
inline void RenderWindow<Frames>::ToggleFullscreen()
{
    swapChain.ToggleFullscreen();
}

template<FrameType Frames>
inline ManagedSwapChain<Frames>& RenderWindow<Frames>::GetSwapChain()
{
    return swapChain;
}

template<FrameType Frames>
inline const HWND RenderWindow<Frames>::GetWindowHandle()
{
    return swapChain.GetWindowHandle();
}

template<FrameType Frames>
inline void RenderWindow<Frames>::SwapFrame()
{
    swapChain.SwapFrame();
}

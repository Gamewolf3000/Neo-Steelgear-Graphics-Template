#include <ShellScalingAPI.h>
#include <Windows.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    //https://stackoverflow.com/questions/48172751/dxgi-monitors-enumeration-does-not-give-full-size-for-dell-p2715q-monitor
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    MSG msg = { };

    while (!(GetKeyState(VK_ESCAPE) & 0x8000) && msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
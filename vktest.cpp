#include <windows.h>
#include <stdio.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "Renderer.hpp"

#include <memory>

std::unique_ptr<Renderer> sRenderer;

static const int kWidth = 640;
static const int kHeight = 480;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int frame = 0;

    switch(iMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    
        case WM_TIMER:
            sRenderer->renderFrame(frame++);
            return 0;
    }

    return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    setbuf(stdout, NULL);

    WNDCLASSEX wndClass;
    const char *szClassName = "vktest";

    wndClass.cbSize=sizeof(WNDCLASSEX);
    wndClass.style=(UINT)NULL;
    wndClass.lpfnWndProc=WndProc;
    wndClass.cbClsExtra=0;
    wndClass.cbWndExtra=0;
    wndClass.hInstance=hInstance;
    wndClass.hIcon=LoadIcon(0,IDI_APPLICATION);
    wndClass.hCursor=LoadCursor(0,IDC_ARROW);
    wndClass.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH);
    wndClass.lpszMenuName=NULL;
    wndClass.lpszClassName=szClassName;
    wndClass.hIconSm=NULL;

    RegisterClassEx(&wndClass);

    HWND hWnd = CreateWindowEx(
        (DWORD)NULL,
        szClassName,
        "Vulkan Test",
        WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION,
        0,
        0,
        kWidth,
        kHeight,
        (HWND)NULL,
        (HMENU)NULL,
        hInstance,
        0
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    sRenderer = std::make_unique<Renderer>(hInstance, hWnd);

    SetTimer(hWnd, 1, 30, NULL);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
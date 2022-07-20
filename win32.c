#include <Windows.h>
#include <stdio.h>
#include "vt2000.h"

#define ScreenWidth 800
#define ScreenHeight 480

#define FPSDef 100

struct FPSTrace {
    unsigned long lag[FPSDef];
    int lagIndex;
    unsigned long lastDrawTick;
} mFPSTrace;

BITMAPINFO hBitmapInfo;
VOID *pvBits;
HDC hdc;
HDC hdcMem;

int AvgFPS() {
    int i;
    int total = 0;
    for (i = 0; i < FPSDef; ++i) {
        total += mFPSTrace.lag[i];
    }
    int avg = total / FPSDef;
    return avg > 0 ? 1000 / avg : 0;
}

void DrawBitmap()
{
    int x, y;

    for (y = 0; y < ScreenHeight; y++) {
        for (x = 0; x < ScreenWidth; x++) {
            if (x < ScreenWidth / 2) {
                ((UINT32 *) pvBits)[x + y * ScreenWidth] = 0xffeeeeee;
            } else {
                ((UINT32 *) pvBits)[x + y * ScreenWidth] = 0xff111111;
            }
        }
    }

    StretchDIBits(hdcMem, 0, 0, ScreenWidth, ScreenHeight, 0, 0, ScreenWidth, ScreenHeight, pvBits, &hBitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    char FPSInfo[50];
    sprintf(FPSInfo, "FPS: %d", AvgFPS());
    TextOutA(hdcMem, 0, 0, FPSInfo, strlen(FPSInfo));

    BitBlt(hdc,0, 0, ScreenWidth, ScreenHeight, hdcMem, 0, 0, SRCCOPY);

    // write lag
    unsigned long ticks = GetTickCount();
    mFPSTrace.lag[mFPSTrace.lagIndex % FPSDef] = ticks - mFPSTrace.lastDrawTick;
    mFPSTrace.lagIndex = (mFPSTrace.lagIndex + 1) % FPSDef;
    mFPSTrace.lastDrawTick = ticks;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            // do cleanup
            free(pvBits);
            DeleteDC(hdcMem);
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
        case WM_TIMER:
        default:
            break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

_Noreturn DWORD WINAPI ThreadRender(LPVOID pParam)
{
    hBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFO);
    hBitmapInfo.bmiHeader.biWidth = ScreenWidth;
    hBitmapInfo.bmiHeader.biHeight = (0 - ScreenHeight);
    hBitmapInfo.bmiHeader.biBitCount = 32;
    hBitmapInfo.bmiHeader.biPlanes = 1;
    hBitmapInfo.bmiHeader.biSizeImage = 4 * ScreenWidth * ScreenHeight;


    memset(mFPSTrace.lag, 0, sizeof(int) * FPSDef);
    mFPSTrace.lastDrawTick = GetTickCount();
    mFPSTrace.lagIndex = 0;

    pvBits = malloc(4 * ScreenWidth * ScreenHeight);
    hdcMem = CreateCompatibleDC(NULL);
    HBITMAP hbt = CreateCompatibleBitmap(hdc, ScreenWidth, ScreenHeight);
    SelectObject(hdcMem, hbt);

    while (1)
    {
        VT_Update();
        DrawBitmap();
        Sleep(1000/FPSDef);
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nCmdShow) {
    char title[50];
    sprintf(title,"vt2000  %dx%d", ScreenWidth, ScreenHeight);
    LPCTSTR lpClassName = title;
    WNDCLASS wndClass = {0};

    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH) COLOR_WINDOWTEXT;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = lpClassName;

    RegisterClass(&wndClass);

    int titleBarHeight = GetSystemMetrics(SM_CYCAPTION);

    HWND hWnd = CreateWindow(lpClassName, lpClassName,
                             WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
                             0, 0, ScreenWidth, ScreenHeight + titleBarHeight,
                             NULL, NULL, hInstance, NULL);

    VT_Init(ScreenWidth, ScreenHeight);

    hdc = GetDC(hWnd);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    DWORD threadID;
    CreateThread(NULL, 0, ThreadRender, NULL, 0, &threadID);

    MSG msg = {0};
    // 获取消息
    while (GetMessage(&msg, NULL, 0, 0)) {
        DispatchMessage(&msg);
    }
    return 0;
}

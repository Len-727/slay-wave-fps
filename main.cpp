//  main.cpp

#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <memory>
#include "Game.h"

// === ImGui 用の前方宣言 ===
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace DirectX;

namespace
{
    std::unique_ptr<Game> g_game;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_game = std::make_unique<Game>();

    // === ウィンドウ作成部分（理解すべき） ===
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;        // ウィンドウサイズ変更時に再描画
    wcex.lpfnWndProc = WndProc;                  // メッセージ処理関数
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"RivalsGameWindowClass";

    if (!RegisterClassExW(&wcex))
    {
        MessageBoxW(nullptr, L"Window class registration failed", L"Error", MB_OK);
        return 1;
    }

    // ウィンドウサイズ設定（ここは変更してみよう！）
    int width = 1280;  
    int height = 720;

    RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(0, L"RivalsGameWindowClass",
        L"Rivals Game - Phase 1", // タイトルを変更してみよう！
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"Window creation failed", L"Error", MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(g_game.get()));

    // ゲーム初期化
    g_game->Initialize(hwnd, width, height);

    // === メインゲームループ（重要！理解すべき） ===
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_game->Tick();  // ゲームのメイン処理（ここに注目！）
        }
    }

    g_game.reset();
    return (int)msg.wParam;
}

// === ウィンドウメッセージ処理（理解＋改良対象） ===
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // === ImGui にメッセージを渡す  ===
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_SIZE:
        if (game && wParam != SIZE_MINIMIZED)
        {
            game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
        }
        break;

        // === キー入力処理（後で改良予定） ===
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        // TODO: ここに他のキー処理を追加していく
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
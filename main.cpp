//  main.cpp

#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <memory>
#include "Game.h"

// === ImGui �p�̑O���錾 ===
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

    // === �E�B���h�E�쐬�����i�������ׂ��j ===
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;        // �E�B���h�E�T�C�Y�ύX���ɍĕ`��
    wcex.lpfnWndProc = WndProc;                  // ���b�Z�[�W�����֐�
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"RivalsGameWindowClass";

    if (!RegisterClassExW(&wcex))
    {
        MessageBoxW(nullptr, L"Window class registration failed", L"Error", MB_OK);
        return 1;
    }

    // �E�B���h�E�T�C�Y�ݒ�i�����͕ύX���Ă݂悤�I�j
    int width = 1280;  
    int height = 720;

    RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(0, L"RivalsGameWindowClass",
        L"Rivals Game - Phase 1", // �^�C�g����ύX���Ă݂悤�I
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

    // �Q�[��������
    g_game->Initialize(hwnd, width, height);

    // === ���C���Q�[�����[�v�i�d�v�I�������ׂ��j ===
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
            g_game->Tick();  // �Q�[���̃��C�������i�����ɒ��ځI�j
        }
    }

    g_game.reset();
    return (int)msg.wParam;
}

// === �E�B���h�E���b�Z�[�W�����i�����{���ǑΏہj ===
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // === ImGui �Ƀ��b�Z�[�W��n��  ===
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

        // === �L�[���͏����i��ŉ��Ǘ\��j ===
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        // TODO: �����ɑ��̃L�[������//���Ă���
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
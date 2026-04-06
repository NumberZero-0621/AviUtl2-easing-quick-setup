#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <string>
#include "aviutl2_sdk/include/aviutl2_sdk/plugin2.h"

#define PLUGIN_NAME L"イージング設定時短プラグイン（AviUtl2移植版）"
#define PLUGIN_VERSION L"v1.0.0"
#define PLUGIN_DEVELOPER L"蝙蝠の目（移植：No.0）"

#define TIMER_EXEC_ID 2233

static HHOOK g_hGetMsg = NULL;
static HHOOK g_hCallWnd = NULL;
static std::wstring g_last_hovered_menu_text = L"";
static bool g_pending_easing_check = false;

// 状態管理データ
static int g_last_click_x = 0;
static int g_last_click_y = 0;
static HWND g_last_click_hwnd = NULL;
static bool g_coord_locked = false;
static DWORD g_lock_released_time = 0;
static bool g_ignore_next_click = false;

// ワイド文字対応ロギング
void Log(const WCHAR *format, ...) {
    WCHAR buf[2048];
    va_list args;
    va_start(args, format);
    _vsnwprintf(buf, 2048, format, args);
    va_end(args);
    OutputDebugStringW(buf);
}

// 座標ロック解除
void UnlockCoord() {
    if (g_coord_locked) {
        g_coord_locked = false;
        g_lock_released_time = GetTickCount();
    }
}

// SendInput による物理的な Alt + クリックのシミュレーション
void SimulateAltClickPhysical(HWND hwnd, int x_client, int y_client) {
    if (!IsWindow(hwnd)) return;

    // クライアント座標をスクリーン座標に変換
    POINT pt = { x_client, y_client };
    ClientToScreen(hwnd, &pt);

    // 現在のカーソル位置を記憶
    POINT oldPt;
    GetCursorPos(&oldPt);

    // 1. ターゲットへ移動
    SetCursorPos(pt.x, pt.y);

    // 2. Alt押し下げ -> 左クリック -> Alt離し を一連の入力として送信
    INPUT inputs[4] = {};
    
    // Alt Down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_MENU;
    
    // Left Down
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    
    // Left Up
    inputs[2].type = INPUT_MOUSE;
    inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    
    // Alt Up
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_MENU;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));

    // 3. アプリケーションが処理するのをわずかに待ってからカーソルを戻す
    Sleep(10);
    SetCursorPos(oldPt.x, oldPt.y);
}

// 1. SendMessage系フック
LRESULT CALLBACK HookProc_CallWnd(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;

        if (cwp->message == WM_CAPTURECHANGED) {
            if ((HWND)cwp->lParam != NULL) {
                g_last_click_hwnd = cwp->hwnd;
                POINT pt; GetCursorPos(&pt); ScreenToClient(cwp->hwnd, &pt);
                g_last_click_x = pt.x; g_last_click_y = pt.y;
                g_coord_locked = true;
                g_lock_released_time = 0;
            } else {
                UnlockCoord();
            }
        }
        else if (cwp->message == WM_INITMENUPOPUP) {
            g_coord_locked = true;
        }
        else if (cwp->message == WM_MENUSELECT) {
            if (HIWORD(cwp->wParam) == 0xFFFF && cwp->lParam == NULL) {
                UnlockCoord();
            } else if (!(HIWORD(cwp->wParam) & MF_POPUP)) {
                WCHAR text[256];
                if (GetMenuStringW((HMENU)cwp->lParam, LOWORD(cwp->wParam), text, 256, MF_BYCOMMAND) > 0) {
                    g_last_hovered_menu_text = text;
                }
            }
        }
        else if (cwp->message == WM_UNINITMENUPOPUP) {
            g_pending_easing_check = true;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// 2. PostMessage系フック (タイマー駆動)
LRESULT CALLBACK HookProc_GetMsg(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSG* msg = (MSG*)lParam;

        if (msg->message == WM_LBUTTONDOWN) {
            if (g_ignore_next_click) {
                g_ignore_next_click = false;
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }
            if (!g_coord_locked) {
                g_last_click_x = LOWORD(msg->lParam);
                g_last_click_y = HIWORD(msg->lParam);
                g_last_click_hwnd = msg->hwnd;
            }
        }
        else if (msg->message == WM_TIMER && msg->wParam == TIMER_EXEC_ID) {
            KillTimer(msg->hwnd, TIMER_EXEC_ID);
            
            HWND hTarget = IsWindow(g_last_click_hwnd) ? g_last_click_hwnd : msg->hwnd;
            g_ignore_next_click = true;
            SimulateAltClickPhysical(hTarget, g_last_click_x, g_last_click_y);
            UnlockCoord();
        }
        else if (msg->message == WM_USER && g_pending_easing_check) {
            g_pending_easing_check = false;

            bool is_lock_valid = g_coord_locked || (GetTickCount() - g_lock_released_time < 1000);

            if (is_lock_valid && g_last_hovered_menu_text.find(L"easing") != std::wstring::npos) {
                // UIの落ち着きを待つため、1ms 後のタイマー経由でクリック実行
                SetTimer(msg->hwnd, TIMER_EXEC_ID, 1, NULL);
            } else {
                UnlockCoord();
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

extern "C" {
__declspec(dllexport) COMMON_PLUGIN_TABLE *GetCommonPluginTable(void) {
    static COMMON_PLUGIN_TABLE table = {PLUGIN_NAME, PLUGIN_NAME L" " PLUGIN_VERSION L" by " PLUGIN_DEVELOPER};
    return &table;
}
__declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE *host) {}
__declspec(dllexport) bool InitializePlugin(DWORD version) {
    DWORD tid = GetCurrentThreadId();
    g_hGetMsg = SetWindowsHookEx(WH_GETMESSAGE, HookProc_GetMsg, NULL, tid);
    g_hCallWnd = SetWindowsHookEx(WH_CALLWNDPROC, HookProc_CallWnd, NULL, tid);
    return true;
}
__declspec(dllexport) void UninitializePlugin() {
    if (g_hGetMsg) UnhookWindowsHookEx(g_hGetMsg);
    if (g_hCallWnd) UnhookWindowsHookEx(g_hCallWnd);
}
}

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <string>
#include "aviutl2_sdk/include/aviutl2_sdk/plugin2.h"

#define PLUGIN_NAME L"イージング設定時短プラグイン"
#define PLUGIN_VERSION L"v3.4.1 (Optimized)"
#define PLUGIN_DEVELOPER L"No.0 (Original: 蝙蝠 of Eye)"

static HHOOK g_hGetMsg = NULL;
static HHOOK g_hCallWnd = NULL;
static std::wstring g_last_hovered_menu_text = L"";
static bool g_pending_easing_check = false;

// 1. SendMessage系を監視するフック (メニュー操作の監視)
LRESULT CALLBACK HookProc_CallWnd(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;

        if (cwp->message == WM_MENUSELECT) {
            UINT uItem = LOWORD(cwp->wParam);
            UINT fuFlags = HIWORD(cwp->wParam);
            HMENU hMenu = (HMENU)cwp->lParam;

            if (fuFlags != 0xFFFF && hMenu != NULL && !(fuFlags & MF_POPUP)) {
                WCHAR text[256] = {0};
                if (GetMenuStringW(hMenu, uItem, text, 256, MF_BYCOMMAND) > 0) {
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

// 2. PostMessage系を監視するフック (UI更新完了の監視)
LRESULT CALLBACK HookProc_GetMsg(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSG* msg = (MSG*)lParam;

        if (msg->message == WM_USER && g_pending_easing_check) {
            g_pending_easing_check = false; 

            if (g_last_hovered_menu_text.find(L"easing") != std::wstring::npos &&
                g_last_hovered_menu_text.find(L"@") != std::wstring::npos) {
                
                // 検証用ポップアップ
                MessageBoxW(msg->hwnd, 
                            (L"イージングスクリプトが選択されました！\n項目: " + g_last_hovered_menu_text).c_str(), 
                            L"Easing Quick Setup (Confirmed)", 
                            MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
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

__declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE *host) {
}

__declspec(dllexport) bool InitializePlugin(DWORD version) {
    // UIスレッドに対してのみフックを登録
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

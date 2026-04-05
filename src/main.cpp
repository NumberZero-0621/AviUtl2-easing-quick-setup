#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <tlhelp32.h>
#include "aviutl2_sdk/include/aviutl2_sdk/plugin2.h"

#define PLUGIN_NAME L"イージング設定時短プラグイン"
#define PLUGIN_VERSION L"v3.4.0"
#define PLUGIN_DEVELOPER L"No.0 (Original: 蝙蝠 of Eye)"

static std::vector<DWORD> g_hooked_tids;
static std::vector<HHOOK> g_hooks;
static DWORD g_last_action_time = 0;
static std::map<HWND, std::wstring> g_prev_texts;

void Log(const char *format, ...) {
  char buf[2048];
  va_list args;
  va_start(args, format);
  vsprintf(buf, format, args);
  va_end(args);
  OutputDebugStringA(buf);
}

void PhysicalClick(HWND hwnd) {
    if (!IsWindow(hwnd)) return;
    RECT r; GetClientRect(hwnd, &r);
    int x = (r.left + r.right) / 2;
    int y = (r.top + r.bottom) / 2;
    SendMessage(hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

void DoBroadHuntFrom(HWND hEdit) {
    DWORD now = GetTickCount();
    if (now - g_last_action_time < 1000) return;
    g_last_action_time = now;

    Log("EQS: ACTION: State change detected. Triggering hunter...");
    
    // 入力欄の ID を取得
    int id = (int)GetWindowLongPtr(hEdit, GWLP_ID);
    
    // 候補1: ID+1
    HWND hNext = GetDlgItem(GetParent(hEdit), id + 1);
    if (id != 0 && hNext && IsWindowVisible(hNext)) {
        Log("EQS: HUNTER: Clicking by ID+1: %p", hNext);
        PhysicalClick(hNext);
    } else {
        // 候補2: 物理座標エイム (右 10px)
        RECT r; GetWindowRect(hEdit, &r);
        POINT pt = { r.right + 10, r.top + (r.bottom - r.top) / 2 };
        HWND hTarget = WindowFromPoint(pt);
        if (hTarget && hTarget != hEdit) {
            Log("EQS: HUNTER: Clicking Neighbor via Coordinates: %p", hTarget);
            PhysicalClick(hTarget);
        }
    }
}

// 状態監視 (ポーリング)
void PollingHunter() {
  for (DWORD tid : g_hooked_tids) {
    EnumThreadWindows(tid, [](HWND hTop, LPARAM lp) -> BOOL {
      if (!IsWindowVisible(hTop)) return TRUE;
      EnumChildWindows(hTop, [](HWND hwnd, LPARAM lp) -> BOOL {
        // 全コントロールのテキストを抜き出す (W版)
        WCHAR title[256] = {0};
        GetWindowTextW(hwnd, title, 256);
        if (wcsstr(title, L"easing") && wcsstr(title, L"@")) {
           std::wstring current = title;
           if (g_prev_texts.count(hwnd) && g_prev_texts[hwnd] != current) {
               Log("EQS: STATE CHANGE detected on %p: '%ls' -> '%ls'", hwnd, g_prev_texts[hwnd].c_str(), current.c_str());
               g_prev_texts[hwnd] = current;
               DoBroadHuntFrom(hwnd);
           } else {
               g_prev_texts[hwnd] = current;
           }
        }
        return TRUE;
      }, 0);
      return TRUE;
    }, 0);
  }
}

LRESULT CALLBACK HookProc_GetMsg(int nCode, WPARAM wParam, LPARAM lParam) {
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void UpdateHooks() {
  DWORD pid = GetCurrentProcessId();
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (hSnapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 te = {sizeof(te)};
    if (Thread32First(hSnapshot, &te)) {
      do {
        if (te.th32OwnerProcessID == pid) {
          bool already = false;
          for (DWORD tid : g_hooked_tids) if (tid == te.th32ThreadID) { already = true; break; }
          if (!already) {
            g_hooks.push_back(SetWindowsHookEx(WH_GETMESSAGE, HookProc_GetMsg, NULL, te.th32ThreadID));
            g_hooked_tids.push_back(te.th32ThreadID);
            Log("EQS: New Thread Hooked: %lu", te.th32ThreadID);
          }
        }
      } while (Thread32Next(hSnapshot, &te));
    }
    CloseHandle(hSnapshot);
  }
}

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD) {
    static int heartbeat_counter = 0;
    if (++heartbeat_counter >= 10) { // 約 2秒おき
        Log("EQS: Heartbeat - Polling %d targets.", (int)g_prev_texts.size());
        UpdateHooks();
        heartbeat_counter = 0;
    }
    PollingHunter();
}

extern "C" {
__declspec(dllexport) COMMON_PLUGIN_TABLE *GetCommonPluginTable(void) {
  static COMMON_PLUGIN_TABLE table = {PLUGIN_NAME, PLUGIN_NAME L" " PLUGIN_VERSION L" by " PLUGIN_DEVELOPER};
  return &table;
}
__declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE *host) {
}
__declspec(dllexport) bool InitializePlugin(DWORD version) {
  Log("EQS: Plugin Loaded v3.4.0 (Polling mode).");
  g_hooked_tids.clear();
  UpdateHooks();
  // 200ms おきにポーリング
  SetTimer(NULL, 0, 200, TimerProc);
  return true;
}
__declspec(dllexport) void UninitializePlugin() {
  for (auto h : g_hooks) { if (h) UnhookWindowsHookEx(h); }
}
}

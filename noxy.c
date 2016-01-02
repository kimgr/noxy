#include "stdafx.h"
#include "resource.h"
#include "noxy.h"

HINSTANCE appInstance = NULL;

static HFONT buttonFont;
static HWND button;
static HKEY notificationKey;
static HANDLE notificationEvent;
static HANDLE notificationWait;

#define WM_PROXYSETTINGS_CHANGED WM_APP + 100

static HFONT CreateButtonFont(HWND hWnd) {
  static const long BUTTON_FONT_SIZE = 64;

  LOGFONT logFont = {0};
  logFont.lfHeight = BUTTON_FONT_SIZE;
  logFont.lfWeight = FW_BOLD;
  _tcscpy_s(logFont.lfFaceName, _countof(logFont.lfFaceName), _T("Arial"));

  return CreateFontIndirect(&logFont);
}

static HKEY OpenConnectionKey(REGSAM perms) {
  HKEY key = NULL;
  DWORD result =
      RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\")
                                      _T("CurrentVersion\\")
                                      _T("Internet Settings\\Connections"),
                   0, perms, &key);
  if (result != ERROR_SUCCESS) {
    SetLastError(result);
    return NULL;
  }

  return key;
}

static BOOL ReadConnectionSettings(HKEY key, LPBYTE data, LPDWORD dataSize) {
  DWORD type = 0;
  DWORD result = RegQueryValueEx(key, _T("DefaultConnectionSettings"), NULL,
                                 &type, data, dataSize);
  if (result != ERROR_SUCCESS) {
    SetLastError(result);
    return FALSE;
  }

  return TRUE;
}

static BOOL WriteConnectionSettings(HKEY key, LPBYTE data, DWORD dataSize) {
  DWORD result = RegSetValueEx(key, _T("DefaultConnectionSettings"), 0,
                               REG_BINARY, data, dataSize);
  if (result != ERROR_SUCCESS) {
    SetLastError(result);
    return FALSE;
  }
  return TRUE;
}

static BOOL IsProxyEnabled(LPBYTE data) { return ((data[8] & 4) == 4); }

static void SetProxyEnabled(LPBYTE data, BOOL enabled) {
  if (enabled)
    data[8] = data[8] | 4;
  else
    data[8] = data[8] & ~4;
}

static void UpdateProxyState() {
  HKEY key = NULL;
  BYTE data[1024] = {0};
  DWORD dataSize = sizeof(data);

  key = OpenConnectionKey(KEY_QUERY_VALUE | KEY_SET_VALUE);
  if (key == NULL)
    return;

  if (ReadConnectionSettings(key, data, &dataSize)) {
    SetProxyEnabled(data, !IsProxyEnabled(data));
    WriteConnectionSettings(key, data, dataSize);
  }

  RegCloseKey(key);
}

static BOOL GetProxyState() {
  HKEY key = NULL;
  BYTE data[1024] = {0};
  DWORD dataSize = sizeof(data);

  key = OpenConnectionKey(KEY_QUERY_VALUE);
  if (key != NULL) {
    ReadConnectionSettings(key, data, &dataSize);
    RegCloseKey(key);
  }

  return IsProxyEnabled(data);
}

static LPCTSTR GetButtonText() {
  if (GetProxyState())
    return _T("Proxy: enabled");
  else
    return _T("Proxy: disabled");
}

static void CALLBACK
OnRegistryChange(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
  PostMessage((HWND)lpParameter, WM_PROXYSETTINGS_CHANGED, 0, 0);

  ResetEvent(notificationEvent);
  RegNotifyChangeKeyValue(notificationKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET,
                          notificationEvent, TRUE);
}

static void StartRegistryChangeMonitor(HWND hwnd) {
  notificationEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  notificationKey = OpenConnectionKey(KEY_NOTIFY);
  RegNotifyChangeKeyValue(notificationKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET,
                          notificationEvent, TRUE);

  RegisterWaitForSingleObject(&notificationWait, notificationEvent,
                              OnRegistryChange, (LPVOID)hwnd, INFINITE,
                              WT_EXECUTEINWAITTHREAD);
}

static void StopRegistryChangeMonitor() {
  UnregisterWait(notificationWait);
  RegCloseKey(notificationKey);
  CloseHandle(notificationEvent);
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  RECT rect = {0};
  int width, height;
  MINMAXINFO* minMaxInfo = NULL;

  switch (message) {
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_PROXY_BUTTON:
      UpdateProxyState();
      SetWindowText(button, GetButtonText());
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;

  case WM_CREATE:
    /* Create the button */
    button = CreateWindowEx(BS_PUSHBUTTON, _T("BUTTON"), GetButtonText(),
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 10, 10,
                            10, hWnd, (HMENU)IDC_PROXY_BUTTON, appInstance, 0);
    buttonFont = CreateButtonFont(hWnd);
    SendMessage(button, WM_SETFONT, (WPARAM)buttonFont,
                (LPARAM)MAKELONG(TRUE, 0));

    /* Set up registry change notification */
    StartRegistryChangeMonitor(hWnd);
    break;

  case WM_DESTROY:
    StopRegistryChangeMonitor();
    DestroyWindow(button);
    DeleteObject(buttonFont);
    PostQuitMessage(0);
    break;

  case WM_GETMINMAXINFO:
    minMaxInfo = (MINMAXINFO*)lParam;
    minMaxInfo->ptMinTrackSize.x = 500;
    minMaxInfo->ptMinTrackSize.y = 250;
    break;

  case WM_PROXYSETTINGS_CHANGED:
    /* Proxy settings changed elsewhere, update the UI */
    SetWindowText(button, GetButtonText());
    break;

  case WM_SIZE:
    GetClientRect(hWnd, &rect);
    width = rect.right - rect.left - 20;
    height = rect.bottom - rect.top - 20;
    MoveWindow(button, 10, 10, width, height, TRUE);
    break;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

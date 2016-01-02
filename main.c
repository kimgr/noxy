#include "stdafx.h"
#include "resource.h"
#include "noxy.h"

ATOM RegisterWindowClass(LPCTSTR className) {
  WNDCLASSEX wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = appInstance;
  wcex.hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_NOXY));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszClassName = className;
  wcex.hIconSm = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine,
                       _In_ int nCmdShow) {
  HWND hWnd;
  MSG msg;
  TCHAR appTitle[64] = {0};
  LPCTSTR windowClass = _T("NOXYWND");

  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  appInstance = hInstance;

  LoadString(appInstance, IDS_APP_TITLE, appTitle, _countof(appTitle));

  RegisterWindowClass(windowClass);

  hWnd = CreateWindow(windowClass, appTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                      CW_USEDEFAULT, 500, 250, NULL, NULL, appInstance, NULL);
  if (!hWnd)
    return 1;

  ShowWindow(hWnd, nCmdShow);

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

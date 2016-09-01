// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"


#pragma data_seg(".shared")
HINSTANCE hInstance = NULL;
HHOOK hHookCBT = NULL;
HHOOK hHookCWPR = NULL;
HWND hWndServer = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,rws")



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	hInstance = hModule;
	return TRUE;
}

LRESULT CALLBACK HookCBT(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0) {
		return CallNextHookEx(hHookCBT, nCode, wParam, lParam);
	}
	PostMessage(hWndServer, WM_APP + nCode, wParam, 0);
	return CallNextHookEx(hHookCBT, nCode, wParam, lParam);
}

LRESULT CALLBACK HookCWPR(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0) {
		return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
	}
	CWPRETSTRUCT *msg = (CWPRETSTRUCT*)lParam;

	PostMessage(hWndServer, WM_APP + 0x4000 + msg->message, (WPARAM)msg->hwnd, (LPARAM)msg->wParam);
	return CallNextHookEx(hHookCWPR, nCode, wParam, lParam);
}

extern "C" BOOL _declspec(dllexport) InstallHook(HWND hWnd, DWORD pid)
{
	hWndServer = hWnd;
	hHookCWPR = SetWindowsHookEx(WH_CALLWNDPROCRET, HookCWPR, hInstance, pid);
	if (!hHookCWPR) {
		return false;
	}
	hHookCBT = SetWindowsHookEx(WH_CBT, HookCBT, hInstance, pid);
	if (!hHookCBT) {
		return false;
	}
	return true;
}

extern "C" BOOL _declspec(dllexport) UninstallHook()
{
	hWndServer = NULL;
	return UnhookWindowsHookEx(hHookCBT);
}
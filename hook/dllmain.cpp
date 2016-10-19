// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#pragma data_seg(".shared")
HINSTANCE hInstance = NULL;
HHOOK hHookCBT = NULL;
HHOOK hHookCWPR = NULL;
HWINEVENTHOOK hHookCursor = NULL;
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

void CALLBACK CursorEventProc(HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime)
{
	if (idObject == OBJID_CURSOR) {
		PostMessage(hWndServer, WM_APP + 0x3FFF, 0, 0);
	}
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

	hHookCursor = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, hInstance, CursorEventProc, pid, 0, WINEVENT_INCONTEXT);
	if (!hHookCursor) {
		return false;
	}
	return true;
}

extern "C" BOOL _declspec(dllexport) UninstallHook()
{
	hWndServer = NULL;
	UnhookWindowsHookEx(hHookCBT);
	UnhookWindowsHookEx(hHookCWPR);
	UnhookWinEvent(hHookCursor);
	return true;
}

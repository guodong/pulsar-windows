// pulsar-windows.cpp : 定义应用程序的入口点。
//

#define USE_OPENH264 1

#include "stdafx.h"
#include "pulsar-windows.h"
#include <WinSock2.h>
#include <list>
#include <map>
#include "cip_window.h"
#include "cip_protocol.h"
#include "keymap.h"
#include <stdint.h>
#include <mutex>
#include <ShlObj.h>
#include <OleCtl.h>
#include <shellapi.h>

#include <codec_api.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp")

#include <cpprest\ws_client.h>
using namespace web;
using namespace web::websockets::client;
using namespace std;

#include <cpprest\producerconsumerstream.h>

//#define DEV 1

extern "C"
{
#include <libswscale\swscale.h>
#include <libavcodec\avcodec.h>
}


#define uint8 uint8_t

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hWndServer;								// 接受hook消息的主窗口句柄
sockaddr_in sinaddr;

SRWLOCK windowsRWLock;
std::map<int, cip_window_t*> windows;
SOCKET serSocket;
SOCKET cliSocket;
std::mutex sendLock;
BOOL forceKeyFrame = false;
LPTSTR szCmdline = _tcsdup(TEXT("C:\\Windows\\system32\\notepad.exe"));
LPWSTR token;
int udp_port = 0; //udp server port
websocket_callback_client client;


// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
typedef BOOL(CALLBACK *INSTALLHOOK)(HWND, DWORD);
typedef BOOL(CALLBACK *UNINSTALLHOOK)();
void sendData(void *payload, int length);
void SendDataWs(void *payload, int length);



/* check whether window is top level */
BOOL isTopWindow(HWND hwnd)
{
	if (hwnd == GetAncestor(hwnd, GA_ROOT)) {
		return true;
	}
	HWND parent = GetAncestor(hwnd, GA_PARENT);
	return (parent == GetDesktopWindow());
}

void HandleEvent(void *payload)
{
	char *data = (char*)payload;
	switch (data[0]) {
	case CIP_EVENT_MOUSE_MOVE: {
		cip_event_mouse_move_t *cemm = (cip_event_mouse_move_t*)data;
		// do not use SetCursorPosB, it will conflict with each user
		// SetCursorPos(cemm->x, cemm->y);
		INPUT input;
		input.type = INPUT_MOUSE;
		input.mi.mouseData = 0;
		int x = cemm->x; //fix position
		int y = cemm->y; //fix position
						 /*if (x > (rc.right - rc.left - 5)) {
						 x = rc.right - rc.left - 5;
						 }
						 if (y > (rc.bottom - rc.top - 41)) {
						 y = rc.bottom - rc.top - 41;
						 }*/
		input.mi.dx = 65535 * x / (GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
		input.mi.dy = 65536 * y / (GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
		input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		SendInput(1, &input, sizeof(input));
		break;
	}
	case CIP_EVENT_MOUSE_DOWN: {
		cip_event_mouse_down_t *cemd = (cip_event_mouse_down_t*)data;
		INPUT input;
		input.type = INPUT_MOUSE;
		if (cemd->code == 1) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		} else if (cemd->code == 3) {
			input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		}
		SendInput(1, &input, sizeof(input));
		break;
	}
	case CIP_EVENT_MOUSE_UP: {
		cip_event_mouse_up_t *cemd = (cip_event_mouse_up_t*)data;
		INPUT input;
		input.type = INPUT_MOUSE;
		if (cemd->code == 1) {
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		} else if (cemd->code == 3) {
			input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		}
		SendInput(1, &input, sizeof(input));
		break;
	}
	case CIP_EVENT_KEY_DOWN: {
		cip_event_key_down_t *cekd = (cip_event_key_down_t*)data;
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.time = 0;
		input.ki.wVk = keymap[cekd->code];
		input.ki.dwExtraInfo = 0;
		input.ki.dwFlags = 0;
		input.ki.wScan = 0;
		SendInput(1, &input, sizeof(input));
		break;
	}
	case CIP_EVENT_KEY_UP: {
		cip_event_key_up_t *ceku = (cip_event_key_up_t*)data;
		INPUT input;
		input.type = INPUT_KEYBOARD;
		input.ki.time = 0;
		input.ki.wVk = keymap[ceku->code];
		input.ki.dwExtraInfo = 0;
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		input.ki.wScan = 0;
		SendInput(1, &input, sizeof(input));
		break;
	}
	default:
		break;
	}
}

int CreateUDPClient()
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		return 1;
	}

	cliSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sinaddr.sin_family = AF_INET;
	sinaddr.sin_port = htons(9000);
	sinaddr.sin_addr.S_un.S_addr = inet_addr("10.10.219.132");

	int len = sizeof(sinaddr);

	// send token to gateway
	/*wchar_t username[100];
	DWORD namesize = 100;
	GetUserName(username, &namesize);*/
	char str[100];
	memset(str, 0, 100);
	wcstombs(str, token, 100);
	sendLock.lock();
	sendto(cliSocket, str, 100, 0, (sockaddr*)&sinaddr, len);
	sendLock.unlock();
	HWND hDesktop = GetDesktopWindow();
	RECT rc;
	GetWindowRect(hDesktop, &rc);
	while (true) {
		char data[255];
		memset(data, 0, 255);
		int recvlen = recvfrom(cliSocket, data, 255, 0, (sockaddr*)&sinaddr, &len);
		int err = WSAGetLastError();
		if (recvlen <= 0) {
			continue;
		}
		HandleEvent(data);
	}
	closesocket(cliSocket);
	WSACleanup();
	return 0;
}

void toeven(size_t *num)
{
	if (*num % 4) {
		*num = *num - (*num % 4);
	}
}

DWORD WINAPI ScreenStreamThread(LPVOID lp)
{
	HWND hDesktop = GetDesktopWindow();
	RECT rc;
	GetWindowRect(hDesktop, &rc);
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	toeven((size_t*)&width);
	toeven((size_t*)&height);

	ISVCEncoder *encoder;
	int rv = WelsCreateSVCEncoder(&encoder);

	SEncParamExt param;
	encoder->GetDefaultParams(&param);
	param.iUsageType = SCREEN_CONTENT_REAL_TIME;
	param.fMaxFrameRate = 30;
	param.iPicWidth = width;
	param.iPicHeight = height;
	param.iTargetBitrate = 300000;
	param.uiMaxNalSize = 65000;
	param.iEntropyCodingModeFlag = 0;
	param.bEnableDenoise = 0;
	param.bPrefixNalAddingCtrl = false;
	param.iTemporalLayerNum = 1;
	param.iSpatialLayerNum = 1;
	param.uiIntraPeriod = 250;
	param.bEnableBackgroundDetection = 1;
	param.bEnableFrameSkip = 1;
	param.bEnableLongTermReference = 0;
	param.iRCMode = RC_QUALITY_MODE;
	param.iComplexityMode = LOW_COMPLEXITY;

	for (int i = 0; i < param.iSpatialLayerNum; i++) {
		param.sSpatialLayers[i].iVideoWidth = width >> (param.iSpatialLayerNum - 1 - i);
		param.sSpatialLayers[i].iVideoHeight = height >> (param.iSpatialLayerNum - 1 - i);
		param.sSpatialLayers[i].fFrameRate = 30;
		param.sSpatialLayers[i].iSpatialBitrate = param.iTargetBitrate;
		param.sSpatialLayers[i].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
		param.sSpatialLayers[i].sSliceArgument.uiSliceNum = 0;
	}


	encoder->InitializeExt(&param);


	SFrameBSInfo info;
	memset(&info, 0, sizeof(SFrameBSInfo));
	SSourcePicture pic;
	memset(&pic, 0, sizeof(SSourcePicture));
	pic.iPicWidth = width;
	pic.iPicHeight = height;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = pic.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
	

	HDC desktopDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(desktopDC);
	HBITMAP hbmp = CreateCompatibleBitmap(desktopDC, width, height);
	SelectObject(memDC, hbmp);

	BYTE *data, *yuvData;
	int size = 4 * width * height;
	data = (BYTE*)malloc(size * 2);
	//yuvData = (BYTE*)malloc(size);
	memset(data, 0, size);
	//memset(yuvData, 0, size);
	char *buf = (char*)malloc(70000);
	
	SwsContext *ctx = sws_getContext(width, height, AV_PIX_FMT_RGB32, width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	AVPicture pFrameYUV;

	avpicture_fill(&pFrameYUV, &data[size], AV_PIX_FMT_YUV420P, width, height);
	const int inLinesize[1] = { 4 * width };
	
	while (true) {
		BitBlt(memDC, 0, 0, width, height, desktopDC, 0, 0, SRCCOPY);

		BITMAPINFOHEADER bmi = { 0 };
		bmi.biSize = sizeof(BITMAPINFOHEADER);
		bmi.biPlanes = 1;
		bmi.biBitCount = 32;
		bmi.biWidth = width;
		bmi.biHeight = -height;
		bmi.biCompression = BI_RGB;

		GetDIBits(memDC, hbmp, 0, height, data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

		sws_scale(ctx, (const uint8_t * const *)&data, inLinesize, 0, height, pFrameYUV.data, pFrameYUV.linesize);


		pic.pData[0] = (unsigned char*)&data[size];
		pic.pData[1] = pic.pData[0] + width * height;
		pic.pData[2] = pic.pData[1] + (width * height >> 2);

		rv = encoder->EncodeFrame(&pic, &info);
		int layer, i;
		if (info.eFrameType != videoFrameTypeSkip) {
			for (layer = 0; layer < info.iLayerNum; layer++) {
				int x = 0;
				for (i = 0; i < info.sLayerInfo[layer].iNalCount; i++) {
					int length = sizeof(cip_event_window_frame_ws_t) + info.sLayerInfo[layer].pNalLengthInByte[i];
					cip_event_window_frame_ws_t *p = (cip_event_window_frame_ws_t*)buf;
					p->type = CIP_EVENT_WINDOW_FRAME;
					p->wid = 0;
					memcpy(buf + sizeof(cip_event_window_frame_ws_t), &(info.sLayerInfo[layer].pBsBuf[x]), info.sLayerInfo[layer].pNalLengthInByte[i]);
					SendDataWs(buf, length);
					x += info.sLayerInfo[layer].pNalLengthInByte[i];
				}
			}
		}
		//Sleep(100);
	}
	sws_freeContext(ctx);
	free(buf);
	free(data);

	return 0;
}


DWORD WINAPI RunCloudware(LPVOID lp)
{
	//Sleep(3000); // wait for main window show
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		szCmdline,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		) {
		goto err;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);

err:
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	cip_event_exit_t ext;
	ext.type = CIP_EVENT_EXIT;
	SendDataWs(&ext, sizeof(ext));
	client.close().then([]() { /* Successfully closed the connection. */ });
#ifndef DEV
	/* logout user */
	ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_FLAG_PLANNED);
#endif
	/* TODO: clean memory and exit*/
	exit(0);
}


DWORD WINAPI SyncState(LPVOID lp)
{
	while (true) {
		Sleep(2000);
		std::map<int, cip_window_t*>::iterator it;
		AcquireSRWLockShared(&windowsRWLock);
		for (it = windows.begin(); it != windows.end(); it++) {
			cip_window_t *window = it->second;
			BOOL visible = IsWindowVisible((HWND)window->wid);
			if (visible) {
				window->visible = visible;
				cip_event_window_show_t cews;
				cews.type = CIP_EVENT_WINDOW_SHOW;
				cews.wid = window->wid;
				cews.bare = 1;
				sendData(&cews, sizeof(cews));
			} else {
				window->visible = visible;
				cip_event_window_show_t cews;
				cews.type = CIP_EVENT_WINDOW_HIDE;
				cews.wid = window->wid;
				cews.bare = 1;
				sendData(&cews, sizeof(cews));
			}
			Sleep(100);
			RECT rc;
			GetWindowRect((HWND)window->wid, &rc);
			cip_event_window_configure_t cewc;
			cewc.type = CIP_EVENT_WINDOW_CONFIGURE;
			cewc.wid = window->wid;
			cewc.x = rc.left;
			cewc.y = rc.top;
			cewc.width = rc.right - rc.left;
			cewc.height = rc.bottom - rc.top;
			cewc.bare = 1;
			cewc.above = 0;
			sendData(&cewc, sizeof(cewc));
		}
		ReleaseSRWLockShared(&windowsRWLock);
	}
	
}

void sendData(void *payload, int length)
{
	sendLock.lock();
	sendto(cliSocket, (char*)payload, length, 0, (sockaddr*)&sinaddr, sizeof(sockaddr_in));
	sendLock.unlock();
}

void SendDataWs(void *payload, int length)
{
	sendLock.lock();
	websocket_outgoing_message msg;
	concurrency::streams::producer_consumer_buffer<uint8_t> buf;
	std::vector<uint8_t> body(length);
	memcpy(&body[0], payload, length);

	auto send_task = buf.putn(&body[0], body.size()).then([&](size_t length) {
		msg.set_binary_message(buf.create_istream(), length);
		return client.send(msg);
	}).then([](pplx::task<void> t)
	{
		try {
			t.get();
		} catch (const websocket_exception& ex) {
			std::cout << ex.what();
		}
	});
	send_task.wait();
	sendLock.unlock();
}

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	int nArgs;
	LPWSTR *szArglist;
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	szCmdline = szArglist[1];
	token = szArglist[2];
	OutputDebugString(token);

	wstring tokenw(token);
	wstring addr = L"ws://10.10.219.132:9000/?type=server&token=" + tokenw;
	//wstring addr = L"ws://192.168.1.202:9999/?type=server&token=" + tokenw;
	client.connect(addr).then([]() {}).wait();

	client.set_message_handler([](websocket_incoming_message msg) {
		uint8_t bf[100];
		msg.body().streambuf().getn(bf, msg.body().streambuf().size());
		HandleEvent(bf);
	});

	InitializeSRWLock(&windowsRWLock);
	WCHAR filepath[128];
	memset(filepath, 0, sizeof(filepath));
	PWSTR path = NULL;
	SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path);
	wcscat_s(filepath, 128, path);
	wcscat_s(filepath, 128, _T("/pulsar.norun"));

	/* if pulsar.norun exists, exit pulsar */
	if (FileExists(filepath)) {
		return 0;
	}


	InitKeymap();

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PULSARWINDOWS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PULSARWINDOWS));

#ifndef DEV
	CreateThread(NULL, 0, ScreenStreamThread, NULL, 0, NULL);
#endif
	//CreateThread(NULL, 0, TickInfo, NULL, 0, NULL);
	//ScreenStreamThread(NULL);
	// begin dll injection
	HINSTANCE hDll = LoadLibrary(TEXT("hook.dll"));
	if (!hDll) {
		MessageBox(NULL, TEXT("cannot load hook.dll"), TEXT("error"), MB_OK);
		return 0;
	}

	INSTALLHOOK InstallHook = (INSTALLHOOK)GetProcAddress(hDll, "InstallHook");
	UNINSTALLHOOK UninstallHook = (UNINSTALLHOOK)GetProcAddress(hDll, "UninstallHook");

	BOOL ret = InstallHook(hWndServer, 0);
	if (!ret) {
		MessageBox(NULL, TEXT("dll inject fail"), TEXT("error"), MB_OK);
		return 0;
	}
	//changeResolution(500, 500);

	CreateThread(NULL, 0, RunCloudware, NULL, 0, NULL);
	//CreateThread(NULL, 0, SyncState, NULL, 0, NULL);

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		switch (msg.message) {
		case WM_APP + HCBT_CREATEWND: {
			Sleep(50);
			HWND hwnd = (HWND)msg.wParam;
			if (isTopWindow(hwnd)) {
				RECT rc;
				GetWindowRect(hwnd, &rc);
				
				cip_window_t *window = cip_window_create((uint32_t)hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, CIP_WINDOW_BARE);

				AcquireSRWLockExclusive(&windowsRWLock);
				windows[window->wid] = window;
				ReleaseSRWLockExclusive(&windowsRWLock);
				cip_event_window_create_t cewc;
				cewc.type = CIP_EVENT_WINDOW_CREATE;
				cewc.wid = window->wid;
				cewc.x = window->x;
				cewc.y = window->y;
				cewc.width = window->width;
				cewc.height = window->height;
				cewc.bare = 1;
				SendDataWs((char*)&cewc, sizeof(cewc));
				//DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
				if (IsWindowVisible(hwnd)) {
						window->visible = true;
						cip_event_window_show_t cews;
						cews.type = CIP_EVENT_WINDOW_SHOW;
						cews.wid = (uint32_t)hwnd;
						cews.bare = 1;
						SendDataWs(&cews, sizeof(cews));
				}
				
			}
			break;
		}
		case WM_APP + HCBT_DESTROYWND: {
			AcquireSRWLockShared(&windowsRWLock);
			if (windows.find((int)msg.wParam) == windows.end()) {
				ReleaseSRWLockShared(&windowsRWLock);
				break;
			}
			ReleaseSRWLockShared(&windowsRWLock);
			cip_window_t *window = windows[(int)msg.wParam];
			cip_event_window_destroy_t cewd;
			cewd.type = CIP_EVENT_WINDOW_DESTROY;
			cewd.wid = (u32)msg.wParam;
			SendDataWs(&cewd, sizeof(cewd));
			AcquireSRWLockExclusive(&windowsRWLock);
			windows.erase((int)msg.wParam);
			ReleaseSRWLockExclusive(&windowsRWLock);
			free(window);
			break;
		}
		case WM_APP + 0x4000 + WM_SHOWWINDOW: {
			HWND hwnd = (HWND)msg.wParam;
			AcquireSRWLockShared(&windowsRWLock);
			if (windows.find((int)msg.wParam) == windows.end()) {
				ReleaseSRWLockShared(&windowsRWLock);
				break;
			}
			ReleaseSRWLockShared(&windowsRWLock);
			cip_window_t *window = windows[(int)hwnd];
			BOOL visible = msg.lParam;
			window->visible = visible;
			if (visible) {
				cip_event_window_show_t cews;
				cews.type = CIP_EVENT_WINDOW_SHOW;
				cews.wid = (uint32_t)hwnd;
				cews.bare = 1;
				SendDataWs(&cews, sizeof(cews));
			} else {
				cip_event_window_hide_t cewh;
				cewh.type = CIP_EVENT_WINDOW_HIDE;
				cewh.wid = (uint32_t)hwnd;
				SendDataWs(&cewh, sizeof(cewh));
			}
			break;
		}
		case WM_APP + 0x4000 + WM_SIZE: {
			HWND hwnd = (HWND)msg.wParam;
			AcquireSRWLockShared(&windowsRWLock);
			if (windows.find((int)msg.wParam) == windows.end()) {
				ReleaseSRWLockShared(&windowsRWLock);
				break;
			}
			ReleaseSRWLockShared(&windowsRWLock);
			Sleep(30);
			cip_window_t *window = windows[(int)hwnd];
			RECT rc;
			GetWindowRect(hwnd, &rc);
			if (window->x == rc.left && window->y == rc.top && window->width == (rc.right - rc.left) && window->height == (rc.bottom - rc.top)) {
				break;
			}
			cip_event_window_configure_t cewc;
			cewc.type = CIP_EVENT_WINDOW_CONFIGURE;
			cewc.wid = (u32)hwnd;
			cewc.x = rc.left;
			cewc.y = rc.top;
			cewc.width = rc.right - rc.left;
			cewc.height = rc.bottom - rc.top;
			cewc.bare = 1;
			cewc.above = 0;
			SendDataWs(&cewc, sizeof(cewc));
			cip_window_t *win = windows[(u32)hwnd];
			if (win->width != cewc.width || win->height != cewc.height) {
				win->width = cewc.width;
				win->height = cewc.height;
			}
			break;
		}
		case WM_APP + 0x4000 + WM_WINDOWPOSCHANGED: {
			HWND hwnd = (HWND)msg.wParam;
			AcquireSRWLockShared(&windowsRWLock);
			if (windows.find((int)msg.wParam) == windows.end()) {
				ReleaseSRWLockShared(&windowsRWLock);
				break;
			}
			ReleaseSRWLockShared(&windowsRWLock);
			Sleep(20);
			cip_window_t *window = windows[(int)hwnd];
			RECT rc;
			GetWindowRect(hwnd, &rc);
			if (window->x == rc.left && window->y == rc.top && window->width == (rc.right - rc.left) && window->height == (rc.bottom - rc.top)) {
				break;
			}
			cip_event_window_configure_t cewc;
			cewc.type = CIP_EVENT_WINDOW_CONFIGURE;
			cewc.wid = (u32)msg.wParam;
			cewc.x = rc.left;
			cewc.y = rc.top;
			cewc.width = rc.right - rc.left;
			cewc.height = rc.bottom - rc.top;
			cewc.bare = 1;
			cewc.above = 0;
			SendDataWs(&cewc, sizeof(cewc));
			windows[(int)msg.wParam]->x = rc.left;
			windows[(int)msg.wParam]->y = rc.top;
			break;
		}
		case WM_APP + 0x3FFF: { // cursor style change event

			// Get information about the global cursor.
			CURSORINFO ci;
			ci.cbSize = sizeof(ci);
			GetCursorInfo(&ci);
			ICONINFO iconInfo;
			GetIconInfo(ci.hCursor, &iconInfo);

			PICTDESC pd;

			pd.cbSizeofstruct = sizeof(PICTDESC);
			pd.picType = PICTYPE_ICON;
			pd.icon.hicon = ci.hCursor;

			LPPICTURE picture;
			HRESULT res = OleCreatePictureIndirect(&pd, IID_IPicture, false,
				reinterpret_cast<void**>(&picture));

			if (!SUCCEEDED(res))
				break;

			LPSTREAM stream;
			res = CreateStreamOnHGlobal(0, true, &stream);

			if (!SUCCEEDED(res)) {
				picture->Release();
				break;
			}

			LONG bytes_streamed;
			res = picture->SaveAsFile(stream, true, &bytes_streamed);


			if (!SUCCEEDED(res)) {
				stream->Release();
				picture->Release();
				break;
			}

			HGLOBAL mem = 0;
			GetHGlobalFromStream(stream, &mem);
			LPVOID data = GlobalLock(mem);

			char *payload = (char*)malloc(3 + bytes_streamed);
			payload[0] = CIP_EVENT_CURSOR;
			payload[1] = iconInfo.xHotspot;
			payload[2] = iconInfo.yHotspot;
			memcpy(&payload[3], data, bytes_streamed);
			SendDataWs(payload, 3 + bytes_streamed);
			free(payload);

			GlobalUnlock(mem);

			stream->Release();
			picture->Release();

			break;
		}
		default:
			break;
		}
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PULSARWINDOWS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PULSARWINDOWS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 500, 100, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   // 主窗口赋给全局变量
   hWndServer = hWnd;
   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择: 
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

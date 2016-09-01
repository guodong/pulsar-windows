// pulsar-windows.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "pulsar-windows.h"
#include <WinSock2.h>
#include <list>
#include <map>
#include "cip_window.h"
#include "cip_protocol.h"
#include <x264.h>
#include <stdint.h>
#include <mutex>

#define uint8 uint8_t

#pragma comment(lib, "Ws2_32.lib")

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND hWndServer;								// 接受hook消息的主窗口句柄
typedef std::list<sockaddr_in> CLIENTS;
CLIENTS clients;
std::map<int, cip_window_t*> windows;
SOCKET serSocket;
std::mutex sendLock;
BOOL forceKeyFrame = false;
std::map<uint8_t, uint8_t> keymap;


// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
typedef BOOL(CALLBACK *INSTALLHOOK)(HWND, DWORD);
typedef BOOL(CALLBACK *UNINSTALLHOOK)();
void sendData(void *payload, int length);

void InitKeymap()
{
	
}

/* check whether window is top level */
BOOL isTopWindow(HWND hwnd)
{
	if (hwnd == GetAncestor(hwnd, GA_ROOT)) {
		return true;
	}
	HWND parent = GetAncestor(hwnd, GA_PARENT);
	return (parent == GetDesktopWindow());
}

int CreateUDPServer()
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		return 1;
	}

	serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serSocket == INVALID_SOCKET) {
		WSACleanup();
		return 1;
	}

	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(24055);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
		closesocket(serSocket);
		WSACleanup();
		return 1;
	}

	sockaddr_in info;
	memset(&info, 0, sizeof(info));
	int info_len = sizeof(info);
	getsockname(serSocket, (sockaddr*)&info, &info_len);

	int a = ntohs(info.sin_port);
	wchar_t buffer[256];
	wsprintfW(buffer, L"%d", a);
	//MessageBoxW(nullptr, buffer, buffer, MB_OK);

	sockaddr_in cliAddr;
	int addrLen = sizeof(cliAddr);
	while (true) {
		char data[255];
		memset(data, 0, 255);
		int len = recvfrom(serSocket, data, 255, 0, (sockaddr*)&cliAddr, &addrLen);
		//MessageBox(NULL, _T("ok"), _T("i"), MB_OK);
		if (strcmp(data, "listen") == 0) {
			forceKeyFrame = true;
			clients.push_back(cliAddr);
			continue;
		}
		switch (data[0]) {
		case CIP_EVENT_MOUSE_MOVE: {
			cip_event_mouse_move_t *cemm = (cip_event_mouse_move_t*)data;
			SetCursorPos(cemm->x, cemm->y);
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
			input.ki.dwFlags = KEYEVENTF_SCANCODE;
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
			input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
			input.ki.wScan = 0;
			SendInput(1, &input, sizeof(input));
			break;
		}
		default:
			break;
		}

	}
}

DWORD WINAPI ServerThread(LPVOID lp)
{
	CreateUDPServer();
	return 0;
}

void toeven(size_t *num)
{
	if (*num % 4) {
		*num = *num - (*num % 4);
	}
}


static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
	return (66 * r + 129 * g + 25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
	return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
	return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}


#define MAKEROWY(NAME, R, G, B, BPP) \
void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
int x;                                                                       \
for (x = 0; x < width; ++x) {                                                \
dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
src_argb0 += BPP;                                                          \
dst_y += 1;                                                                \
}                                                                            \
}                                                                              \
void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
uint8* dst_u, uint8* dst_v, int width) {                \
const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
int x;                                                                       \
for (x = 0; x < width - 1; x += 2) {                                         \
uint8 ab = (src_rgb0[B] + src_rgb0[B + BPP] +                              \
src_rgb1[B] + src_rgb1[B + BPP]) >> 2;                          \
uint8 ag = (src_rgb0[G] + src_rgb0[G + BPP] +                              \
src_rgb1[G] + src_rgb1[G + BPP]) >> 2;                          \
uint8 ar = (src_rgb0[R] + src_rgb0[R + BPP] +                              \
src_rgb1[R] + src_rgb1[R + BPP]) >> 2;                          \
dst_u[0] = RGBToU(ar, ag, ab);                                             \
dst_v[0] = RGBToV(ar, ag, ab);                                             \
src_rgb0 += BPP * 2;                                                       \
src_rgb1 += BPP * 2;                                                       \
dst_u += 1;                                                                \
dst_v += 1;                                                                \
}                                                                            \
if (width & 1) {                                                             \
uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
dst_u[0] = RGBToU(ar, ag, ab);                                             \
dst_v[0] = RGBToV(ar, ag, ab);                                             \
}                                                                            \
}

MAKEROWY(ARGB, 2, 1, 0, 4)

void ARGBToI420x(const uint8_t* src_argb,
	uint8_t* dst_y,
	uint8_t* dst_u,
	uint8_t* dst_v,
	int width, int height)
{
	int x, y;
	for (y = 0; y < height - 1; y += 2) {
		for (x = 0; x < width - 1; x += 2) {

		}
	}
}

int ARGBToI420(const uint8_t* src_argb, int src_stride_argb,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	int y;
	void(*ARGBToUVRow)(const uint8_t* src_argb0, int src_stride_argb,
		uint8* dst_u, uint8_t* dst_v, int width) = ARGBToUVRow_C;
	void(*ARGBToYRow)(const uint8* src_argb, uint8_t* dst_y, int pix) =
		ARGBToYRow_C;
	if (!src_argb ||
		!dst_y || !dst_u || !dst_v ||
		width <= 0 || height == 0) {
		return -1;
	}
	// Negative height means invert the image.
	if (height < 0) {
		height = -height;
		src_argb = src_argb + (height - 1) * src_stride_argb;
		src_stride_argb = -src_stride_argb;
	}

	for (y = 0; y < height - 1; y += 2) {
		ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
		ARGBToYRow(src_argb, dst_y, width);
		ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
		src_argb += src_stride_argb * 2;
		dst_y += dst_stride_y * 2;
		dst_u += dst_stride_u;
		dst_v += dst_stride_v;
	}
	if (height & 1) {
		ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
		ARGBToYRow(src_argb, dst_y, width);
	}
	return 0;
}



RECT getDamageRect()
{
	RECT rc;
	memset(&rc, 0, sizeof(rc));
	rc.left = 10000;
	rc.top = 10000;
	rc.right = 0;
	rc.bottom = 0;
	std::map<int, cip_window_t*>::iterator it;
	for (it = windows.begin(); it != windows.end(); it++) {
		if (!it->second->visible) {
			continue;
		}
		if (it->second->x < rc.left) {
			rc.left = it->second->x;
		}
		if (it->second->y < rc.top) {
			rc.top = it->second->y;
		}
		if (it->second->x + it->second->width > rc.right) {
			rc.right = it->second->x + it->second->width;
		}
		if (it->second->y + it->second->height > rc.bottom) {
			rc.bottom = it->second->y + it->second->height;
		}
	}

	/* no visible windows */
	if (rc.left > rc.right) {
		rc.left = rc.right = rc.top = rc.bottom = 0;
	}
	return rc;
}

int RGB2YUV_YR[256], RGB2YUV_YG[256], RGB2YUV_YB[256];

int RGB2YUV_UR[256], RGB2YUV_UG[256], RGB2YUV_UBVR[256];

int RGB2YUV_VG[256], RGB2YUV_VB[256];

void InitLookupTable()

{

	int i;

	for (i = 0; i < 256; i++) RGB2YUV_YR[i] = (float)65.481 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_YG[i] = (float)128.553 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_YB[i] = (float)24.966 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_UR[i] = (float)37.797 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_UG[i] = (float)74.203 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_VG[i] = (float)93.786 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_VB[i] = (float)18.214 * (i << 8);

	for (i = 0; i < 256; i++) RGB2YUV_UBVR[i] = (float)112 * (i << 8);

}

int ConvertRGB2YUV(int w, int h, unsigned char *bmp, uint8_t *yuv)

{

	uint8_t *u, *v, *y, *uu, *vv;

	uint8_t *pu1, *pu2, *pu3, *pu4;

	uint8_t *pv1, *pv2, *pv3, *pv4;

	unsigned char *r, *g, *b;

	int i, j;

	uu = new uint8_t[w*h];

	vv = new uint8_t[w*h];

	if (uu == NULL || vv == NULL)

		return 0;

	y = yuv;

	u = uu;

	v = vv;

	// Get r,g,b pointers from bmp image data....

	r = bmp;

	g = bmp + 1;

	b = bmp + 2;

	//Get YUV values for rgb values...

	for (i = 0; i<h; i++)

	{

		for (j = 0; j<w; j++)

		{

			*y++ = (RGB2YUV_YR[*r] + RGB2YUV_YG[*g] + RGB2YUV_YB[*b] + 1048576) >> 16;

			*u++ = (-RGB2YUV_UR[*r] - RGB2YUV_UG[*g] + RGB2YUV_UBVR[*b] + 8388608) >> 16;

			*v++ = (RGB2YUV_UBVR[*r] - RGB2YUV_VG[*g] - RGB2YUV_VB[*b] + 8388608) >> 16;

			r += 3;

			g += 3;

			b += 3;

		}

	}

	// Now sample the U & V to obtain YUV 4:2:0 format

	// Sampling mechanism...

	// Get the right pointers...

	u = yuv + w*h;

	v = u + (w*h) / 4;

	// For U

	pu1 = uu;

	pu2 = pu1 + 1;

	pu3 = pu1 + w;

	pu4 = pu3 + 1;

	// For V

	pv1 = vv;

	pv2 = pv1 + 1;

	pv3 = pv1 + w;

	pv4 = pv3 + 1;

	// Do sampling....

	for (i = 0; i<h; i += 2)
	{

		for (j = 0; j<w; j += 2)

		{

			*u++ = (*pu1 + *pu2 + *pu3 + *pu4) >> 2;

			*v++ = (*pv1 + *pv2 + *pv3 + *pv4) >> 2;

			pu1 += 2;

			pu2 += 2;

			pu3 += 2;

			pu4 += 2;

			pv1 += 2;

			pv2 += 2;

			pv3 += 2;

			pv4 += 2;

		}

		pu1 += w;

		pu2 += w;

		pu3 += w;

		pu4 += w;

		pv1 += w;

		pv2 += w;

		pv3 += w;

		pv4 += w;

	}

	delete uu;

	delete vv;

	return 1;

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

	x264_param_t param;
	x264_param_default_preset(&param, "veryfast", "zerolatency");
	param.i_csp = X264_CSP_I420;
	param.i_width = width;
	param.i_height = height;
	param.i_slice_max_size = 65000;

	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.rc.f_rf_constant = 24;

	if (x264_param_apply_profile(&param, "baseline") < 0) {
		return 1;
	}

	x264_picture_t pic;
	if (x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height) < 0) {
		return 1;
	}

	x264_t *encoder = x264_encoder_open(&param);
	if (!encoder) {
		goto fail2;
	}

	HDC desktopDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(desktopDC);
	HBITMAP hbmp = CreateCompatibleBitmap(desktopDC, width, height);
	SelectObject(memDC, hbmp);
	BYTE *data;
	data = (BYTE*)malloc(4 * width * height);
	x264_nal_t *nal = NULL;
	int32_t i_nal = 0;
	x264_picture_t picout;
	char *buf = (char*)malloc(70000);
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
		ARGBToI420(data, width * 4,
			pic.img.plane[0], width,
			pic.img.plane[1], width / 2,
			pic.img.plane[2], width / 2,
			width, height);
		//ConvertRGB2YUV(width, height, data, pic.img.plane[0]);

		if (forceKeyFrame) {
			pic.i_type = X264_TYPE_KEYFRAME;
		}
		pic.i_pts++;


		int i_frame_size = x264_encoder_encode(encoder, &nal, &i_nal, &pic, &picout);
		if (i_frame_size < 0) {
			return 1;
		}
		int i;
		for (i = 0; i < i_nal; ++i) {
			/* broadcast event */
			int length = sizeof(cip_event_window_frame_ws_t) + nal[i].i_payload;
			cip_event_window_frame_ws_t *p = (cip_event_window_frame_ws_t*)buf;
			p->type = CIP_EVENT_WINDOW_FRAME;
			p->wid = 0;
			memcpy(buf + sizeof(cip_event_window_frame_ws_t), nal[i].p_payload, nal[i].i_payload);
			sendData(buf, length);
		}
		pic.i_type = X264_TYPE_AUTO;
		Sleep(40);
	}

	free(buf);
	free(data);
	return 0;
fail2:
	x264_picture_clean(&pic);
	return 1;
}

void sendData(void *payload, int length)
{
	sendLock.lock();
	CLIENTS::iterator i;
	for (i = clients.begin(); i != clients.end(); i++) {
		sendto(serSocket, (char*)payload, length, 0, (sockaddr*)&(*i), sizeof(sockaddr_in));
	}
	sendLock.unlock();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

	InitKeymap();
	InitLookupTable();

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

	CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
	CreateThread(NULL, 0, ScreenStreamThread, NULL, 0, NULL);
	//CreateThread(NULL, 0, SyncState, NULL, 0, NULL);

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

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		switch (msg.message) {
		case WM_APP + HCBT_CREATEWND: {
			Sleep(30);
			HWND hwnd = (HWND)msg.wParam;
			if (isTopWindow(hwnd)) {
				RECT rc;
				GetWindowRect(hwnd, &rc);
				
				cip_window_t *window = cip_window_create((uint32_t)hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, CIP_WINDOW_BARE);
				windows[window->wid] = window;
				cip_event_window_create_t cewc;
				cewc.type = CIP_EVENT_WINDOW_CREATE;
				cewc.wid = window->wid;
				cewc.x = window->x;
				cewc.y = window->y;
				cewc.width = window->width;
				cewc.height = window->height;
				cewc.bare = 1;
				sendData((char*)&cewc, sizeof(cewc));
				DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
				if (IsWindowVisible(hwnd)) {
						window->visible = true;
						cip_event_window_show_t cews;
						cews.type = CIP_EVENT_WINDOW_SHOW;
						cews.wid = (uint32_t)hwnd;
						cews.bare = 1;
						sendData(&cews, sizeof(cews));
				}
				//forceKeyFrame = true;
				
			}
			break;
		}
		case WM_APP + HCBT_DESTROYWND: {
			if (windows.find((int)msg.wParam) == windows.end()) {
				break;
			}
			cip_window_t *window = windows[(int)msg.wParam];
			cip_event_window_destroy_t cewd;
			cewd.type = CIP_EVENT_WINDOW_DESTROY;
			cewd.wid = (u32)msg.wParam;
			sendData(&cewd, sizeof(cewd));
			windows.erase((int)msg.wParam);
			free(window);
			break;
		}
		case WM_APP + 0x4000 + WM_SHOWWINDOW: {
			
			HWND hwnd = (HWND)msg.wParam;
			if (windows.find((int)msg.wParam) == windows.end()) {
				break;
			}
			cip_window_t *window = windows[(int)hwnd];
			BOOL visible = msg.lParam;// IsWindowVisible(hwnd);
			window->visible = visible;
			if (visible) {
				cip_event_window_show_t cews;
				cews.type = CIP_EVENT_WINDOW_SHOW;
				cews.wid = (uint32_t)hwnd;
				cews.bare = 1;
				sendData(&cews, sizeof(cews));
			} else {
				cip_event_window_hide_t cewh;
				cewh.type = CIP_EVENT_WINDOW_HIDE;
				cewh.wid = (uint32_t)hwnd;
				sendData(&cewh, sizeof(cewh));
			}
			break;
		}
		case WM_APP + 0x4000 + WM_SIZE: {
			HWND hwnd = (HWND)msg.wParam;
			if (windows.find((int)hwnd) == windows.end()) {
				break;
			}

			//Sleep(100);
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
			sendData(&cewc, sizeof(cewc));
			cip_window_t *win = windows[(u32)hwnd];
			if (win->width != cewc.width || win->height != cewc.height) {
				win->width = cewc.width;
				win->height = cewc.height;
			}
			break;
		}
		case WM_APP + 0x4000 + WM_WINDOWPOSCHANGED: {
			HWND hwnd = (HWND)msg.wParam;
			if (windows.find((int)hwnd) == windows.end()) {
				break;
			}
			//Sleep(20);
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
			sendData(&cewc, sizeof(cewc));
			windows[(int)msg.wParam]->x = rc.left;
			windows[(int)msg.wParam]->y = rc.top;
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

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

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

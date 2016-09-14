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
#include <ShlObj.h>

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
typedef std::list<sockaddr_in> CLIENTS;
CLIENTS clients;

SRWLOCK windowsRWLock;
std::map<int, cip_window_t*> windows;
SOCKET serSocket;
std::mutex sendLock;
BOOL forceKeyFrame = false;
std::map<uint8_t, uint8_t> keymap;
LPTSTR szCmdline = _tcsdup(TEXT("C:\\Windows\\Notepad.exe"));
int udp_port = 0; //udp server port


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
	keymap[27] = 0x1B;      //  ESC
	keymap[112] = 0x70;     //  F1
	keymap[113] = 0x71;     //  F2
	keymap[114] = 0x72;      // F3
	keymap[115] = 0x73;     //  F4
	keymap[116] = 0x74;      // F5
	keymap[117] = 0x75;    //   F6
	keymap[118] = 0x76;      // F7
	keymap[119] = 0x77;     //  F8
	keymap[120] = 0x78;      // F9
	keymap[121] = 0x79;     //  F10
	keymap[122] = 0x7A;    //   F11
	keymap[123] = 0x7B;     // F12



	keymap[192] = 0xC0;       //  `
	keymap[49] = 0x31;        // 1
	keymap[50] = 0x32;        // 2
	keymap[51] = 0x33;       //  3
	keymap[52] = 0x34;        // 4
	keymap[53] = 0x35;        //5
	keymap[54] = 0x36;       //  6
	keymap[55] = 0x37;      //  7
	keymap[56] = 0x38;       //  8
	keymap[57] = 0x39;    //    9
	keymap[48] = 0x30;         //0 
	keymap[189] = 0xBD;      //-
	keymap[187] = 0xBB;     //+
	keymap[8] = 0x08;     //delete


	keymap[9] = 0x09;         // Tab;
	keymap[81] = 0x51;       //    Q
	keymap[87] = 0x57;        //   W
	keymap[69] = 0x45;         //  E
	keymap[82] = 0x52;        //   R
	keymap[84] = 0x54;        //   T
	keymap[89] = 0x59;        //   Y
	keymap[85] = 0x55;         //  U
	keymap[73] = 0x49;        //   I
	keymap[79] = 0x4F;        //   O
	keymap[80] = 0x50;        //   P
	keymap[219] = 0xDB;     //     [ 
	keymap[221] = 0xDD;       //   ]
	keymap[13] = 0x0D;        // Enter

	keymap[20] = 0x14;     // Caps Lock
	keymap[65] = 0x41;     //  A
	keymap[83] = 0x53;      // S
	keymap[68] = 0x44;   //    D
	keymap[70] = 0x46;       //F
	keymap[71] = 0x47;      // G
	keymap[72] = 0x48;     //  H
	keymap[74] = 0x4A;      // J
	keymap[75] = 0x4B;      // K
	keymap[76] = 0x4C;    //   L
	keymap[186] = 0xBA;    //   ;
	keymap[222] = 0xDE;   //    " 
	keymap[220] = 0xDC;    //   \

	keymap[16] = 0x10;         //   SHIFT key
	keymap[90] = 0x5A;        //   Z
	keymap[88] = 0x58;         //  X
	keymap[67] = 0x43;         //  C
	keymap[86] = 0x56;        //   V
	keymap[66] = 0x42;        //   B
	keymap[78] = 0x4E;         //  N
	keymap[77] = 0x4D;       //   M
	keymap[188] = 0xBC;     //   <
	keymap[190] = 0xBE;      //   >
	keymap[191] = 0xBF;      //   ?
	keymap[16] = 0x10;          // SHIFT key

	keymap[17] = 0x11;      //  CTRL key
	keymap[91] = 0xA4;       //Left MENU
	keymap[18] = 0x12;        //ALT key
	keymap[32] = 0x20;      //  Spacebar
	keymap[18] = 0x12;    //    ALT key
	keymap[92] = 0xA5;     //  Right MENU key
	keymap[93] = 0x02;    //  Right mouse button
	keymap[17] = 0x11;     //   CTRL key

	keymap[145] = 0x91;
	keymap[19] = 0;
	keymap[144] = 0x90;
	keymap[111] = 0xBF;
	keymap[106] = 0x6A;
	keymap[109] = 0xBD;
	keymap[45] = 0x2D;
	keymap[36] = 0x24;
	keymap[33] = 0x21;
	keymap[103] = 0x67;
	keymap[104] = 0x68;
	keymap[105] = 0x69;
	keymap[107] = 0xBB;
	keymap[46] = 0x2E;
	keymap[35] = 0x23;
	keymap[34] = 0x22;
	keymap[100] = 0x64;
	keymap[101] = 0x65;
	keymap[102] = 0x66;
	keymap[38] = 0x26;
	keymap[97] = 0x61;
	keymap[98] = 0x62;
	keymap[99] = 0x63;
	keymap[13] = 0x0D;
	keymap[37] = 0x25;
	keymap[40] = 0x28;
	keymap[39] = 0x27;
	keymap[96] = 0x60;
	keymap[110] = 0xBE;
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

void recover(sockaddr_in client)
{
	std::map<int, cip_window_t*>::iterator it;
	for (it = windows.begin(); it != windows.end(); it++) {
		cip_window_t *window = it->second;
		cip_event_window_create_t cewc;
		cewc.type = CIP_EVENT_WINDOW_CREATE;
		cewc.wid = window->wid;
		cewc.x = window->x;
		cewc.y = window->y;
		cewc.width = window->width;
		cewc.height = window->height;
		cewc.bare = 1;
		sendData((char*)&cewc, sizeof(cewc));
		//DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		if (IsWindowVisible((HWND)window->wid)) {
			window->visible = true;
			cip_event_window_show_t cews;
			cews.type = CIP_EVENT_WINDOW_SHOW;
			cews.wid = window->wid;
			cews.bare = 1;
			sendData(&cews, sizeof(cews));
		}
	}
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
	serAddr.sin_port = 0; // htons(24055);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
		closesocket(serSocket);
		WSACleanup();
		return 1;
	}

	/* get udp server random port*/
	sockaddr_in info;
	memset(&info, 0, sizeof(info));
	int info_len = sizeof(info);
	getsockname(serSocket, (sockaddr*)&info, &info_len);

	/* set udp port to global varible */
	udp_port = ntohs(info.sin_port);

	/* write port info to file c:/users/%user%/pulsar-port.txt */
	WCHAR filepath[128];
	memset(filepath, 0, sizeof(filepath));
	PWSTR path = NULL;
	SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path);
	wcscat_s(filepath, 128, path);
	wcscat_s(filepath, 128, _T("/pulsar-port.txt"));

	HANDLE fp = CreateFile(filepath, GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	char buf[10];
	memset(buf, 0, 10);
	_itoa_s(udp_port, buf, 10);
	DWORD written = 0;
	WriteFile(fp, buf, strlen(buf), &written, NULL);
	CloseHandle(fp);

	sockaddr_in cliAddr;
	int addrLen = sizeof(cliAddr);
	while (true) {
		char data[255];
		memset(data, 0, 255);
		int len = recvfrom(serSocket, data, 255, 0, (sockaddr*)&cliAddr, &addrLen);
		//MessageBox(NULL, _T("ok"), _T("i"), MB_OK);
		if (strcmp(data, "listen") == 0) {
			clients.push_back(cliAddr);
			recover(cliAddr);
			forceKeyFrame = true;
			continue;
		}
		switch (data[0]) {
		case CIP_EVENT_MOUSE_MOVE: {
			cip_event_mouse_move_t *cemm = (cip_event_mouse_move_t*)data;
			// do not use SetCursorPos, it will conflict with each user
			// SetCursorPos(cemm->x, cemm->y);
			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi.mouseData = 0;
			input.mi.dx = cemm->x * (65536 / GetSystemMetrics(SM_CXSCREEN));//x being coord in pixels
			input.mi.dy = cemm->y * (65536 / GetSystemMetrics(SM_CYSCREEN));//y being coord in pixels
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

/* streaming screen image to h.264 stream */
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
	x264_param_default_preset(&param, "slow", "zerolatency");
	param.i_csp = X264_CSP_I420;
	param.i_width = width;
	param.i_height = height;
	param.i_slice_max_size = 65000;
	param.i_threads = 0;
	param.i_keyint_max = 30;
	

	param.b_vfr_input = 0;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.rc.f_rf_constant = 28;
	param.rc.f_rf_constant_max = 50;
	param.rc.i_rc_method = X264_RC_CRF;
	param.rc.i_vbv_max_bitrate = 1800;
	param.rc.i_bitrate = 1500;
	//param.b_intra_refresh = 1;
	param.rc.i_qp_max = 40;
	param.rc.i_qp_constant = 25;
	/*param.i_fps_num = 10;
	param.i_fps_den = 1;
	param.i_timebase_num = 1;
	param.i_timebase_den = 10;*/


	if (x264_param_apply_profile(&param, "baseline") < 0) {
		return 1;
	}

	x264_picture_t pic;
	if (x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height) < 0) {
		return 1;
	}
	pic.i_pts = 0;

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

	SwsContext *ctx = sws_getContext(width, height, AV_PIX_FMT_RGB32, width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	AVPicture pFrameYUV, pFrameRGB;

	avpicture_fill(&pFrameYUV, pic.img.plane[0], AV_PIX_FMT_YUV420P, width, height);
	const int inLinesize[1] = { 4 * width };
	while (true) {
		if (clients.empty()) {
			Sleep(1000);
			continue;
		}
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

		/*ARGBToI420(data, width * 4,
			pic.img.plane[0], width,
			pic.img.plane[1], width / 2,
			pic.img.plane[2], width / 2,
			width, height);*/

		if (forceKeyFrame) {
			pic.i_type = X264_TYPE_KEYFRAME;
		}


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
		pic.i_pts++;
		pic.i_type = X264_TYPE_AUTO;
		//Sleep(50);
	}
	sws_freeContext(ctx);
	free(buf);
	free(data);
	return 0;
fail2:
	x264_picture_clean(&pic);
	return 1;
}

DWORD WINAPI RunCloudware(LPVOID lp)
{
	Sleep(1000); // wait for main window show
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (_tcscmp(szCmdline, _T("iexplore")) == 0) {
		WCHAR cmd[] = _T("c:\\Program Files\\Internet Explorer\\iexplore.exe");
		if (!CreateProcess(NULL,   // No module name (use command line)
			cmd,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi)) {
			goto err;
		}
	} else {
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
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

err:
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	cip_event_exit_t ext;
	ext.type = CIP_EVENT_EXIT;
	sendData(&ext, sizeof(ext));

	/* logout user */
	ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_FLAG_PLANNED);

	/* TODO: clean memory and exit*/
	exit(0);
}


DWORD WINAPI SyncState(LPVOID lp)
{
	while (true) {
		Sleep(10000);
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
			//Sleep(1000);
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
	CLIENTS::iterator i;
	for (i = clients.begin(); i != clients.end(); i++) {
		sendto(serSocket, (char*)payload, length, 0, (sockaddr*)&(*i), sizeof(sockaddr_in));
	}
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

    // TODO: 在此放置代码。
	//Sleep(3000); // wait for desktop init
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
	if (_tcscmp(lpCmdLine, _T("")) != 0) {
		szCmdline = lpCmdLine;
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

	CreateThread(NULL, 0, RunCloudware, NULL, 0, NULL);
	CreateThread(NULL, 0, SyncState, NULL, 0, NULL);

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		switch (msg.message) {
		case WM_APP + HCBT_CREATEWND: {
			Sleep(100);
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
				sendData((char*)&cewc, sizeof(cewc));
				//DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
				if (IsWindowVisible(hwnd)) {
						window->visible = true;
						cip_event_window_show_t cews;
						cews.type = CIP_EVENT_WINDOW_SHOW;
						cews.wid = (uint32_t)hwnd;
						cews.bare = 1;
						sendData(&cews, sizeof(cews));
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
			sendData(&cewd, sizeof(cewd));
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
			AcquireSRWLockShared(&windowsRWLock);
			if (windows.find((int)msg.wParam) == windows.end()) {
				ReleaseSRWLockShared(&windowsRWLock);
				break;
			}
			ReleaseSRWLockShared(&windowsRWLock);
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

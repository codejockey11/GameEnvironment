#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#define CURL_STATICLIB
#define _USE_MATH_DEFINES

#define MYMAKEFOURCC(str) ((DWORD)(BYTE)(str[0]) | ((DWORD)(BYTE)(str[1]) << 8) | ((DWORD)(BYTE)(str[2]) << 16) | ((DWORD)(BYTE)(str[3]) << 24 ))

#include <atlbase.h>
#include <comdef.h>
#include <CommCtrl.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <D3d12SDKLayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dwrite.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <ExDisp.h>
#include <iphlpapi.h>
#include <locale.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <Mmdeviceapi.h>
#include <mmsystem.h>
#include <mysql.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>
#include <wininet.h>
#include <winsock2.h>
#include <wrl.h>
#include <wrl/client.h>
#include <ws2tcpip.h>
#include <X3DAudio.h>
#include <xapofx.h>
#include <XAudio2.h>
#include <XAudio2fx.h>

#include <zlib.h>

#include <curl\curl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#include <Functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "msxml6.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Ws2_32.lib")

#pragma comment(lib, "XAudio2.lib")

#pragma comment(lib, "libmysql.lib")

#pragma comment(lib, "libcurl.lib")

#pragma comment(lib, "zlibd.lib")

#define SAFE_DELETE(p) { delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p) { delete[] p; p = nullptr; }

// 包含基本窗体程序头文件
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdint.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

const uint32_t SCREEN_WIDTH = 960;
const uint32_t SCREEN_HEIGHT = 480;

// 全局声明
IDXGISwapChain			*g_pSwapchain = nullptr;		// 交换链接口指针
ID3D11Device			*g_pDev		  = nullptr;		// Direct3D设备接口
ID3D11DeviceContext		*g_pDevcon	  = nullptr;		// Direct3D设备上下文

ID3D11RenderTargetView 	*g_pRTView	  = nullptr;

ID3D11InputLayout		*g_pLayout 	  = nullptr;		// 输入布局指针
ID3D11VertexShader		*g_pVS		  = nullptr;		// 顶点着色指针
ID3D11PixelShader		*g_pPS		  = nullptr;		// 像素着色指针

ID3D11Buffer			*g_pVBuffer	  = nullptr;		// 顶点缓存

// 顶点缓存结构
struct VERTEX
{
	XMFLOAT3	Position;
	XMFLOAT4	Color;
};

template<class T>
inline void SafeRelease(T **ppInterfaceToRelease)
{
	if(*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease) -> Release();

		(*ppInterfaceToRelease) = nullptr;
	}
}

void CreateRenderTarget()
{
	HRESULT hr;
	ID3D11Texture2D *pBackBuffer;

	// 获取返回缓存指针
	g_pSwapchain -> GetBuffer(0, _uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// 创建一个渲染目标视图
	g_pDev -> CreateRenderTargetView(pBackBuffer, NULL, &g_pRTView);

	pBackBuffer -> Release();

	// 绑定视图
	g_pDevcon -> OMSetRenderTargets(1, &g_pRTView, NULL);
}

void SetViewPort()
{
	D3D11_VIEWPORT viewPort;
	ZeroMemory(&viewPort, sizeof(D3D11_VIEWPORT));

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = SCREEN_WIDTH;
	viewPort.Height = SCREEN_HEIGHT;

	g_pDevcon -> RSSetViewports(1, &viewPort);
}

// 加载与准备着色的函数
void InitPipeline()
{
	// 加载并编译两种着色
	ID3DBlob *VS, *PS;

	D3DReadFileToBlob(L"copy.vso", &VS);
	D3DReadFileToBlob(L"copy.pso", &PS);

	// 将着色层包装成着色对象
	g_pDev -> CreateVertexShader(VS -> GetBufferPointer(), VS -> GetBufferSize(), NULL, &g_pVS);
	g_pDev -> CreatePixelShader(PS -> GetBufferPointer(), PS -> GetBufferSize(), NULL, &g_pPS);

	// 设置着色对象
	g_pDevcon -> VSSetShader(g_pVS, 0, 0);
	g_pDevcon -> PSSetShader(g_pPS, 0, 0);

	// 创建输入布局对象
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	g_pDev -> CreateInputLayout(ied, 2, VS -> GetBufferPointer(), VS -> GetBufferSize(), &g_pLayout);
	g_pDevcon -> IASetInputLayout(g_pLayout);

	VS -> Release();
	PS -> Release();
}

// 创建图形以进行渲染的程序
void InitGraphics()
{
	// 使用 VERTEX 结构体创建一个三角形
	VERTEX OurVertices[] =
	{
		{ XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.45f, -0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.45f, -0.5f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	// 创建一个顶点缓存
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;				// 由 CPU 和 GPU 进行写操作
	bd.ByteWidth = sizeof(VERTEX) * 3;			// 三个 VERTEX 结构体的大小
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// 使用顶点缓存
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	// 允许 CPU 写入缓存

	g_pDev -> CreateBuffer(&bd, NULL, &g_pVBuffer);	// 创建缓存

	// 将顶点复制到缓存中
	D3D11_MAPPED_SUBRESOURCE ms;
	g_pDevcon -> Map(g_pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);		// 映射缓存
	memcpy(ms.pData, OurVertices, sizeof(VERTEX) * 3);							// 复制数据
	g_pDevcon -> Unmap(g_pVBuffer, NULL);										// 取消映射缓存
}

// 准备待使用的图形资源的函数
HRESULT CreateGraphicsResources(HWND hWnd)
{
	HRESULT hr = S_OK;

	if(g_pSwapchain == nullptr)
	{
		// 创建一个结构体保存交换链的相关信息
		DXGI_SWAP_CHAIN_DESC scd;

		// 清空使用的结构体
		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

		// 填充交换链描述结构体
		scd.BufferCount = 1;								// 后端缓存
		scd.BufferDesc.Width = SCREEN_WIDTH;
		scd.BufferDesc.Height = SCREEN_HEIGHT;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 使用32位颜色
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 指定交换链使用的方式
		scd.OutputWindow = hWnd;							// 指定使用的窗体
		scd.SampleDesc.Count = 4;							// 指定多重采样的数量
		scd.Windowed = TRUE;								// 窗口/全屏模式
		scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// 允许全屏切换

		const D3D_FEATURE_LEVEL FeatureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};

		D3D_FEATURE_LEVEL FeatureLevelSupported;

		HRESULT hr = S_OK;

		// 创建一个设备，设备上下文使用 scd 结构体中保存的数据
		hr = D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			0,
			FeatureLevels,
			_countof(FeatureLevels),
			D3D11_SDK_VERSION,
			&scd,
			&g_pSwapchain,
			&g_pDev,
			&FeatureLevelSupported,
			&g_pDevcon
		);

		if(hr == E_INVALIDARG)
		{
			hr = D3D11CreateDeviceAndSwapChain(
				NULL,
				D3D_DRIVER_TYPE_HARDWARE,
				NULL,
				0,
				&FeatureLevelSupported,
				1,
				D3D11_SDK_VERSION,
				&scd,
				&g_pSwapchain,
				&g_pDev,
				NULL,
				&g_pDevcon
			);
		}

		if(hr == S_OK)
		{
			CreateRenderTarget();
			SetViewPort();
			InitPipeline();
			InitGraphics();
		}
	}

	return hr;
}

void DiscardGraphicsResources()
{
	SafeRelease(&g_pLayout);
	SafeRelease(&g_pVS);
	SafeRelease(&g_pPS);
	SafeRelease(&g_pVBuffer);
	SafeRelease(&g_pSwapchain);
	SafeRelease(&g_pRTView);
	SafeRelease(&g_pDev);
	SafeRelease(&g_pDevcon);
}

// 渲染单个帧的函数
void RenderFrame()
{
	// 将缓存清空为深蓝色
	const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	g_pDevcon -> ClearRenderTargetView(g_pRTView, clearColor);

	// 在缓存中进行 3D 渲染
	{
		// 选择顶点缓存中需要显示的顶点
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;
		g_pDevcon -> IASetVertexBuffers(0, 1, &g_pVBuffer, &stride, &offset);

		// 选择要使用的 primtice 类型
		g_pDevcon -> IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 将顶点缓存绘制到后端缓存中
		g_pDevcon -> Draw(3, 0);
	}

	// 交换后端缓存和前端缓存
	g_pSwapchain -> Present(0, 0);
}

// 窗体程序进程原型
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// 窗体程序的入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	// 窗体处理程序句柄，被函数填充
	HWND hWnd;
	// 保存窗体程序类信息的结构体
	WNDCLASSEX wc;

	// 清除要使用的窗体类
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// 使用必需的信息填充结构体
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = _T("WindowClass1");

	// 注册窗体类
	RegisterClassEx(&wc);

	// 创建一个窗体，并使用创建结果作为句柄
	hWnd = CreateWindowEx(
		0,
		_T("WindowClass1"),						// 窗体类的名称
		_T("Hello, Game Engine![Direct 3D]"),	// 窗体的标题
		WS_OVERLAPPEDWINDOW,					// 窗体的样式
		100,									// 窗体的 X 坐标
		100,									// 窗体的 Y 坐标
		SCREEN_WIDTH,							// 窗体的宽度
		SCREEN_HEIGHT,							// 窗体的高度
		NULL,									// 父窗体
		NULL,									// 菜单
		hInstance,								// 应用程序句柄
		NULL									// 多窗体
	);

	// 将窗体显示在屏幕上
	ShowWindow(hWnd, nCmdShow);

	// 进入主循环

	// 保存窗体事件消息的结构体
	MSG msg;

	// 等待队列中的下个消息，将结果保存在 msg 中
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		// 将按键消息转换成正确的格式
		TranslateMessage(&msg);

		// 将消息发送到窗体 WindowProc 函数中
		DispatchMessage(&msg);
	}

	// 将这一部分的 WM_QUIT 消息发送到窗体
	return msg.wParam;
}

// 程序的主要消息处理方法
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool wasHandled = false;

	// 对给定的消息进行排序并找到需要执行的代码
	switch (message)
	{
		case WM_CREATE:
		{
			wasHandled = true;
		}
		break;
		case WM_PAINT:
		{
			result = CreateGraphicsResources(hWnd);
			RenderFrame();
			wasHandled = true;
		}
		break;
		case WM_SIZE:
			{
				if(g_pSwapchain != nullptr)
				{
					DiscardGraphicsResources();
				}
				wasHandled = true;
			}
		break;
		case WM_DESTROY:
			{
				DiscardGraphicsResources();
				PostQuitMessage(0);
				wasHandled = true;
			}
		break;
		case WM_DISPLAYCHANGE:
		{
			InvalidateRect(hWnd, nullptr, false);
			wasHandled = true;
		}
		break;
		default:
			break;
	}

	// 处理其他消息
	if(!wasHandled)
	{
		result = DefWindowProc(hWnd, message, wParam, lParam);
	}
	return result;
}
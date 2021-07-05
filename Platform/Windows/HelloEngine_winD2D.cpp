// 包含基本窗体程序头文件
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <d2d1.h>

ID2D1Factory				*pFactory = nullptr;
ID2D1HwndRenderTarget		*pRenderTarget = nullptr;
ID2D1SolidColorBrush		*pLightSlateGrayBrush = nullptr;
ID2D1SolidColorBrush		*pCornflowerBlueBrush = nullptr;

template<class T>
inline void SafeRelease(T **ppInterfaceToRelease)
{
	if(*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease) -> Release();

		(*ppInterfaceToRelease) = nullptr;
	}
}

HRESULT CreateGraphicsResources(HWND hWnd)
{
	HRESULT hr = S_OK;
	if(pRenderTarget == nullptr)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		hr = pFactory -> CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hWnd, size),
			&pRenderTarget
		);

		if(SUCCEEDED(hr))
		{
			hr = pRenderTarget -> CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &pLightSlateGrayBrush);
		}
		if(SUCCEEDED(hr))
		{
			hr = pRenderTarget ->  CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &pCornflowerBlueBrush);
		}
	}
	return hr;
}

void DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pLightSlateGrayBrush);
	SafeRelease(&pCornflowerBlueBrush);
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

	// 初始化 COM
	if(FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		return -1;
	}

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
	hWnd = CreateWindowEx(0,
		_T("WindowClass1"),						// 窗体的名称
		_T("Hello, GameEngine[Direct 2D]!"),	// 窗体的标题
		WS_OVERLAPPEDWINDOW,					// 窗体样式
		100,									// 窗体的x坐标
		100,									// 窗体的y坐标
		960,									// 窗体的宽度
		540,									// 窗体的高度
		NULL,									// 父窗体
		NULL,									// 菜单栏
		hInstance,								// 句柄
		NULL);									// 用于多个窗体

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

	// 卸载 COM
	CoUninitialize();

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
				if(FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
				{
					result = -1;		// 窗体创建失败
					return result;
				}
				wasHandled = true;
				result = 0;
			}
		break;
		case WM_PAINT:
		{
			HRESULT hr = CreateGraphicsResources(hWnd);
			if(SUCCEEDED(hr))
			{
				PAINTSTRUCT ps;
				BeginPaint(hWnd, &ps);

				// 开始构建 GPU 绘制命令
				pRenderTarget -> BeginDraw();

				// 用白色清空背景
				pRenderTarget -> Clear(D2D1::ColorF(D2D1::ColorF::White));

				// 重新获取绘制区域的尺寸
				D2D1_SIZE_F rtSize = pRenderTarget -> GetSize();

				// 绘制一个网格背景
				int width = static_cast<int>(rtSize.width);
				int height = static_cast<int>(rtSize.height);

				for(int x = 0; x < width; x += 10)
				{
					pRenderTarget -> DrawLine(
						D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
						D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
						pLightSlateGrayBrush,
						0.5f
					);
				}

				for(int y = 0; y < height; y += 10)
				{
					pRenderTarget -> DrawLine(
						D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
						D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
						pLightSlateGrayBrush,
						0.5f
					);
				}

				// 绘制两个矩形
				D2D1_RECT_F rectangle1 = D2D1::RectF(
					rtSize.width / 2 - 50.0f,
					rtSize.height / 2 - 50.0f,
					rtSize.width / 2 + 50.0f,
					rtSize.height / 2 + 50.0f
				);

				D2D1_RECT_F rectangle2 = D2D1::RectF(
					rtSize.width / 2 - 100.0f,
					rtSize.height / 2 - 100.0f,
					rtSize.width / 2 + 100.0f,
					rtSize.height / 2 + 100.0f
				);

				// 绘制一个填充的矩形
				pRenderTarget -> FillRectangle(&rectangle1, pLightSlateGrayBrush);

				// 绘制一个只有边框的矩形
				pRenderTarget -> DrawRectangle(&rectangle2, pCornflowerBlueBrush);

				// 结束 GPU 绘制命令构建
				hr = pRenderTarget -> EndDraw();
				if(FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
				{
					DiscardGraphicsResources();
				}
				EndPaint(hWnd, &ps);
			}
			wasHandled = true;
		}
		break;
		case WM_SIZE:
			{
				if(pRenderTarget != nullptr)
				{
					RECT rc;
					GetClientRect(hWnd, &rc);

					D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
					pRenderTarget -> Resize(size);
				}
				wasHandled = true;
			}
		break;
		case WM_DESTROY:
			{
				DiscardGraphicsResources();
				if(pFactory)
				{
					pFactory -> Release();
					pFactory = nullptr;
				}
				PostQuitMessage(0);
				result = 0;
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
// 包含基本窗体程序头文件
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

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
	ZeroMemeory(&wc, sizeof(WNDCLASSEX));

	// 使用必需的信息填充结构体
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HEADRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = _T("WindowClass1");

	// 注册窗体类
	RegisterClassEx(&wc);

	// 创建一个窗体，并使用创建结果作为句柄
	hWnd = CreateWindowEx(0,
		_T("WindowClass1"),			// 窗体的名称
		_T("Hello, GameEngine!"),	// 窗体的标题
		WS_OVERLAPPENDWINDOW,		// 窗体样式
		300,						// 窗体的x坐标
		300,						// 窗体的y坐标
		500,						// 窗体的宽度
		400,						// 窗体的高度
		NULL,						// 父窗体
		NULL,						// 菜单栏
		hInstance,					// 句柄
		NULL);						// 用于多个窗体

	// 将窗体显示在屏幕上
	ShowWindow(hWnd, nCmdShow);

	// 进入主循环

	// 保存窗体事件消息的结构体
	MSG msg;

	// 等待队列中的下个消息，将结果保存在 msg 中
	while (GetMessage(&msg, NULL, 0, 0))
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
	// 对给定的消息进行排序并找到需要执行的代码
	switch (message)
	{
		// 当窗体被关闭时
		case WM_DESTORY:
		{
			// 关闭程序
			PostQuitMessage(0);
			return 0;	
		}
		break;
		default:
			break;
	}

	// 处理其他消息
	return DefWindowProc(hWnd, message, wParam, lParam);
}
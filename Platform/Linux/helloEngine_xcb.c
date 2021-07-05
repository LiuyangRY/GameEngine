#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

int main(void)
{
	xcb_connection_t		*pConn;
	xcb_screen_t			*pScreen;
	xcb_window_t			window;
	xcb_gcontext_t			foreground;
	xcb_gcontext_t			background;
	xcb_generic_event_t		*pEvent;
	u_int32_t				mask = 0;
	u_int32_t				values[2];
	u_int8_t				isQuit = 0;

	char title[] = "Hello, GameEngine!";
	char title_icon[] = "Hello, GameEngine!(iconified)";

	// 与 X Server 建立连接
	pConn = xcb_connect(0, 0);

	// 获取第一个屏幕 
	pScreen = xcb_setup_roots_iterator(xcb_get_setup(pConn)).data;

	// 获取根窗体
	window = pScreen -> root;

	// 创建黑色的前景
	foreground = xcb_generate_id(pConn);
	mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0] = pScreen -> black_pixel;
	values[1] = 0;
	xcb_create_gc(pConn, foreground, window, mask, values);

	// 创建白色背景
	background = xcb_generate_id(pConn);
	mask = XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0] = pScreen -> white_pixel;
	values[1] = 0;
	xcb_create_gc(pConn, background, window, mask, values);

	// 创建窗体
	window = xcb_generate_id(pConn);
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = pScreen -> white_pixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
	xcb_create_window(pConn, 			// 连接
		XCB_COPY_FROM_PARENT,			// 级别
		window,							// 窗体编号
		pScreen -> root,				// 父窗体
		20, 20,							// x, y坐标
		640, 480,						// 宽度，高度
		10,								// 边框宽度
		XCB_WINDOW_CLASS_INPUT_OUTPUT,	// 类型
		pScreen -> root_visual,			// 可视
		mask, values);

	// 设置窗体标题
	xcb_change_property(pConn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		strlen(title), title);

	// 设置窗体图标的标题
	xcb_change_property(pConn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8,
		strlen(title_icon), title_icon);

	// 在屏幕上映射窗体
	xcb_map_window(pConn, window);

	xcb_flush(pConn);

	while((pEvent = xcb_wait_for_event(pConn)) && !isQuit)
	{
		switch (pEvent -> response_type & ~0x80)
		{
		case XCB_EXPOSE:
			break;
		case XCB_KEY_PRESS:
			isQuit = 1;
			break;
		default:
			break;
		}
		free(pEvent);
	}
	return 0;
}
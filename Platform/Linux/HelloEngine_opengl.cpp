#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB		0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// 检查字符串是否存在的帮助类，改编自：http://www.opengl.org/resources/features/OGLextensions/
static bool IsExtensionSupported(const char *extList, const char *extension)
{
	const char *start;
	const char *where, *terminator;

	// 扩展名不应该存在空格
	where = strchr(extension, ' ');
	if(where || *extension == '\0')
	{
		return false;
	}

	// 类的解析需要注意一点 OpenGL 扩展字符串，不要被子字符串欺骗了
	for(start = extList; ;)
	{
		where = strstr(start, extension);
		if(!where)
		{
			break;
		}

		terminator = where + strlen(extension);

		if(where == start || *(where - 1) == ' ')
		{
			if(*terminator == ' ' || *terminator == '\0')
			{
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev)
{
	ctxErrorOccurred = true;
	return 0;
}

void DrawAQuad()
{
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1., 1., -1., 1., 1., 20.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

	glBegin(GL_QUADS);
	glColor3f(1., 0., 0.);
	glVertex3f(-.75, -.75, 0.);
	glColor3f(0., 1., 0.);
	glVertex3f(.75, -.75, 0.);
	glColor3f(0., 0., 1.);
	glVertex3f(.75, .75, 0.);
	glColor3f(1., 1., 0.);
	glVertex3f(-.75, .75, 0.);
	glEnd();
	glFlush();
}

int main(void)
{
	xcb_connection_t		*pConn;
	xcb_screen_t			*pScreen;
	xcb_window_t			window;
	xcb_gcontext_t			foreground;
	xcb_gcontext_t			background;
	xcb_generic_event_t		*pEvent;
	xcb_colormap_t colormap;
	uint32_t				mask = 0;
	uint32_t				values[3];
	uint8_t					isQuit = 0;

	char title[] = "Hello, GameEngine![OpenGL]";
	char title_icon[] = "Hello, GameEngine!(iconified)";

	Display *display;
	int default_screen;
	GLXContext context;
	GLXFBConfig *fb_configs;
	GLXFBConfig fb_config;
	int num_fb_configs = 0;
	XVisualInfo *vi;
	GLXDrawable drawable;
	GLXWindow glxwindow;
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
	const char *glxExts;

	// 获取一个匹配的 FB 配置
	static int visual_attribs[] =
	{
		GLX_X_RENDERABLE		, True,
		GLX_DRAWABLE_TYPE		, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE			, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE		, GLX_TRUE_COLOR,
		GLX_RED_SIZE			, 8,
		GLX_GREEN_SIZE			, 8,
		GLX_BLUE_SIZE			, 8,
		GLX_ALPHA_SIZE			, 8,
		GLX_DEPTH_SIZE			, 24,
		GLX_STENCIL_SIZE		, 8,
		GLX_DOUBLEBUFFER		, True,
		// GLX_SAMPLE_BUFFERS	, 1,
		// GLX_SAMPLES			, 4,
		None
	};

	int glx_major, glx_minor;

	// Open Xlib 显示
	display = XOpenDisplay(NULL);
	if(!display)
	{
		fprintf(stderr, "无法显示窗体\n");
		return -1;
	}

	// FB 配置被添加到 GLX 1.3 版本中
	if(!glXQueryVersion(display, &glx_major, &glx_minor) || ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1))
	{
		fprintf(stderr, "无效的 GLX 版本\n");
		return -1;
	}

	default_screen = DefaultScreen(display);

	// 查询帧缓存配置
	fb_configs = glXChooseFBConfig(display, default_screen, visual_attribs, &num_fb_configs);
	if(!fb_configs || num_fb_configs == 0)
	{
		fprintf(stderr, "glxGetFBConfigs 失败\n");
		return -1;
	}

	// 选择每像素点样本最多的配置/可视化
	{
		int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

		for(int i = 0; i < num_fb_configs; ++i)
		{
			XVisualInfo *vi = glXGetVisualFromFBConfig(display, fb_configs[i]);
			if(vi)
			{
				int samp_buf, samples;
				glXGetFBConfigAttrib(display, fb_configs[i], GLX_SAMPLE_BUFFERS, &samp_buf);
				glXGetFBConfigAttrib(display, fb_configs[i], GLX_SAMPLES, &samples);

				printf("匹配 fb配置 %d, 可视化编号 0x%lx: SAMPLE_BUFFERS = %d,样本 = %d\n", i, vi -> visualid, samp_buf, samples);
				if(best_fbc < 0 || (samp_buf && samples > best_num_samp))
				{
					best_fbc = i;
					best_num_samp = samples;
				}
				if(worst_fbc < 0 || !samp_buf || samples < worst_num_samp)
				{
					worst_fbc = i;
					worst_num_samp = samples;
				}
			}

			
			XFree(vi);
		}
		fb_config = fb_configs[best_fbc];
	}

	// 获取可视化界面
	vi = glXGetVisualFromFBConfig(display, fb_config);
	printf("Chosen visual ID = 0x%lx\n", vi -> visualid);

	// 与 X Server 建立连接
	pConn = XGetXCBConnection(display);
	if(!pConn)
	{
		XCloseDisplay(display);
		fprintf(stderr, "无法获取 xcb 连接\n");
		return -1;
	}

	// 获取事件队列
	XSetEventQueueOwner(display, XCBOwnsEventQueue);

	// 查找 XCB 屏幕
	xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(pConn));
	for(int screen_num = vi -> screen; screen_iter.rem && screen_num > 0; --screen_num, xcb_screen_next(&screen_iter));
	pScreen = screen_iter.data;

	// 获取根窗体
	window = pScreen -> root;

	// 创建 XID 的颜色图谱
	colormap = xcb_generate_id(pConn);

	xcb_create_colormap(
		pConn,
		XCB_COLORMAP_ALLOC_NONE,
		colormap,
		window,
		vi -> visualid
	);

	// 创建窗体
	window = xcb_generate_id(pConn);
	mask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	values[0] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
	values[1] = colormap;
	values[2] = 0;
	xcb_create_window(pConn, 			// 连接
		XCB_COPY_FROM_PARENT,			// 级别
		window,							// 窗体编号
		pScreen -> root,				// 父窗体
		20, 20,							// x, y坐标
		640, 480,						// 宽度，高度
		10,								// 边框宽度
		XCB_WINDOW_CLASS_INPUT_OUTPUT,	// 类型
		vi -> visualid,					// 可视
		mask, 
		values
	);

	XFree(vi);

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

	// 获取默认屏幕的 GLX 扩展列表
	glxExts = glXQueryExtensionsString(display, default_screen);

	// 注意：在调用 glXGetProcAddressARB 方法前，创建上下文或者让当前对象成为上下文不是必须的
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

	// 创建 OpenGL 上下文
	ctxErrorOccurred = false;
	int(*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

	if(!IsExtensionSupported(glxExts, "GLX_ARB_create_context") || !glXCreateContextAttribsARB)
	{
		printf("glXCreateContextAttribsARB() 未发现，将使用以前的 GLX 上下文\n");
		context = glXCreateNewContext(display, fb_config, GLX_RGBA_TYPE, 0, True);
		if(!context)
		{
			fprintf(stderr, "glXCreateNewContext 执行失败\n");
			return -1;
		}
	}
	else
	{
		int context_attribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
			GLX_CONTEXT_MINOR_VERSION_ARB, 0,
			None
		};

		printf("创建上下文\n");
		context = glXCreateContextAttribsARB(display, fb_config, 0, True, context_attribs);

		XSync(display, False);
		if(!ctxErrorOccurred && context)
		{
			printf("创建 GL 3.0 上下文\n");
		}
		else
		{
			context_attribs[1] = 1;
			context_attribs[3] = 0;
			ctxErrorOccurred = false;
			printf("创建 GL 3.0 上下文失败，将使用以前版本的 GLX 上下文\n");
			context = glXCreateContextAttribsARB(display, fb_config, 0, True, context_attribs);
		}
	}

	XSync(display, False);

	XSetErrorHandler(oldHandler);

	if(ctxErrorOccurred || !context)
	{
		printf("创建 OpenGL 上下文失败\n");
		return -1;
	}

	// 指定上下文是一个直接上下文
	if(!glXIsDirect(display, context))
	{
		printf("包含间接 GLX 渲染上下文\n");
	}
	else
	{
		printf("包含直接 GLX 上下文\n");
	}

	// 创建 GLX 窗体
	glxwindow = glXCreateWindow(
		display,
		fb_config,
		window,
		0
	);

	if(!window)
	{
		xcb_destroy_window(pConn, window);
		glXDestroyContext(display, context);
		fprintf(stderr, "glXDestoryContext 执行失败\n");
		return -1;
	}

	drawable = glxwindow;

	// 让 OpenGL 上下文到当前
	if(!glXMakeContextCurrent(display, drawable, drawable, context))
	{
		xcb_destroy_window(pConn, window);
		glXDestroyContext(display, context);
		fprintf(stderr, "glXMakeContextCurrent 执行失败\n");
		return -1;
	}

	while(!isQuit && (pEvent = xcb_wait_for_event(pConn)))
	{
		switch (pEvent -> response_type & ~0x80)
		{
		case XCB_EXPOSE:
			{
				DrawAQuad();
				glXSwapBuffers(display, drawable);
			}
			break;
		case XCB_KEY_PRESS:
			isQuit = 1;
			break;
		default:
			break;
		}
		free(pEvent);
	}
	// 清理
	xcb_disconnect(pConn);
	return 0;
}
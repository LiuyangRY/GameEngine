#pragma once
#include "IApplication.hpp"

namespace GameEngine
{
	class BaseApplication : implements IApplication
	{
	public:
		virtual int Initialize();
		virtual void Finalize();
		virtual void Tick();

		virtual bool IsQuit();

	protected:
		// 程序从主循环退出的标志
		bool m_bQuit;
	};
}
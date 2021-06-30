#include "BaseApplication.hpp"

// 转化命令行，读取配置，初始化所有子模块
int GameEngine::BaseApplication::Initialize()
{
	m_bQuit = false;

	return 0;
}

// 销毁所有子模块，并清理所有的运行时临时文件
void GameEngine::BaseApplication::Finalize()
{
}

// 每次主循环中执行一次
void GameEngine::BaseApplication::Tick()
{
}

// 是否退出主循环
bool GameEngine::BaseApplication::IsQuit()
{
	return m_bQuit;
}
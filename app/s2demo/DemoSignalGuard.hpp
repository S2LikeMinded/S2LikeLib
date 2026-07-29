#pragma once

#include <atomic>
#include <csignal>

namespace S2Demo
{
	class DemoSignalGuard
	{
	public:
		static inline std::atomic<bool> interrupted{ false };

		DemoSignalGuard()
		{
			interrupted = false;
			m_prevSignalHandler = std::signal(SIGINT, &DemoSignalGuard::onSignal);
		}

		~DemoSignalGuard()
		{
			std::signal(SIGINT, m_prevSignalHandler);
			interrupted = false;
		}

		static bool isInterrupted() noexcept
		{
			return interrupted.load();
		}

	private:
		using SignalHandler = void(*)(int);
		SignalHandler m_prevSignalHandler{ nullptr };

		static void onSignal(int sig)
		{
			if (sig == SIGINT)
			{
				interrupted = true;
			}
		}
	};
}

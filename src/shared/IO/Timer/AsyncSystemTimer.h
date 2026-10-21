#ifndef MANGOS_IO_TIMER_ASYNCSYSTEMTIMER_H
#define MANGOS_IO_TIMER_ASYNCSYSTEMTIMER_H

#include "Common.h"
#include "Policies/Singleton.h"
#include "TimerHandle.h"
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

namespace IO { namespace Timer {
    // Queue ownership is shared with the worker; handles only keep a weak reference.
    struct TimerQueue
    {
        std::mutex mutex;
        std::condition_variable changed;
        PendingTimers pending;
        bool running = true;
    };

    class AsyncSystemTimer : public MaNGOS::Singleton<AsyncSystemTimer, MaNGOS::ClassLevelLockable<AsyncSystemTimer, std::mutex>>
    {
    public:
        AsyncSystemTimer();
        ~AsyncSystemTimer();
        AsyncSystemTimer(AsyncSystemTimer const&) = delete;
        AsyncSystemTimer& operator=(AsyncSystemTimer const&) = delete;
        void RemoveAllTimersAndStopThread();

        // Callbacks run serially on the timer thread. Do not use for map/game logic.
        template <class Rep, class Period>
        std::shared_ptr<TimerHandle> ScheduleFunctionOnce(std::chrono::duration<Rep, Period> delay, std::function<void()> const& function)
        {
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
            return Schedule(milliseconds > 0 ? uint64_t(milliseconds) : 0, function);
        }

    private:
        std::shared_ptr<TimerHandle> Schedule(uint64_t milliseconds, std::function<void()> const& function);
        static void Run(std::shared_ptr<TimerQueue> const& queue);
        std::shared_ptr<TimerQueue> m_queue;
        std::thread m_thread;
    };
}}
#define sAsyncSystemTimer MaNGOS::Singleton<IO::Timer::AsyncSystemTimer>::Instance()
#endif

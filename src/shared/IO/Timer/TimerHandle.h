#ifndef MANGOS_IO_TIMER_TIMERHANDLE_H
#define MANGOS_IO_TIMER_TIMERHANDLE_H
#include <functional>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace IO { namespace Timer {
    class TimerHandle;
    using PendingTimers = std::multimap<std::chrono::steady_clock::time_point, std::shared_ptr<TimerHandle>>;
    struct TimerQueue;
    class AsyncSystemTimer;
    class TimerHandle
    {
        friend class AsyncSystemTimer;
    public:
        // Removes a pending callback and waits for an executing callback to finish.
        // A callback may cancel itself; cancellation is also safe after timer shutdown.
        void Cancel();
    private:
        TimerHandle(std::shared_ptr<TimerQueue> const& queue, std::function<void()> callback)
            : m_queue(queue), m_callback(std::move(callback)) {}
        // Iterator and pending flag are protected by TimerQueue::mutex.
        PendingTimers::iterator m_pending;
        bool m_isPending = false;
        std::weak_ptr<TimerQueue> m_queue;
        std::recursive_mutex m_callbackMutex;
        std::function<void()> m_callback;
    };
}}
#endif

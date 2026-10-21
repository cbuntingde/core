#include "AsyncSystemTimer.h"
#include "IO/Multithreading/CreateThread.h"
#include "Policies/SingletonImp.h"
#include "Log.h"

INSTANTIATE_SINGLETON_1(IO::Timer::AsyncSystemTimer);

IO::Timer::AsyncSystemTimer::AsyncSystemTimer() : m_queue(std::make_shared<TimerQueue>()),
    m_thread(IO::Multithreading::CreateThread("SystemTimer", [queue = m_queue]() { Run(queue); })) {}

IO::Timer::AsyncSystemTimer::~AsyncSystemTimer()
{
    RemoveAllTimersAndStopThread();
}

void IO::Timer::AsyncSystemTimer::RemoveAllTimersAndStopThread()
{
    decltype(m_queue->pending) canceled;
    {
        std::lock_guard<std::mutex> lock(m_queue->mutex);
        m_queue->running = false;
        for (auto const& entry : m_queue->pending)
            entry.second->m_isPending = false;
        canceled.swap(m_queue->pending);
    }
    for (auto const& entry : canceled)
        entry.second->Cancel();
    canceled.clear();
    m_queue->changed.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

std::shared_ptr<IO::Timer::TimerHandle> IO::Timer::AsyncSystemTimer::Schedule(uint64_t milliseconds, std::function<void()> const& function)
{
    auto handle = std::shared_ptr<TimerHandle>(new TimerHandle(m_queue, function));
    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::time_point::max() - now).count();
    auto deadline = milliseconds >= uint64_t(remaining) ? std::chrono::steady_clock::time_point::max()
        : now + std::chrono::milliseconds(milliseconds);
    {
        std::lock_guard<std::mutex> lock(m_queue->mutex);
        if (!m_queue->running)
            return nullptr;
        handle->m_pending = m_queue->pending.emplace(deadline, handle);
        handle->m_isPending = true;
    }
    m_queue->changed.notify_all();
    return handle;
}

void IO::Timer::AsyncSystemTimer::Run(std::shared_ptr<TimerQueue> const& queue)
{
    std::unique_lock<std::mutex> lock(queue->mutex);
    while (queue->running)
    {
        if (queue->pending.empty())
        {
            queue->changed.wait(lock);
            continue;
        }
        auto first = queue->pending.begin();
        if (first->first > std::chrono::steady_clock::now())
        {
            auto deadline = first->first;
            queue->changed.wait_until(lock, deadline);
            continue;
        }
        auto handle = std::move(first->second);
        handle->m_isPending = false;
        queue->pending.erase(first);
        lock.unlock();
        {
            std::lock_guard<std::recursive_mutex> callbackLock(handle->m_callbackMutex);
            auto callback = std::move(handle->m_callback);
            if (callback)
            {
                try { callback(); }
                catch (std::exception const& error)
                {
                    sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Timer callback failed: %s", error.what());
                }
                catch (...)
                {
                    sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Timer callback failed with an unknown exception");
                }
            }
        }
        lock.lock();
    }
}

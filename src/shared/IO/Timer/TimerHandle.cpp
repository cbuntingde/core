#include "AsyncSystemTimer.h"

void IO::Timer::TimerHandle::Cancel()
{
    if (auto queue = m_queue.lock())
    {
        std::lock_guard<std::mutex> lock(queue->mutex);
        if (m_isPending)
        {
            queue->pending.erase(m_pending);
            m_isPending = false;
            queue->changed.notify_all();
        }
    }
    // The worker never holds the queue lock while acquiring this lock.
    std::lock_guard<std::recursive_mutex> lock(m_callbackMutex);
    m_callback = nullptr;
}

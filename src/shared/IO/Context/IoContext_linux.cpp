#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>
#include "Log.h"
#include "IoContext.h"
#include "IO/SystemErrorToString.h"

IO::IoContext::IoContext(IO::Native::FileHandle epollDescriptor, IO::Native::FileHandle contextSwitchEventFd)
        : m_epollDescriptor(epollDescriptor), m_contextSwitchNotifyEventFd(contextSwitchEventFd), m_isRunning{true}
{
}

IO::IoContext::~IoContext()
{
    ::close(m_contextSwitchNotifyEventFd);
    ::close(m_epollDescriptor);
}

// The batch of events that RunUntilShutdown() is currently dispatching on this thread.
// Only the events after `next` are still pending.
struct DispatchBatch
{
    struct epoll_event* events;
    int next;
    int count;
};
static thread_local DispatchBatch* g_currentDispatchBatch = nullptr;

std::unique_ptr<IO::IoContext> IO::IoContext::CreateIoContext()
{
    // Initialize our main epoll queue
    int const epollSizeHint = 50; // <-- hint, how much initial epoll space we want to have. But in modern kernels this is ignored anyway.
    int epollDescriptor = ::epoll_create(epollSizeHint);
    if (epollDescriptor == -1)
    {
        sLog.Out(LOG_NETWORK, LOG_LVL_ERROR, "CreateIoContext() -> ::epoll_create(...) Error: %s", SystemErrorToString(errno).c_str());
        return nullptr;
    }

    // Add eventfd, where we can listen to incoming context switch events
    uint32_t constexpr initialCounter = 0;
    IO::Native::FileHandle contextSwitchEventFd = ::eventfd(initialCounter, 0);
    if (contextSwitchEventFd == -1)
    {
        sLog.Out(LOG_NETWORK, LOG_LVL_ERROR, "CreateIoContext() -> ::eventfd(...) : %s", SystemErrorToString(errno).c_str());
        return nullptr;
    }

    // Add our contextSwitchEventFd to the epoll set
    struct epoll_event event{};
    event.events = EPOLLIN | EPOLLET; // We are using edge here, since we are just using it as a "once" signalling process system
    event.data.u32 = static_cast<uint32_t>(IoContextEpollTargetType::ContextSwitchRequest);
    if (::epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, contextSwitchEventFd, &event) == -1)
    {
        sLog.Out(LOG_NETWORK, LOG_LVL_ERROR, "CreateIoContext() -> ::epoll_ctl(...) Error: %s", SystemErrorToString(errno).c_str());
        return nullptr;
    }

    return std::unique_ptr<IO::IoContext>(new IO::IoContext(epollDescriptor, contextSwitchEventFd));
}

void IO::IoContext::RunUntilShutdown()
{
    int const maxEventsPerLoop = 250;

    struct epoll_event events[maxEventsPerLoop];

    while (m_isRunning)
    {
        int numEvents = ::epoll_wait(m_epollDescriptor, events, maxEventsPerLoop, 500);
        if (numEvents == -1)
        {
            if (errno != EINTR) // ignore interrupted system call
                sLog.Out(LOG_NETWORK, LOG_LVL_ERROR, "RunEventLoop -> ::epoll_wait(...) Error: %s", SystemErrorToString(errno).c_str());
            continue;
        }

        DispatchBatch batch{events, 0, numEvents};
        g_currentDispatchBatch = &batch;
        while (batch.next < batch.count)
        {
            struct epoll_event const& event = events[batch.next++];
            if (event.data.ptr == nullptr)
                continue; // The receiver was destroyed by an earlier event of this batch

            if (event.data.u32 == static_cast<uint32_t>(IoContextEpollTargetType::ContextSwitchRequest))
            {
                while (!m_contextSwitchQueue.empty())
                {
                    IO::SystemIoEventReceiver* eventReceiver;
                    {
                        std::lock_guard<std::mutex> lock(m_contextSwitchQueueLock);
                        if (m_contextSwitchQueue.empty()) // re-check after we locked the queue if it's really not empty
                            continue;
                        eventReceiver = m_contextSwitchQueue.front();
                        m_contextSwitchQueue.pop();
                    }
                    eventReceiver->OnIoEvent(0);
                }
            }
            else
            {
                ((SystemIoEventReceiver*)(event.data.ptr))->OnIoEvent(event.events);
            }
        }
        g_currentDispatchBatch = nullptr;
    }
}

void IO::IoContext::ForgetReceiverInCurrentBatch(IO::SystemIoEventReceiver const* eventReceiver)
{
    DispatchBatch* batch = g_currentDispatchBatch;
    if (batch == nullptr)
        return;

    for (int i = batch->next; i < batch->count; i++)
    {
        if (batch->events[i].data.ptr == eventReceiver)
            batch->events[i].data.ptr = nullptr;
    }
}

bool IO::IoContext::IsRunning() const
{
    return m_isRunning;
}

void IO::IoContext::Shutdown()
{
    m_isRunning = false;
}

void IO::IoContext::PostForImmediateInvocation(IO::SystemIoEventReceiver* eventReceiver)
{
    m_contextSwitchQueueLock.lock();
    m_contextSwitchQueue.push(eventReceiver);
    m_contextSwitchQueueLock.unlock();
    ::eventfd_write(m_contextSwitchNotifyEventFd, 1);
}

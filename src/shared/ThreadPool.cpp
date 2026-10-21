/*
 * Copyright (C) 2017 Elysium Project <https://github.com/elysium-project>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ThreadPool.h"
#include "Log.h"
#include "IO/Multithreading/CreateThread.h"
#include <mysql.h>
#include <stdexcept>

#ifdef WIN32
#undef ERROR
#undef IGNORE
#endif

ThreadPool::ThreadPool(std::string const& name, int numThreads, ClearMode when, ErrorHandling mode) :
     m_poolName(name), m_errorHandling(mode), m_size(numThreads), m_clearMode(when), m_active(0)
{
    if (numThreads < 0)
        throw std::invalid_argument("ThreadPool size cannot be negative");
    m_workers.reserve(m_size);
}

ThreadPool::~ThreadPool()
{
    std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
    m_status = Status::TERMINATING;
    m_waitForWork.notify_all();
    lock.unlock();
    // Join before worker destruction changes the virtual dispatch table.
    for (auto& worker : m_workers)
        if (worker->thread.joinable())
            worker->thread.join();
}

std::future<void> ThreadPool::processWorkload(Callable pre, Callable post)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
    return startWorkload(pre, post);
}

std::future<void> ThreadPool::startWorkload(Callable pre, Callable post)
{
    if (m_clearMode == ClearMode::AT_NEXT_WORKLOAD &&
            m_status == Status::READY && m_dirty)
        clearWorkload();
    if (m_status != Status::READY || m_workload.empty())
        return std::future<void>();
    m_errors.clear();
    m_result = std::promise<void>();
    m_dirty = true;
    m_active = std::min(m_size, m_workload.size());
    m_index = 0;
    m_status = Status::PROCESSING;
    for (int i = 0; i < m_active; i++)
        m_workers[i]->prepare(pre, post);
    m_waitForWork.notify_all();
    return m_result.get_future();
}

std::future<void> ThreadPool::processWorkload(workload_t &workload, Callable pre, Callable post)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
    if (m_status != Status::READY)
        return std::future<void>();
    m_workload = workload;
    m_dirty = false;
    return startWorkload(pre, post);
}

std::future<void> ThreadPool::processWorkload(workload_t &&workload, Callable pre, Callable post)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
    if (m_status != Status::READY)
        return std::future<void>();
    m_workload = std::move(workload);
    m_dirty = false;
    return startWorkload(pre, post);
}

ThreadPool::Status ThreadPool::status() const
{
    return m_status;
}

size_t ThreadPool::size() const
{
    return m_size;
}

std::vector<std::exception_ptr> ThreadPool::taskErrors() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);
    return m_errors;
}

void ThreadPool::worker::waitForWork()
{
    std::shared_lock<std::shared_timed_mutex> lock(pool->m_mutex); //locked!
    while(!busy && pool->status() != Status::TERMINATING) //wait for work
        pool->m_waitForWork.wait(lock);
}

ThreadPool &ThreadPool::operator<<(Callable packaged_task)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
    if (m_status == Status::PROCESSING || m_status == Status::ERROR)
        throw "Attempt to append a task to a load being processed!";
    if (m_clearMode == ClearMode::AT_NEXT_WORKLOAD && m_dirty)
        clearWorkload();
    m_workload.emplace_back(packaged_task);
    return *this;
}

void ThreadPool::clearWorkload()
{
    m_dirty = false;
    m_workload.clear();
}

ThreadPool::worker::worker(ThreadPool *pool, int id, ThreadPool::ErrorHandling mode) :
    id(id), errorHandling(mode), pool(pool)
{
}

ThreadPool::worker::~worker()
{
    if (thread.joinable())
        thread.join();
}

void ThreadPool::worker::loop_wrapper()
{
    loop();
}

bool ThreadPool::executeTask(Callable const& task)
{
    if (!task)
        return true;
    try { task(); }
    catch (...)
    {
        if (m_errorHandling == ErrorHandling::NONE)
            throw;
        if (m_errorHandling == ErrorHandling::IGNORE)
            return false;
        {
            std::unique_lock<std::shared_timed_mutex> lock(m_mutex);
            m_errors.push_back(std::current_exception());
            if (m_errorHandling == ErrorHandling::TERMINATE && m_status != Status::TERMINATING)
                m_status = Status::ERROR; // Cancel this batch; all active workers still complete.
        }
        try { throw; }
        catch (std::exception const& error)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "A ThreadPool task generated an exception: %s", error.what());
        }
        catch (...)
        {
            sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "A ThreadPool task generated an unknown exception");
        }
        return false;
    }
    return true;
}

void ThreadPool::worker::prepare(ThreadPool::Callable pre, ThreadPool::Callable post)
{
    busy = true;
    this->pre = pre;
    this->post = post;
}

void ThreadPool::worker::loop()
{
    while(true)
    {
        waitForWork();
        if (pool->m_status == Status::TERMINATING)
            return;

        if (pool->executeTask(pre))
        {
            doWork();
            pool->executeTask(post);
        }

        busy = false;
        int remaning = --(pool->m_active);
        if (!remaning)
        {
            std::unique_lock<std::shared_timed_mutex> lock(pool->m_mutex);
            if (pool->m_clearMode == ClearMode::UPPON_COMPLETION)
                pool->clearWorkload();
            if (!pool->m_errors.empty())
            {
                if (pool->m_status != Status::TERMINATING)
                    pool->m_status = Status::READY;
                pool->m_result.set_exception(pool->m_errors.front());
            }
            else
            {
                if (pool->m_status != Status::TERMINATING)
                    pool->m_status = Status::READY;
                pool->m_result.set_value();
            }
        }
    }
}

ThreadPool::worker_mq::worker_mq(ThreadPool *pool, int id, ThreadPool::ErrorHandling mode) :
    worker(pool,id,mode)
{
}

void ThreadPool::worker_mq::doWork()
{
    while (it < pool->m_workload.size() && pool->m_status == Status::PROCESSING)
    {
        int w = it;
        it += pool->m_size; //if it fails, we might want to skip this task.
        pool->executeTask(pool->m_workload[w]);
    }
}

void ThreadPool::worker_mq::prepare(ThreadPool::Callable pre, ThreadPool::Callable post)
{
    it = id;
    worker::prepare(pre, post);
}

ThreadPool::worker_sq::worker_sq(ThreadPool *pool, int id, ThreadPool::ErrorHandling mode) :
    worker(pool,id,mode)
{
}

void ThreadPool::worker_sq::doWork()
{
    int i = pool->m_index++;
    while (i < pool->m_workload.size() && pool->m_status == Status::PROCESSING)
    {
        pool->executeTask(pool->m_workload[i]);
        i = pool->m_index++;
    }
}

template <class T>
ThreadPool::worker_mysql<T>::worker_mysql(ThreadPool *tp, int id, ThreadPool::ErrorHandling e):
    T(tp, id, e)
{
}

template <class T>
void ThreadPool::worker_mysql<T>::doWork()
{
    mysql_thread_init();
    T::doWork();
    mysql_thread_end();
}


template struct ThreadPool::worker_mysql<ThreadPool::SingleQueue>;
template struct ThreadPool::worker_mysql<ThreadPool::MultiQueue>;

template void ThreadPool::start<ThreadPool::SingleQueue>();
template void ThreadPool::start<ThreadPool::MultiQueue>();
template void ThreadPool::start<ThreadPool::MySQL<ThreadPool::SingleQueue>>();
template void ThreadPool::start<ThreadPool::MySQL<ThreadPool::MultiQueue>>();

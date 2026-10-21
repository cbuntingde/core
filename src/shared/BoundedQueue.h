#ifndef MANGOS_BOUNDED_QUEUE_H
#define MANGOS_BOUNDED_QUEUE_H

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

// Admission and accounting are one operation under the same mutex. The weight
// represents encoded payload bytes; the item limit also bounds object overhead.
template <typename T, size_t MaxItems, size_t MaxWeight>
class BoundedQueue
{
public:
    bool tryAdd(T item, size_t weight)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_items.size() >= MaxItems || weight > MaxWeight - m_weight)
            return false;
        m_items.emplace_back(std::move(item), weight);
        m_weight += weight;
        return true;
    }

    template <typename Checker>
    bool next(T& item, Checker& checker)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_items.empty() || !checker.Process(m_items.front().first))
            return false;
        item = std::move(m_items.front().first);
        m_weight -= m_items.front().second;
        m_items.pop_front();
        return true;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_items.clear();
        m_weight = 0;
    }
private:
    std::mutex m_mutex;
    std::deque<std::pair<T, size_t>> m_items;
    size_t m_weight = 0;
};
#endif

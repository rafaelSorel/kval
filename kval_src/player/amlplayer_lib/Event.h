#pragma once

#include "Mutex.h"
#include "Exception.h"
#include "EventListener.h"
#include <vector>

template<typename T>  // where T : EventArgs

/**
 * @brief The Event class
 */
class Event
{
    Mutex mutex;
    std::vector<EventListenerWPTR<T>> listeners;

public:
    ~Event() {}

    void Invoke(void *sender, const T& args)
    {
        if (sender == nullptr)
            throw ArgumentNullException();

        mutex.Lock();

        for (auto item : listeners)
        {
            auto sptr = item.lock();
            if (sptr)
            {
                sptr->Invoke(sender, args);
            }
        }
        //TODO: Clean up dead objects
        mutex.Unlock();
    }

    void AddListener(EventListenerWPTR<T> listener)
    {
        auto listenerSPTR = listener.lock();
        if (!listenerSPTR)
            throw ArgumentNullException();

        mutex.Lock();
        bool isDuplicate = false;
        for (auto item : listeners)
        {
            auto sptr = item.lock();
            if (sptr == listenerSPTR)
            {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate)
        {
            listeners.push_back(listener);
        }
        mutex.Unlock();
    }

    void RemoveListener(EventListenerSPTR<T> listener)
    {
        if (listener == nullptr)
            throw ArgumentNullException();

        mutex.Lock();
        for (auto item : listeners)
        {
            auto sptr = item.lock();
            if (sptr == listener)
            {
                listeners.erase(item);
                break;
            }
        }
        mutex.Unlock();
    }
};

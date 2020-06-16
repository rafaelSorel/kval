#pragma once


template<typename T>
using EventFunction = std::function<void(void* sender, const T& args)>;

template<typename T>
class Event;


template<typename T>

/**
 * @brief The EventListener class
 */
class EventListener
{
    friend class Event<T>;
    EventFunction<T> target;

public:
    EventListener()
    {
    }

    EventListener(EventFunction<T> target)
        : target(target)
    {
    }

    ~EventListener()
    {
        //// Remove event
        //if (source)
        //{
        // source->RemoveListener(this);
        //}
    }


    void Invoke(void *sender, const T& args)
    {
        if (target)
        {
            target(sender, args);
        }
    }
};

template<typename T>
using EventListenerSPTR = std::shared_ptr<EventListener<T>>;

template<typename T>
using EventListenerWPTR = std::weak_ptr<EventListener<T>>;

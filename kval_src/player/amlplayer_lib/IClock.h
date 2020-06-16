#pragma once

#include <memory>
#include <vector>
#include "Exception.h"


/**
 * @brief The IClockSink class
 */
class IClockSink
{
public:
    virtual ~IClockSink() {}
    virtual void SetTimeStamp(double value) = 0;

protected:
    IClockSink() {}
};
typedef std::shared_ptr<IClockSink> IClockSinkSPTR;


/**
 * @brief The ClockList class
 */
class ClockList
{
    std::vector<IClockSinkSPTR> sinks;

public:
    auto begin() -> decltype(sinks.begin())
    {
        return sinks.begin();
    }
    auto end() -> decltype(sinks.end())
    {
        return sinks.end();
    }

    void Add(IClockSinkSPTR item)
    {
        if (!item)
            throw ArgumentNullException();

        sinks.push_back(item);
    }

    void Remove(IClockSinkSPTR item)
    {
        if (!item)
            throw ArgumentNullException();

        bool found = false;
        for (auto iter = sinks.begin(); iter != sinks.end(); ++iter)
        {
            if (*iter == item)
            {
                sinks.erase(iter);
                found = true;
                break;
            }
        }
        if (!found)
            throw InvalidOperationException("The item was not found in list.");
    }
    void Clear()
    {
        sinks.clear();
    }
};

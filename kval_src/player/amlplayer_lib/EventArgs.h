#pragma once

/**
 * @brief The EventArgs class
 */
class EventArgs
{
    static EventArgs empty;

public:
    static EventArgs Empty()
    {
        return empty;
    }
};

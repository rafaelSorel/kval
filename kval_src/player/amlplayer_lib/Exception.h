#ifndef __EXCEPTION_CUST_H_
#define __EXCEPTION_CUST_H_

#include <exception>
#include <cstdio>
#include <iostream>

/**
 * @brief The Exception class
 */
class Exception : public std::exception
{
    std::string m_msg{};
public:
    Exception(const char* message): 
        m_msg(message) {}
    Exception(){}
    virtual ~Exception(){}
    virtual const char* what() const throw() {
        return m_msg.c_str();
    }
};

/**
 * @brief The NotSupportedException class
 */
class NotSupportedException : public Exception
{
public:
    NotSupportedException() {}

    NotSupportedException(const char* message)
        : Exception(message)
    {}

    virtual ~NotSupportedException() {}
};

/**
 * @brief The NotImplementedException class
 */
class NotImplementedException : public Exception
{
public:
    NotImplementedException()
    {}

    NotImplementedException(const char* message)
        : Exception(message)
    {}

    virtual ~NotImplementedException() {}
};

/**
 * @brief The ArgumentException class
 */
class ArgumentException : public Exception
{
public:
    ArgumentException()
    {}

    ArgumentException(const char* message)
        : Exception(message)
    {}

    virtual ~ArgumentException() {}
};

/**
 * @brief The ArgumentOutOfRangeException class
 */
class ArgumentOutOfRangeException : public Exception
{
public:
    ArgumentOutOfRangeException()
    {}

    ArgumentOutOfRangeException(const char* message)
        : Exception(message)
    {}

    virtual ~ArgumentOutOfRangeException() {}
};

/**
 * @brief The ArgumentNullException class
 */
class ArgumentNullException : public Exception
{
public:
    ArgumentNullException()
    {}

    ArgumentNullException(const char* message)
        : Exception(message)
    {}

    virtual ~ArgumentNullException() {}
};

/**
 * @brief The InvalidOperationException class
 */
class InvalidOperationException : public Exception
{
public:
    InvalidOperationException()
    {}

    InvalidOperationException(const char* message)
        : Exception(message)
    {}

    virtual ~InvalidOperationException() {}
};
#endif //__EXCEPTION_CUST_H_
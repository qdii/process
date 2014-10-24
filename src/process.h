#ifndef PS_PROCESS_H
#define PS_PROCESS_H

// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include <string>

namespace ps
{

/* @brief Describes a process */
template< typename T >
struct process
{
    explicit
    process( pid_t pid, const std::string & cmdline );

    process();

    /**@brief This is the name that makes most sense to a human
     *        It could be the title bar, or the product name as
     *        advertized by its creator */
    std::string title() const;

    /**@brief Checks whether this object is valid and describes
     *        a process (even a non-running, or non-existing one) */
    bool valid() const;


private:
    pid_t       m_pid;
    std::string m_cmdline;
};

template< typename T >
process<T>::process( pid_t pid, const std::string & cmdline )
    : m_pid( pid )
    , m_cmdline( cmdline )
{
}

template< typename T >
process<T>::process()
    : m_pid( 0 )
    , m_cmdline( "" )
{
}

template< typename T >
std::string process<T>::title() const
{
    return m_cmdline;
}

template< typename T >
bool process<T>::valid() const
{
    return m_pid != 0;
}

} // namespace ps

#endif // PS_PROCESS_H

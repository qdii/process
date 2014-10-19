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
    process( pid_t pid, std::string cmdline );

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

} // namespace ps

#endif // PS_PROCESS_H

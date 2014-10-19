#ifndef PS_LIST_H
#define PS_LIST_H
//
// VERSION 1.0
// AUTHOR: Victor Lavaud <victor.lavaud@gmail.com>

#include "process.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

namespace ps
{

template < typename T > static
bool read_entry_from_procfs( boost::filesystem::directory_iterator pos,
                             process<T> & out )
{
    using namespace boost::filesystem;

    // the path of a directory entry of /proc, something like /proc/12113
    const path entry_path = absolute( pos->path() );

    if ( !is_directory( entry_path ) )
        return false;

    // we store the pid, which is the *filename* of the entry (/proc/12113)
    pid_t pid = 0;
    try
    {
        pid = boost::lexical_cast<pid_t>( entry_path.filename().string().c_str() );
    }
    catch( const boost::bad_lexical_cast & )
    {
        // /proc also contains directory entries which are not processes, such as "dri"
        return false;
    }

    // we make it an absolute entry, because we are not necessarily executing from /proc
    // /proc/12113/cmdline contains the name of the process
    const path path_to_cmdline = entry_path / "cmdline";

    // if cmdline does not exist, it might be that we just don't have the rights to read /proc/12113
    if ( !exists( path_to_cmdline ) )
        return false;

    // if we do not have the rights to read cmdline, then creating the ifstream may fail
    std::ifstream cmdline( path_to_cmdline.c_str() );
    if ( !cmdline.is_open() )
        return false;

    // /proc/11241/cmdline contains the full name of the executable
    std::string process_name;
    std::getline( cmdline, process_name );

    // kernel threads, for instance, have empty cmdline
    if ( process_name.empty() )
        return false;

    out = process<T>( pid, process_name );
    return true;
}

template < typename T > static
std::vector< process<T> > get_entries_from_procfs()
{
    using namespace boost::filesystem;
    std::vector< process<T> > all_processes;

    directory_iterator pos( "/proc" );
    // until this is the last iterator, try and retrieve the process info
    for ( ; pos != directory_iterator(); ++pos )
    {
        process<T> next_process;
        if ( read_entry_from_procfs( pos, next_process ) )
            all_processes.push_back( std::move( next_process ) );
    }

    return all_processes;
}

template < typename T >
struct iterator
{
    explicit
    iterator( boost::filesystem::directory_iterator pos );

    process<T> operator*();
    iterator<T> operator++();

private:
    boost::filesystem::directory_iterator m_pos;
};

template< typename T >
iterator<T>::iterator( boost::filesystem::directory_iterator pos )
    : m_pos( pos )
{
}

template< typename T >
bool operator!=( const iterator<T>& a, const iterator<T>& b )
{
    return a.m_pos != b.m_pos;
}
    
template< typename T >
bool operator==( const iterator<T>& a, const iterator<T>& b )
{
    return a.m_pos == b.m_pos;
}

template < typename T >
process<T> iterator<T>::operator*()
{
    return read_entry_from_procfs( m_pos );
}

template < typename T >
iterator<T> end()
{
    return iterator<T>( boost::filesystem::directory_iterator() );
}

template < typename T >
iterator<T> iterator<T>::operator++()
{
    if ( m_pos != boost::filesystem::directory_iterator() )
        return iterator<T>( m_pos++ );

    return iterator<T>( m_pos );
}

template < typename T >
struct const_iterator
{
    const process<T> operator*();
    const_iterator operator++();
};

template < typename T >
iterator<T> begin()
{
}

template < typename T >
const_iterator<T> cbegin()
{
}

template < typename T >
const_iterator<T> cend()
{
}

}

#endif // PS_LIST_H

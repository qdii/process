#ifndef PS_JAVA_H
#define PS_JAVA_H

#include "common.h"

namespace ps
{

inline
std::vector< std::string > split_space_delimited_string( std::string input )
{
    std::istringstream buffer( input );

    return std::vector< std::string >(
        std::istream_iterator< std::string >( buffer ),
        std::istream_iterator< std::string >()
    );
}

inline
std::pair< std::string, std::string >
extract_name_and_icon_from_argv( const std::vector< std::string > & arguments )
{
    assert( !arguments.empty() );
    using namespace boost::filesystem;

    std::pair< std::string, std::string > result;
    std::string name;
    std::string icon;

    // These strings let java programmers tell OSX information
    // about their programs
    static const std::string ICON_TOKEN( "-Xdock:icon=" );
    static const std::string NAME_TOKEN( "-Xdock:name=" );

    // We look for them through the arguments of the program
    std::string arg;
    for ( unsigned i = 0, last = arguments.size(); i < last; ++i )
    {
        arg = arguments[i];
        auto namePos = arg.find( NAME_TOKEN );
        if ( namePos != std::string::npos )
        {
            name = arg.substr( namePos + NAME_TOKEN.size() );
            continue;
        }

        auto iconPos = arg.find( ICON_TOKEN );
        if ( iconPos != 0 )
            continue;

        icon = arg.substr( iconPos + ICON_TOKEN.size() );

        // hack: arguments sometimes contain white space, like in
        // the path /Library/Application Support, in which case
        // we need to append the next argument
        std::ostringstream aggregator;
        aggregator << icon;
        while ( is_regular_file( aggregator.str() ) == false )
        {
            if ( ++i == last )
                break;

            std::string nextArgument = arguments[i];
            aggregator << " " << nextArgument;
        }

        if ( is_regular_file( aggregator.str() ) )
            icon = aggregator.str();
    }

    if ( !name.empty() && !icon.empty() )
    {
        result.first  = std::move( name );
        result.second = std::move( icon );
    }

    return result;
}

inline
std::pair< std::string, std::string >
extract_name_and_icon_from_argv( std::string argv )
{    
    const std::vector< std::string > arguments =
        split_space_delimited_string( std::move( argv ) );

    return extract_name_and_icon_from_argv( arguments );
}

inline
std::string get_java_name( const pid_t pid )
{
    assert( pid != INVALID_PID );

    const auto & name_and_icon = 
        extract_name_and_icon_from_argv( ps::details::get_argv_from_pid( pid ) );

    return name_and_icon.first;
}

} // namespace ps
#endif // PS_JAVA_H

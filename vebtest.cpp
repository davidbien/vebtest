
// vebtest.cpp
// Test Van Emde Boas tree stuff.
// dbien: 11JUN2020

#include <thread>
#include "syslogmgr.h"
#include "jsonstrm.h"
#include "jsonobjs.h"
#include "_strutil.h"
#include "_vebtree.h"
#include "syslogmgr.inl"

std::string g_strProgramName;

__BIENUTIL_USING_NAMESPACE

int
TryMain( int argc, char *argv[] )
{
#define USAGE "Usage: %s"
    g_strProgramName = *argv;
    n_SysLog::InitSysLog( argv[0], LOG_PERROR, LOG_USER );
#if 0
    typedef VebTreeFixed< 65536 > _tyVebTree64;
    _tyVebTree64 veb;
    size_t stSizeOfVeb = sizeof(veb);
    const size_t stUniverse = _tyVebTree64::s_kstUniverse;
#else
    typedef VebTreeVariable< 256 > _tyVebTreeSummary;
    typedef VebTreeVariable< 65536, _tyVebTreeSummary > _tyVebTree64;
    _tyVebTree64 veb;
    veb.Init( 10000000 );
    const size_t stUniverse = 10000000;
#endif
    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        if ( 31 == stCur )
        {
            int Z = 1;
            ++Z;
        }
        veb.Insert( stCur );
    }
    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        if ( 63 == stCur )
        {
            int Z = 1;
            ++Z;
        }
        veb.Delete( stCur );
    }
    assert( !veb.FHasAnyElements() );
    return 0;
}

int
main( int argc, char **argv )
{
    try
    {
        return TryMain( argc, argv );
    }
    catch ( const std::exception & rexc )
    {
        n_SysLog::Log( eslmtError, "%s: *** Exception: [%s]", g_strProgramName.c_str(), rexc.what() );
        fprintf( stderr, "%s: *** Exception: [%s]\n", g_strProgramName.c_str(), rexc.what() );
        return -123;
    }
}


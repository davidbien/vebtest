
// vebtest.cpp
// Test Van Emde Boas tree stuff.
// dbien: 11JUN2020

#include <stdlib.h>
#include <thread>
#include <random>
#include "syslogmgr.h"
#include "jsonstrm.h"
#include "jsonobjs.h"
#include "_strutil.h"
#include "_simpbv.h"
#include "_vebtree.h"
#include "syslogmgr.inl"

std::string g_strProgramName;

__BIENUTIL_USING_NAMESPACE

int
TryMain( int argc, char *argv[] )
{
#define USAGE "Usage: %s [percentage populated(1-1000)=30][random seed]"
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
    const size_t stUniverse = 1000000;
    veb.Init( stUniverse );
#endif
    uint32_t nRandSeed = time(0);
    if ( argc > 2 )
         nRandSeed = atoi( argv[2] );
    n_SysLog::Log( eslmtInfo, "%s: iRandSeed[%d]", g_strProgramName.c_str(), nRandSeed );

    uint32_t nPercentPop = 30;
    if ( argc > 1 )
        nPercentPop = atoi( argv[1] );
    if ( nPercentPop > 1000 )
        nPercentPop = 1000;
    n_SysLog::Log( eslmtInfo, "%s: nPercentPop[%u/1000]", g_strProgramName.c_str(), nPercentPop );

    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        veb.Insert( stCur );
    }
    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        veb.Delete( stCur );
    }
    assert( !veb.FHasAnyElements() );
    assert( veb.FEmpty( true ) ); // Recursively ensure emptiness.

    // Now test successor and predecessor.
    typedef _simple_bitvec< size_t, allocator< size_t > > _tyBV;

    // Get a default random generator and seed it:
    std::default_random_engine rneDefault( nRandSeed );
    std::uniform_int_distribution< size_t > sidDist( 0, stUniverse-1 );
    auto genRand = std::bind( sidDist, rneDefault );

    _tyBV bvMirror( stUniverse );

    size_t nElementsGenerate = ( stUniverse * nPercentPop ) / 1000;
    size_t nInserted = 0;
    while( nElementsGenerate-- )
    {
        size_t st = genRand();
        bvMirror.setbit( st );
        nInserted += veb.FCheckInsert( st );
    }
    n_SysLog::Log( eslmtInfo, "%s: nInserted[%lu]", g_strProgramName.c_str(), nInserted );

    // Make some copies so we can reuse them:


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


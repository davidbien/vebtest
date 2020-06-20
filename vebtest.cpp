
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
    typedef VebTreeFixed< 65536 > _tyVebTree;
    _tyVebTree veb;
    size_t stSizeOfVeb = sizeof(veb);
    const size_t stUniverse = _tyVebTree::s_kstUniverse;
#else
    typedef VebTreeWrap< 256 > _tyVebTreeSummary;
    typedef VebTreeWrap< 65536, _tyVebTreeSummary > _tyVebTree;
    _tyVebTree veb;
    veb.AssertValid();
    const size_t stUniverse = 1000000;
    veb.Init( stUniverse );
    veb.AssertValid();
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
    else
    if ( !nPercentPop )
        nPercentPop = 30;
    n_SysLog::Log( eslmtInfo, "%s: nPercentPop[%u/1000]", g_strProgramName.c_str(), nPercentPop );

    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        veb.Insert( stCur );
    }
    veb.AssertValid();

    // Make some copies so we can reuse them:
    _tyVebTree vebCopy( veb );
    vebCopy.AssertValid();
    _tyVebTree vebCopy2;
    vebCopy2 = vebCopy;
    vebCopy2.AssertValid();

    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        vebCopy2.Delete( stCur );
    }
    vebCopy2.AssertValid();
    assert( !vebCopy2.FHasAnyElements() );
    assert( vebCopy2.FEmpty( true ) ); // Recursively ensure emptiness.
    vebCopy2.swap( veb );
    assert( veb.FEmpty( true ) ); // Recursively ensure emptiness.

    // Now test successor and predecessor.
    typedef _simple_bitvec< size_t, allocator< size_t > > _tyBV;

    // Get a default random generator and seed it:
    std::default_random_engine rneDefault( nRandSeed );
    std::uniform_int_distribution< size_t > sidDist( 0, stUniverse-1 );
    auto genRand = std::bind( sidDist, rneDefault );

    _tyBV bvMirror( stUniverse );
    bvMirror.clear();
    uint64_t nSetBitsInit = bvMirror.countsetbits();

    size_t nElementsGenerate = ( stUniverse * nPercentPop ) / 1000;
    size_t nInserted = 0;
    while( nElementsGenerate-- )
    {
        size_t st = genRand();
        bvMirror.setbit( st );
        nInserted += veb.FCheckInsert( st );
        veb.AssertValid();
    }
    n_SysLog::Log( eslmtInfo, "%s: nInserted[%lu]", g_strProgramName.c_str(), nInserted );
    uint64_t nSetBitsBefore = bvMirror.countsetbits();
    assert( nInserted == nSetBitsBefore );

    vebCopy2 = veb;
    assert( vebCopy2 == veb );
    assert( veb == vebCopy2 );
    vebCopy.Clear();
    vebCopy.swap( vebCopy2 );
    assert( vebCopy2.FEmpty( true ) );
    _tyVebTree vebCopy3( std::move( vebCopy ) );
    //vebCopy2 = std::move( vebCopy );
    assert( vebCopy.FEmpty( true ) );
    assert( vebCopy3 == veb );
    assert( veb == vebCopy3 );

    vebCopy2 = std::move( vebCopy3 );
    assert( vebCopy3.FEmpty( true ) );
    assert( vebCopy2 == veb );

    // Now let's actually get to the successor and predecessor testing:
    _tyBV bvMirrorCopy( bvMirror );
    uint64_t nSetBitsBeforeCopy = bvMirrorCopy.countsetbits();
    assert( nSetBitsBeforeCopy == nSetBitsBefore );

    { //B: Successor:
        // Algorithm: Move through and get each successive bit and clear it in the bvMirror.
        // Then bvMirror should be empty at the end.
        // Boundary: Bit 0.
        if ( veb.FHasElement( 0 ) )
            bvMirror.clearbit( 0 );
        size_t nCurEl = 0;
        size_t nSuccessor;
        while( !!( nSuccessor = veb.NSuccessor( nCurEl ) ) )
        {
            assert( nSuccessor > nCurEl );
            assert( bvMirror.isbitset( nSuccessor ) );
            bvMirror.clearbit( nSuccessor );
            nCurEl = nSuccessor;
        }
        uint64_t nSetBitsAfter = bvMirror.countsetbits();
        assert( !nSetBitsAfter );
    } //EB

    { //B: Predecessor:
        // Algorithm: Move through and get each predecessive bit and clear it in the bvMirrorCopy.
        // Then bvMirrorCopy should be empty at the end.
        // Boundary: Bit 0.
        if ( veb.FHasElement( stUniverse-1 ) )
            bvMirrorCopy.clearbit( stUniverse-1 );
        //size_t nCurEl = 983084;
        size_t nCurEl = stUniverse-1;
        size_t nPredecessor;
        size_t nthCall = 0;
        while( _tyVebTree::s_kitNoPredecessor != ( nPredecessor = veb.NPredecessor( nCurEl ) ) )
        {
            assert( nPredecessor < nCurEl );
            assert( bvMirrorCopy.isbitset( nPredecessor ) );
            bvMirrorCopy.clearbit( nPredecessor );
            nCurEl = nPredecessor;
            ++nthCall;
        }
        assert( !bvMirrorCopy.countsetbits() );
    } //EB

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


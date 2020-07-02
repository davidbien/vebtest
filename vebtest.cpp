
// vebtest.cpp
// Test Van Emde Boas tree stuff.
// dbien: 11JUN2020

#include <stdlib.h>
#include <thread>
#include <random>
#include <chrono>
#include "syslogmgr.h"
#include "jsonstrm.h"
#include "jsonobjs.h"
#include "_strutil.h"
#include "_simpbv.h"
#include "_vebtree.h"
#include "syslogmgr.inl"

std::string g_strProgramName;

__BIENUTIL_USING_NAMESPACE

template < class t_tyTest >
chrono::steady_clock::duration
TimeTest( t_tyTest _tTest, uint32_t _nIterations )
{
    chrono::steady_clock::duration dTotal = 0;
    while( _nIterations-- )
    {
        chrono::steady_clock::time_point tpBegin = chrono::steady_clock::now();
        _tTest(); // run test.
        chrono::steady_clock::time_point tpEnd = chrono::steady_clock::now();
        dTotal += ( tpEnd - tpBegin );
        _tTest.reset(); // We don't time the reset.
    }
}

class TestBase
{
    uint32_t m_nIterations;
    string m_strDesc;
};

// Run all timing tests with the given random seed, percentage population, and number of elements.
void
TimingTests( uint32_t _nIterations, uint32_t _nRandSeed, uint32_t _nPercentPop, uint64_t _nElements )
{
    std::default_random_engine rneDefault( _nRandSeed );
    std::uniform_int_distribution< size_t > sidDist( 0, _nElements-1 );
    auto genRand = std::bind( sidDist, rneDefault );

    // Both VebTree and simple_bitvec just memset their memory to zero - no need to time this.
    typedef VebTreeWrap< 256 > _tyVebTreeSummary;
    typedef VebTreeWrap< 65536, _tyVebTreeSummary > _tyVebTree;
    _tyVebTree vebTest( _nElements );

    typedef _simple_bitvec< uint64_t, allocator< uint64_t > > _tyBV;
    _tyBV sbvTest( _nElements );
}

int
TryMain( int argc, char *argv[] )
{
#define USAGE "Usage: %s nElements nIterations [percentage populated(1-1000)=30] [random seed]"
    typedef JsoValue< char > _tyJsoValue;
    g_strProgramName = *argv;

    if ( argc < 3 )
    {
        fprintf( stderr, USAGE "\n", *argv );
        return -1;
    }
    uint64_t nElements = strtoul( argv[1], 0, 0 ); // allow hex numbers to be entered.
    if ( !nElements )
        nElements = 100;
    uint64_t nIterations = strtoul( argv[2], 0, 0 ); // allow hex numbers to be entered.
    if ( !nIterations )
        nIterations = 1;
    uint32_t nRandSeed = time(0);
    if ( argc > 4 )
    {
        uint64_t n64RandSeed = strtoul( argv[4], 0, 0 );
        if ( n64RandSeed > UINT32_MAX )
        {
            fprintf( stderr, "%s: nRandSeed is greater than INT32_MAX nRandSeed[%lu] UINT32_MAX[%lu].\n", argv[0], n64RandSeed, uint64_t( UINT32_MAX ) );
            fprintf( stderr, USAGE "\n", *argv );
            return -2;
        }
        nRandSeed = n64RandSeed;
    }

    uint32_t nPercentPop = 30;
    if ( argc > 3 )
        nPercentPop = atoi( argv[3] );
    if ( nPercentPop > 1000 )
        nPercentPop = 1000;
    else
    if ( !nPercentPop )
        nPercentPop = 30;
    
    {//B
        _tyJsoValue jvLog( ejvtObject );
        jvLog("nElements").SetValue( nElements );
        jvLog("nIterations").SetValue( nIterations );
        jvLog("nRandSeed").SetValue( nRandSeed );
        jvLog("nPercentPop").SetValue( nPercentPop );
        n_SysLog::InitSysLog( argv[0], LOG_PERROR, LOG_USER, &jvLog );
    }//EB

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
    size_t stUniverse = nElements;// INT_MAX + 65535ull;
    veb.Init( stUniverse );
    veb.AssertValid();
#endif

    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        veb.Insert( stCur );
    }
    veb.AssertValid();

    _tyVebTree vebAll( stUniverse );
    vebAll.InsertAll();
    vebAll.AssertValid();
    assert( vebAll == veb );

    // Make some copies so we can reuse them:
    _tyVebTree vebCopy( veb );
    vebCopy.AssertValid();
    _tyVebTree vebCopy2;
    vebCopy2 = vebCopy;
    vebCopy2.AssertValid();

    for ( size_t stCur = 0; stCur < stUniverse; ++stCur )
    {
        bool fDeleted = vebCopy2.FCheckDelete( stCur );
        assert( fDeleted );
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
    }
    veb.AssertValid();
    n_SysLog::Log( eslmtInfo, "%s: nInserted[%lu]", g_strProgramName.c_str(), nInserted );
    uint64_t nSetBitsBefore = bvMirror.countsetbits();
    assert( nInserted == nSetBitsBefore );

    {//B - test bitwise functions:
        _tyVebTree vebInvert( veb );
        vebInvert.BitwiseInvert();
        vebInvert.AssertValid();
        vebInvert &= veb;
        vebInvert.AssertValid();
        assert( vebInvert.FEmpty( true ) );
        vebInvert |= veb;
        assert( vebInvert == veb );
        vebInvert.BitwiseInvert();
        vebInvert ^= veb; // This should make vebInvert fully occupied.
        vebInvert.AssertValid();
        vebInvert.BitwiseInvert();
        assert( vebInvert.FEmpty( true ) );
    }//EB

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
    _tyBV bvMirrorCopy2( bvMirror );

    { //B: Successor:
        // Algorithm: Move through and get each successive bit and clear it in the bvMirror.
        // Then bvMirror should be empty at the end.
        size_t nCurEl = _tyVebTree::s_kitNoSuccessor;
        size_t nSuccessor;
        while( _tyVebTree::s_kitNoSuccessor != ( nSuccessor = veb.NSuccessor( nCurEl ) ) )
        {
            assert( ( nCurEl == numeric_limits< size_t >::max() ) || ( nSuccessor > nCurEl ) );
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
        size_t nCurEl = _tyVebTree::s_kitNoPredecessor;
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

    _tyBV bvMirrorCopy3( bvMirrorCopy2 );
    { //B: SuccessorDelete:
        // Algorithm: Move through and get each successive bit and clear it in the bvMirror.
        // Then bvMirror should be empty at the end.
        // Boundary: Bit 0.
        _tyVebTree vebSuccessorDelete( veb );
        size_t nCurEl = _tyVebTree::s_kitNoSuccessor;
        size_t nSuccessor;
        while( _tyVebTree::s_kitNoSuccessor != ( nSuccessor = vebSuccessorDelete.NSuccessorDelete( nCurEl ) ) )
        {
            assert( ( nCurEl == numeric_limits< size_t >::max() ) || ( nSuccessor > nCurEl ) );
            assert( bvMirrorCopy2.isbitset( nSuccessor ) );
            bvMirrorCopy2.clearbit( nSuccessor );
            nCurEl = nSuccessor;
        }
        uint64_t nSetBitsAfter = bvMirrorCopy2.countsetbits();
        assert( !nSetBitsAfter );
        assert( vebSuccessorDelete.FEmpty( true ) );
    } //EB

    { //B: PredecessorDelete:
        // Algorithm: Move through and get each predecessive bit and clear it in the bvMirrorCopy.
        // Then bvMirrorCopy should be empty at the end.
        // Boundary: Bit 0.
        _tyVebTree vebPredecessorDelete( veb );
        //size_t nCurEl = 983084;
        size_t nCurEl = _tyVebTree::s_kitNoPredecessor;
        size_t nPredecessor;
        while( _tyVebTree::s_kitNoPredecessor != ( nPredecessor = vebPredecessorDelete.NPredecessorDelete( nCurEl ) ) )
        {
            assert( nPredecessor < nCurEl );
            assert( bvMirrorCopy3.isbitset( nPredecessor ) );
            bvMirrorCopy3.clearbit( nPredecessor );
            nCurEl = nPredecessor;
        }
        assert( !bvMirrorCopy3.countsetbits() );
        assert( vebPredecessorDelete.FEmpty( true ) );
    } //EB

    veb.Resize( stUniverse /= 2, true );
    veb.AssertValid();

    for ( size_t stCur = stUniverse; --stCur; )
    {
        (void)veb.FCheckDelete( stCur );
    }
    assert( veb.FEmpty( true ) );

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

#if 0
    const clockid_t cidClockTypeBegin = CLOCK_REALTIME;
    const clockid_t cidClockTypeEnd = CLOCK_TAI;
    for ( int cidCur = cidClockTypeBegin; cidCur <= cidClockTypeEnd; ++cidCur )
    {
        timespec ts;
        int n = clock_getres( cidCur, &ts );
        if ( !n )
            printf( "clock_getres(): cidCur[%d] ts.tv_sec[%ld] ts.tv_nsec[%ld].\n", cidCur, (size_t)ts.tv_sec, ts.tv_nsec );
        else
            fprintf( stderr, "clock_getres(): failed with errno[%d].\n", errno );
        
    }
#endif 
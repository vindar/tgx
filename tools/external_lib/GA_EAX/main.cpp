#ifndef __ENVIRONMENT__
#include "env.h"
#endif

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static bool WriteSmallExact( TEnvironment* gEnv, char* dstFile )
{
  const int n = gEnv->fEvaluator->Ncity;
  if( n < 2 || n > 8 )
    return false;

  std::vector<int> order( n );
  std::vector<int> best( n );
  for( int i = 0; i < n; ++i )
    order[ i ] = i;
  best = order;

  int bestValue = 0x7fffffff;
  do {
    int value = 0;
    for( int i = 0; i < n; ++i )
      value += gEnv->fEvaluator->Direct( order[ i ], order[ ( i + 1 ) % n ] );
    if( value < bestValue ){
      bestValue = value;
      best = order;
      if( bestValue <= 1 )
        break;
    }
  } while( std::next_permutation( order.begin() + 1, order.end() ) );

  for( int i = 0; i < n; ++i ){
    const int current = best[ i ];
    gEnv->tBest.fLink[ current ][ 0 ] = best[ ( i + n - 1 ) % n ];
    gEnv->tBest.fLink[ current ][ 1 ] = best[ ( i + 1 ) % n ];
  }
  gEnv->tBest.fEvaluationValue = bestValue;
  gEnv->WriteBest( dstFile );
  return true;
}

int main( int argc, char* argv[] )
{
  if( argc < 6 ){
    printf( "usage: tgx_ga_eax_stripifier graph.txt output_prefix pop kids max_generations [max_seconds] [initial_population] [rng_seed]\n" );
    return 1;
  }

  TEnvironment* gEnv = new TEnvironment();
  int rngSeed = argc >= 9 ? atoi( argv[8] ) : 1111;
  InitURandom( rngSeed );

  gEnv->fFileNameTSP = argv[1];
  char* dstFile = argv[2];
  gEnv->fNumOfPop = atoi( argv[3] );
  gEnv->fNumOfKids = atoi( argv[4] );
  gEnv->fMaxGenerations = atoi( argv[5] );
  gEnv->fMaxSeconds = argc >= 7 ? atoi( argv[6] ) : 0;
  gEnv->fFileNameInitPop = ( argc >= 8 && argv[7][0] != '-' ) ? argv[7] : NULL;
  gEnv->fTargetValue = 1;

  gEnv->Define();
  if( WriteSmallExact( gEnv, dstFile ) )
    return 0;
  gEnv->DoIt();
  gEnv->PrintOn( 0, dstFile );
  gEnv->WriteBest( dstFile );

  /* The original GA-EAX code has fragile destructor paths on some small
     sparse 0/1 instances. This command-line tool is short-lived, so let the
     OS reclaim the process memory after the result files have been written. */
  return 0;
}

#ifndef __ENVIRONMENT__
#include "env.h"
#endif

#include <stdio.h>
#include <stdlib.h>

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
  gEnv->fFileNameInitPop = argc >= 8 ? argv[7] : NULL;

  gEnv->Define();
  gEnv->DoIt();
  gEnv->PrintOn( 0, dstFile );
  gEnv->WriteBest( dstFile );

  /* The original GA-EAX code has fragile destructor paths on some small
     sparse 0/1 instances. This command-line tool is short-lived, so let the
     OS reclaim the process memory after the result files have been written. */
  return 0;
}

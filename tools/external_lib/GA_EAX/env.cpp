#ifndef __ENVIRONMENT__
#include "env.h"
#endif

#include <math.h> 
     
void MakeRandSol( TEvaluator* eval , TIndi& indi );
void Make2optSol( TEvaluator* eval , TIndi& indi );

TEnvironment::TEnvironment()
{
  fEvaluator = new TEvaluator();
  fMaxGenerations = 0;
  fMaxSeconds = 0;
  fTargetValue = -1;
}


TEnvironment::~TEnvironment()
{
  int N = fEvaluator->Ncity;
  delete [] fIndexForMating;
  delete [] tCurPop;
  delete tCross;
  delete tKopt;

  for( int i = 0; i < N; ++i ) 
    delete [] fEdgeFreq[ i ];
  delete [] fEdgeFreq;
  delete fEvaluator;
}


void TEnvironment::Define()
{
  fEvaluator->SetInstance( fFileNameTSP );
  int N = fEvaluator->Ncity;

  fIndexForMating = new int [ fNumOfPop + 1 ];  

  tCurPop = new TIndi [ fNumOfPop ];
  for ( int i = 0; i < fNumOfPop; ++i )
    tCurPop[i].Define( N );

  tBest.Define( N );

  tCross = new TCross( N );
  tCross->eval = fEvaluator;                 
  tCross->fNumOfPop = fNumOfPop;             

  tKopt = new TKopt( N );
  tKopt->eval = fEvaluator;
  tKopt->SetInvNearList();

  fEdgeFreq = new int* [ N ]; 
  for( int i = 0; i < N; ++i ) 
    fEdgeFreq[ i ] = new int [ fEvaluator->fNearNumMax+1 ]; // Large
}


void TEnvironment::DoIt()
{
  this->fTimeStart = clock();   

  if( fFileNameInitPop == NULL )
    this->InitPop();                       
  else
    this->ReadPop( fFileNameInitPop );     

  this->fTimeInit = clock();    

  this->Init();
  this->GetEdgeFreq();

  while( 1 )
 {
    this->SetAverageBest();
    printf( "%d: %d %lf\n", fCurNumOfGen, fBestValue, fAverageValue );

    if( this->TerminationCondition() ) break;

    this->SelectForMating();

    for( int s =0; s < fNumOfPop; ++s )
    {
      this->GenerateKids( s );     
      this->SelectForSurvival( s ); 
    }
    ++fCurNumOfGen;
  }

  this->fTimeEnd = clock();   
}
 
/* See Section 2.2 */
void TEnvironment::Init()
{
  fAccumurateNumCh = 0;
  fCurNumOfGen = 0;
  fStagBest = 0;
  fMaxStagBest = 0;
  fStage = 1;          /* Stage I */
  fFlagC[ 0 ] = 4;     /* Diversity preservation: 1:Greedy, 2:--- , 3:Distance, 4:Entropy (see Section 4) */
  fFlagC[ 1 ] = 1;     /* Eset Type: 1:Single-AB, 2:Block2 (see Section 3) */ 
} 

/* See Section 2.2 */
bool TEnvironment::TerminationCondition()
{
  if( fTargetValue >= 0 && fBestValue <= fTargetValue )
    return true;
  if( fMaxGenerations > 0 && fCurNumOfGen >= fMaxGenerations )
    return true;
  if( fMaxSeconds > 0 ){
    int elapsed = (int)((double)(clock() - fTimeStart)/(double)CLOCKS_PER_SEC);
    if( elapsed >= fMaxSeconds )
      return true;
  }

  if ( fAverageValue - fBestValue < 0.001 )  
    return true;

  if( fStage == 1 ) /* Stage I */      
  {
    if( fStagBest == int(1500/fNumOfKids) && fMaxStagBest == 0 ){ /* 1500/N_ch (See Section 2.2) */
      fMaxStagBest =int( fCurNumOfGen / 10 );                  /* fMaxStagBest = G/10 (See Section 2.2) */
    } 
    else if( fMaxStagBest != 0 && fMaxStagBest <= fStagBest ){ /* Terminate Stage I (proceed to Stage II) */
      fStagBest = 0;
      fMaxStagBest = 0;
      fCurNumOfGen1 = fCurNumOfGen;
      fFlagC[ 1 ] = 2; 
      fStage = 2;      
    }
    return false;
  }

  if( fStage == 2 ){ /* Stage II */
    if( fStagBest == int(1500/fNumOfKids) && fMaxStagBest == 0 ){ /* 1500/N_ch (See Section 2.2) */
      fMaxStagBest = int( (fCurNumOfGen - fCurNumOfGen1) / 10 ); /* fMaxStagBest = G/10 (See Section 2.2) */
    } 
    else if( fMaxStagBest != 0 && fMaxStagBest <= fStagBest ){ /* Terminate Stage II and GA */
      return true;
    }

    return false;
  }

  return false;
}


void TEnvironment::SetAverageBest() 
{
  int stockBest = tBest.fEvaluationValue;
  
  fAverageValue = 0.0;
  fBestIndex = 0;
  fBestValue = tCurPop[0].fEvaluationValue;
  
  for(int i = 0; i < fNumOfPop; ++i ){
    fAverageValue += tCurPop[i].fEvaluationValue;
    if( tCurPop[i].fEvaluationValue < fBestValue ){
      fBestIndex = i;
      fBestValue = tCurPop[i].fEvaluationValue;
    }
  }
  
  tBest = tCurPop[ fBestIndex ];
  fAverageValue /= (double)fNumOfPop;

  if( tBest.fEvaluationValue < stockBest ){
    fStagBest = 0;
    fBestNumOfGen = fCurNumOfGen;
    fBestAccumeratedNumCh = fAccumurateNumCh;
  }
  else ++fStagBest;
}


void TEnvironment::InitPop()
{
  for ( int i = 0; i < fNumOfPop; ++i ){ 
    tKopt->MakeRandSol( tCurPop[ i ] );    /* Make a random tour */
    tKopt->DoIt( tCurPop[ i ] );           /* Apply the local search with the 2-opt neighborhood */ 
    fEvaluator->TranceLinkOrder( tCurPop[ i ] );  // Large 
  }
}


void TEnvironment::SelectForMating()
{
  /* fIndexForMating[] <-- a random permutation of 0, ..., fNumOfPop-1 */
  tRand->Permutation( fIndexForMating, fNumOfPop, fNumOfPop ); 
  fIndexForMating[ fNumOfPop ] = fIndexForMating[ 0 ];
}

void TEnvironment::SelectForSurvival( int s )
{
}


void TEnvironment::GenerateKids( int s )
{
  tCross->SetParents( tCurPop[fIndexForMating[s]], tCurPop[fIndexForMating[s+1]], fFlagC, fNumOfKids );  
  
  /* Note: tCurPop[fIndexForMating[s]] is replaced with a best offspring solutions in tCorss->DoIt(). 
     fEegeFreq[][] is also updated there. */
  tCross->DoIt( tCurPop[fIndexForMating[s]], tCurPop[fIndexForMating[s+1]], fNumOfKids, 1, fFlagC, fEdgeFreq );

  fAccumurateNumCh += tCross->fNumOfGeneratedCh;
}


void TEnvironment::GetEdgeFreq()  // Large
{
  int N = fEvaluator->Ncity;
  int k0, k1;
  
  for( int j1 = 0; j1 < N; ++j1 )
    for( int j2 = 0; j2 < fEvaluator->fNearNumMax; ++j2 ) 
      fEdgeFreq[ j1 ][ j2 ] = 0;

  
  for( int i = 0; i < fNumOfPop; ++i )
  {
    for(int j = 0; j < N; ++j )
    {
      k0 = tCurPop[ i ].fOrder[ j ][ 0 ];      
      k1 = tCurPop[ i ].fOrder[ j ][ 1 ];
      if( k0 != -1 ) 
	++fEdgeFreq[ j ][ k0 ];
      if( k1 != -1 ) 
      ++fEdgeFreq[ j ][ k1 ];
    }
  }
}


void TEnvironment::PrintOn( int n, char* dstFile ) 
{
  printf( "n = %d val = %d Gen = %d Time = %d %d\n" , 
	  n, 
	  tBest.fEvaluationValue, 
	  fCurNumOfGen, 
	  (int)((double)(this->fTimeInit - this->fTimeStart)/(double)CLOCKS_PER_SEC), 
	  (int)((double)(this->fTimeEnd - this->fTimeStart)/(double)CLOCKS_PER_SEC) );
  fflush(stdout);

  FILE *fp;
  char filename[ 1024 ];
  sprintf( filename, "%s_Result", dstFile );
  fp = fopen( filename, "a");
  
  fprintf( fp, "%d %d %d %d %d\n" , 
	   n, 
	   tBest.fEvaluationValue, 
	   fCurNumOfGen, 
	   (int)((double)(this->fTimeInit - this->fTimeStart)/(double)CLOCKS_PER_SEC), 
	   (int)((double)(this->fTimeEnd - this->fTimeStart)/(double)CLOCKS_PER_SEC) );
  
  fclose( fp );
}


void TEnvironment::WriteBest( char* dstFile ) 
{
  FILE *fp;
  char filename[ 1024 ];
  sprintf( filename, "%s_BestSol", dstFile );
  fp = fopen( filename, "a");
  
  fEvaluator->WriteTo( fp, tBest );

  fclose( fp );
}


void TEnvironment::WritePop( int n, char* dstFile ) 
{
  FILE *fp;
  char filename[ 1024 ];
  sprintf( filename, "%s_POP_%d", dstFile, n );
  fp = fopen( filename, "w");

  for( int s = 0; s < fNumOfPop; ++s )
    fEvaluator->WriteTo( fp, tCurPop[ s ] );

  fclose( fp );
}


void TEnvironment::ReadPop( char* fileName )
{
  FILE* fp;

  if( ( fp = fopen( fileName, "r" ) ) == NULL ){
    printf( "Read Error1\n"); 
    fflush( stdout );
    exit( 1 );
  }

  for ( int i = 0; i < fNumOfPop; ++i ){ 
    if( fEvaluator->ReadFrom( fp, tCurPop[ i ] ) == false ){
      printf( "Read Error2\n"); 
      fflush( stdout );
      exit( 1 );
    }
    fEvaluator->TranceLinkOrder( tCurPop[ i ] );  // Large
  }
  fclose( fp );
}



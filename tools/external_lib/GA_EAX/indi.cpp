#ifndef __INDI__
#include "indi.h"
#endif



TIndi::TIndi()
{                
  fN = 0;
  fLink = NULL;
  fOrder = NULL;           // Large
  fEvaluationValue = 0;
}


TIndi::~TIndi()
{
  for ( int i = 0; i < fN; ++i ) 
    delete[] fLink[ i ];
  delete[] fLink;
  for ( int i = 0; i < fN; ++i )  // Large
    delete[] fOrder[ i ];
  delete[] fOrder;
}


void TIndi::Define( int N )
{
  fN = N;
  
  fLink = new int* [ fN ];
  for( int i = 0; i < fN; ++i ) 
    fLink[ i ] = new int [ 2 ];
  fOrder = new int* [ fN ];        // Large
  for( int i = 0; i < fN; ++i ) 
    fOrder[ i ] = new int [ 2 ];
} 


TIndi& TIndi::operator = ( const TIndi& src )
{
  fN = src.fN;

  for ( int i = 0; i < fN; ++i ){ 
    for ( int j = 0; j < 2; ++j ){ 
      fLink[i][j] = src.fLink[i][j];
      fOrder[i][j] = src.fOrder[i][j];  // Large
    }
  }
  fEvaluationValue = src.fEvaluationValue;

  return *this;
}

bool TIndi::operator == ( const TIndi& src )
{
  int curr,next,pre;
  int flag_identify;

  if( fN != src.fN )  
    return false;
  if( fEvaluationValue != src.fEvaluationValue )  
    return false;
  
  curr = 0;
  pre = -1;
  for( int i = 0; i < fN; ++i )
  {
    if( fLink[curr][0] == pre ) 
      next = fLink[curr][1];
    else 
      next = fLink[curr][0];
	
    if( src.fLink[curr][0] != next && src.fLink[curr][1] != next ) 
    {
      return false;
    }

    pre = curr;    
    curr = next; 
  }

  return true;
}



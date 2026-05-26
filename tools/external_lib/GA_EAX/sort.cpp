#ifndef __Sort__
#include "sort.h"
#endif

TSort* tSort = NULL;

void InitSort( void )
{
  tSort = new TSort();
}

TSort::TSort()
{
}

TSort::~TSort()
{
}


void TSort::Index( double* Arg, int numOfArg, int* indexOrderd, int numOfOrd )
{
  int indexBest = 0;
  double valueBest;
  int *checked;
  checked = new int [ numOfArg ];

  assert( Arg[0] < 99999999999.9 );

  for( int i = 0 ; i < numOfArg ; ++i ) 
    checked[ i ] = 0;
  
  for( int i = 0; i < numOfOrd; ++i )
  {
    valueBest = 99999999999.9;
    for( int j = 0; j < numOfArg; ++j )
    {
      if( ( Arg[j] < valueBest ) && checked[j]==0){
	valueBest = Arg[j];
	indexBest = j;
      }
    }
    indexOrderd[ i ]=indexBest;
    checked[ indexBest ]=1;
  }

  delete [] checked;
}


void TSort::Index_B( double* Arg, int numOfArg, int* indexOrderd, int numOfOrd )
{
  int indexBest = 0; 
  double valueBest;
  int *checked;
  checked = new int [ numOfArg ];

  assert( Arg[0] > -99999999999.9 );

  for( int i = 0 ; i < numOfArg ; ++i ) 
    checked[ i ] = 0;
  
  for( int i = 0; i < numOfOrd; ++i )
  {
    valueBest = -99999999999.9;
    for( int j = 0; j < numOfArg; ++j )
    {
      if( ( Arg[j] > valueBest ) && checked[j]==0){
	valueBest = Arg[j];
	indexBest = j;
      }
    }
    indexOrderd[ i ]=indexBest;
    checked[ indexBest ]=1;
  }

  delete [] checked;
}


void TSort::Index( int* Arg, int numOfArg, int* indexOrderd, int numOfOrd )
{
  int indexBest = 0;
  int valueBest;
  int *checked;
  checked = new int [ numOfArg ];

  assert( Arg[0] < 99999999 );

  for( int i = 0 ; i < numOfArg ; ++i ) 
    checked[ i ] = 0;
  
  for( int i = 0; i < numOfOrd; ++i )
  {
    valueBest = 99999999;
    for( int j = 0; j < numOfArg; ++j )
    {
      if( ( Arg[j] < valueBest ) && checked[j]==0){
	valueBest = Arg[j];
	indexBest = j;
      }
    }
    indexOrderd[ i ]=indexBest;
    checked[ indexBest ]=1;
  }

  delete [] checked;
}


void TSort::Index_B( int* Arg, int numOfArg, int* indexOrderd, int numOfOrd )
{
  int indexBest = 0;
  int valueBest;
  int *checked;
  checked = new int [ numOfArg ];

  assert( Arg[0] > -999999999 );

  for( int i = 0 ; i < numOfArg ; ++i ) 
    checked[ i ] = 0;
  
  for( int i = 0; i < numOfOrd; ++i )
  {
    valueBest = -999999999;
    for( int j = 0; j < numOfArg; ++j )
    {
      if( ( Arg[j] > valueBest ) && checked[j]==0){
	valueBest = Arg[j];
	indexBest = j;
      }
    }
    indexOrderd[ i ]=indexBest;
    checked[ indexBest ]=1;
  }

  delete [] checked;
}


void TSort::Sort( int* Arg, int numOfArg )
{
  int indexBest = 0;
  int valueBest;
  int stock;

  assert( Arg[0] < 99999999 );

  for( int i = 0; i < numOfArg; ++i )
  {
    valueBest = 99999999;
    for( int j = i; j < numOfArg; ++j )
    {
      if( ( Arg[j] < valueBest ) ){
	valueBest = Arg[j];
	indexBest = j;
      }
    }
    stock = Arg[ i ];
    Arg[ i ] = Arg[ indexBest ];
    Arg[ indexBest ] = stock;
  }
}

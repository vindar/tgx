#ifndef __EVALUATOR__
#include "evaluator.h"
#endif

#include <stdlib.h>

TEvaluator::TEvaluator()
{
  fEdgeDisOrder = NULL;
  fNearCity = NULL;
  Ncity = 0;
  fNearNumMax = 50;
  x = NULL;
  y = NULL;
}

TEvaluator::~TEvaluator()
{
  if( fEdgeDisOrder != NULL ){
    for( int i = 0; i < Ncity; ++i )
      delete [] fEdgeDisOrder[ i ];
    delete [] fEdgeDisOrder;
  }
  if( fNearCity != NULL ){
    for( int i = 0; i < Ncity; ++i )
      delete [] fNearCity[ i ];
    delete [] fNearCity;
  }
  delete [] x;
  delete [] y;
}

bool TEvaluator::IsAdjacent( int i, int j ) const
{
  if( i < 0 || i >= Ncity || j < 0 || j >= Ncity || i == j )
    return false;
  const std::vector<int>& adj = fAdj[ i ];
  for( size_t k = 0; k < adj.size(); ++k )
    if( adj[ k ] == j )
      return true;
  return false;
}

void TEvaluator::SetInstance( char filename[] )
{
  FILE* fp = fopen( filename, "r" );
  if( fp == NULL ){
    printf( "Cannot open graph file: %s\n", filename );
    exit( 1 );
  }

  if( fscanf( fp, "%d", &Ncity ) != 1 || Ncity <= 0 ){
    printf( "Invalid TGX graph header\n" );
    exit( 1 );
  }

  fAdj.assign( Ncity, std::vector<int>() );
  for( int i = 0; i < Ncity; ++i ){
    int deg = 0;
    if( fscanf( fp, "%d", &deg ) != 1 || deg < 0 ){
      printf( "Invalid adjacency row %d\n", i );
      exit( 1 );
    }
    fAdj[ i ].reserve( deg );
    for( int k = 0; k < deg; ++k ){
      int n = -1;
      if( fscanf( fp, "%d", &n ) != 1 || n < 0 || n >= Ncity || n == i ){
        printf( "Invalid neighbor in row %d\n", i );
        exit( 1 );
      }
      fAdj[ i ].push_back( n );
    }
  }
  fclose( fp );

  x = new double [ Ncity ];
  y = new double [ Ncity ];
  for( int i = 0; i < Ncity; ++i ){
    x[ i ] = (double)i;
    y[ i ] = 0.0;
  }

  fEdgeDisOrder = new int* [ Ncity ];
  fNearCity = new int* [ Ncity ];
  for( int i = 0; i < Ncity; ++i ){
    fEdgeDisOrder[ i ] = new int [ fNearNumMax + 1 ];
    fNearCity[ i ] = new int [ fNearNumMax + 1 ];
  }

  for( int i = 0; i < Ncity; ++i ){
    int pos = 0;
    fNearCity[ i ][ pos ] = i;
    fEdgeDisOrder[ i ][ pos ] = 0;
    ++pos;

    for( size_t k = 0; k < fAdj[ i ].size() && pos <= fNearNumMax; ++k ){
      fNearCity[ i ][ pos ] = fAdj[ i ][ k ];
      fEdgeDisOrder[ i ][ pos ] = 0;
      ++pos;
    }

    int step = 1;
    while( pos <= fNearNumMax ){
      int c = ( i + step ) % Ncity;
      ++step;
      if( c == i || IsAdjacent( i, c ) )
        continue;
      bool duplicate = false;
      for( int k = 0; k < pos; ++k )
        if( fNearCity[ i ][ k ] == c )
          duplicate = true;
      if( duplicate )
        continue;
      fNearCity[ i ][ pos ] = c;
      fEdgeDisOrder[ i ][ pos ] = 1;
      ++pos;
      if( step > Ncity + fNearNumMax + 4 ){
        fNearCity[ i ][ pos - 1 ] = ( i + 1 ) % Ncity;
        fEdgeDisOrder[ i ][ pos - 1 ] = Direct( i, fNearCity[ i ][ pos - 1 ] );
      }
    }
  }
}

void TEvaluator::DoIt( TIndi& indi )
{
  int d = 0;
  for( int i = 0; i < Ncity; ++i ){
    d += this->Direct( i, indi.fLink[i][0] );
    d += this->Direct( i, indi.fLink[i][1] );
  }
  indi.fEvaluationValue = d / 2;
}

int TEvaluator::Direct( int i, int j )
{
  return IsAdjacent( i, j ) ? 0 : 1;
}

void TEvaluator::TranceLinkOrder( TIndi& indi )
{
  for( int i = 0; i < Ncity; ++i ){
    indi.fOrder[ i ][ 0 ] = this->GetOrd( i, indi.fLink[ i ][ 0 ] );
    indi.fOrder[ i ][ 1 ] = this->GetOrd( i, indi.fLink[ i ][ 1 ] );
  }
}

int TEvaluator::GetOrd( int a, int b )
{
  for( int s = 0; s <= fNearNumMax; ++s )
    if( fNearCity[ a ][ s ] == b )
      return s;
  return -1;
}

void TEvaluator::WriteTo( FILE* fp, TIndi& indi )
{
  assert( Ncity == indi.fN );
  int* array = new int [ Ncity ];
  int curr, next, pre, st, count;

  count = 0;
  pre = -1;
  curr = 0;
  st = 0;
  while( 1 ){
    array[ count++ ] = curr + 1;
    if( count > Ncity ){
      printf( "Invalid\n" );
      delete [] array;
      return;
    }
    if( indi.fLink[ curr ][ 0 ] == pre )
      next = indi.fLink[ curr ][ 1 ];
    else
      next = indi.fLink[ curr ][ 0 ];
    pre = curr;
    curr = next;
    if( curr == st )
      break;
  }

  if( this->CheckValid( array, indi.fEvaluationValue ) == false )
    printf( "Individual is invalid\n" );

  fprintf( fp, "%d %d\n", indi.fN, indi.fEvaluationValue );
  for( int i = 0; i < indi.fN; ++i )
    fprintf( fp, "%d ", array[ i ] );
  fprintf( fp, "\n" );
  delete [] array;
}

bool TEvaluator::ReadFrom( FILE* fp, TIndi& indi )
{
  assert( Ncity == indi.fN );
  int* array = new int [ Ncity ];
  int curr, next, pre, st, count;
  int N, value;

  if( fscanf( fp, "%d %d", &N, &value ) == EOF ){
    delete [] array;
    return false;
  }
  assert( N == Ncity );
  indi.fN = N;
  indi.fEvaluationValue = value;

  for( int i = 0; i < Ncity; ++i ){
    if( fscanf( fp, "%d", &array[ i ] ) == EOF ){
      delete [] array;
      return false;
    }
  }

  if( this->CheckValid( array, indi.fEvaluationValue ) == false ){
    printf( "Individual is invalid\n" );
    delete [] array;
    return false;
  }

  for( int i = 0; i < Ncity; ++i )
    array[ i ] -= 1;

  indi.fLink[ array[ 0 ] ][ 0 ] = array[ Ncity - 1 ];
  indi.fLink[ array[ 0 ] ][ 1 ] = array[ 1 ];
  indi.fLink[ array[ Ncity - 1 ] ][ 0 ] = array[ Ncity - 2 ];
  indi.fLink[ array[ Ncity - 1 ] ][ 1 ] = array[ 0 ];
  for( int i = 1; i < Ncity - 1; ++i ){
    indi.fLink[ array[ i ] ][ 0 ] = array[ i - 1 ];
    indi.fLink[ array[ i ] ][ 1 ] = array[ i + 1 ];
  }
  delete [] array;
  return true;
}

bool TEvaluator::CheckValid( int* array, int value )
{
  int* check = new int [ Ncity ];
  for( int i = 0; i < Ncity; ++i )
    check[ i ] = 0;

  for( int i = 0; i < Ncity; ++i ){
    if( array[ i ] <= 0 || array[ i ] > Ncity ){
      delete [] check;
      return false;
    }
    ++check[ array[ i ] - 1 ];
  }
  for( int i = 0; i < Ncity; ++i ){
    if( check[ i ] != 1 ){
      delete [] check;
      return false;
    }
  }
  delete [] check;

  int distance = 0;
  for( int i = 0; i < Ncity - 1; ++i )
    distance += this->Direct( array[ i ] - 1, array[ i + 1 ] - 1 );
  distance += this->Direct( array[ Ncity - 1 ] - 1, array[ 0 ] - 1 );
  return distance == value;
}

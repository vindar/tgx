#ifndef __RAND__
#include "rand.h"
#endif

#if defined(_MSC_VER)
static unsigned long long tgx_rand48_state = 1;
static void seed48(unsigned short seed16v[3])
{
  tgx_rand48_state = ((unsigned long long)seed16v[2] << 32) |
                     ((unsigned long long)seed16v[1] << 16) |
                     (unsigned long long)seed16v[0];
}
static double drand48()
{
  tgx_rand48_state = (0x5DEECE66DULL * tgx_rand48_state + 0xBULL) & ((1ULL << 48) - 1);
  return (double)tgx_rand48_state / (double)(1ULL << 48);
}
#endif

TRandom* tRand = NULL;

void InitURandom()
{
  int seed;
  unsigned short seed16v[3];

  seed = 1111;     

  seed16v[0] = 100;
  seed16v[1] = 200;
  seed16v[2] = seed;

  seed48( seed16v );
  tRand = new TRandom();
  srand( seed );

  // srand((unsigned int)time(NULL));
}


void InitURandom( int dd )
{
  int seed;
  unsigned short seed16v[3];

  seed = dd;       

  seed16v[0] = 100;
  seed16v[1] = 200;
  seed16v[2] = seed;

  seed48( seed16v );
  tRand = new TRandom();
  srand( seed );
}


TRandom::TRandom()
{
}


TRandom::~TRandom()
{
}


int TRandom::Integer( int minNumber, int maxNumber )
{
  return minNumber + (int)(drand48() * (double)(maxNumber - minNumber + 1));
}


double TRandom::Double( double minNumber, double maxNumber )
{
  return minNumber + drand48() * (maxNumber - minNumber);
}


void TRandom::Permutation( int *array, int numOfElement, int numOfSample )
{
  int i,j,k,r;
  int *b;

  if( numOfElement <= 0 )
    return;

  b= new int[numOfElement];

  for(j=0;j<numOfElement;j++) b[j]=0;
  for(i=0;i<numOfSample;i++)
  {  
    r=rand()%(numOfElement-i);
    k=0;
    for(j=0;j<=r;j++)
    {
      while(b[k]==1)
      {
	k++;
      }
      k++;
    }
    array[i]=k-1;
    b[k-1]=1;
  }
  delete [] b;
}


double TRandom::NormalDistribution( double mu, double sigma )
{
  double U1,U2,X;
  double PI = 3.1415926;
  
  while( 1 ){
    U1 = this->Double( 0.0, 1.0 );
    if( U1 != 0.0 ) break;
  }
  U2 = this->Double( 0.0, 1.0 );

  X = sqrt(-2.0*log(U1)) * cos(2*PI*U2);
  return( mu + sigma*X );
}


void TRandom::Shuffle( int *array, int numOfElement )
{
  int *a;
  int *b;

  a = new int[numOfElement];
  b = new int[numOfElement];

  this->Permutation( b, numOfElement, numOfElement ); 

  for( int i = 0; i < numOfElement; ++i )
    a[ i ] = array[ i ]; 
  for( int i = 0; i < numOfElement; ++i )
    array[ i ] = a[ b[ i ] ]; 

  delete [] a;
  delete [] b;
}




#ifndef __INDI__
#define __INDI__


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



class TIndi {
public:
  TIndi();
  ~TIndi();
  void Define( int N );
  TIndi& operator = ( const TIndi& src );
  bool operator == (  const TIndi& indi2 );

  int fN;
  int** fLink;
  int** fOrder;         // Large
  int fEvaluationValue;
};


#endif


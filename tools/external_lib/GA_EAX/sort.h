#ifndef __SORT__
#define __SORT__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>


extern void InitSort( void );

class TSort {
 public:
  TSort();
  ~TSort();
  void Index( double* Arg, int numOfArg, int* indexOrderd, int numOfOrd );
  void Index( int* Arg, int numOfArg, int* indexOrderd, int numOfOrd );
  void Index_B( double* Arg, int numOfArg, int* indexOrderd, int numOfOrd );
  void Index_B( int* Arg, int numOfArg, int* indexOrderd, int numOfOrd );
  void Sort( int* Arg, int numOfArg );
};

extern TSort* tSort;


#endif



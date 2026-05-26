#ifndef __EVALUATOR__
#define __EVALUATOR__

#ifndef __INDI__
#include "indi.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

class TEvaluator {
 public:
  TEvaluator();
  ~TEvaluator();
  void SetInstance( char filename[] );
  void DoIt( TIndi& indi );
  int Direct( int i, int j );
  void TranceLinkOrder( TIndi& indi );
  int GetOrd( int a, int b );

  void WriteTo( FILE* fp, TIndi& indi );
  bool ReadFrom( FILE* fp, TIndi& indi );
  bool CheckValid( int* array, int value );

  int fNearNumMax;
  int **fNearCity;
  int **fEdgeDisOrder;
  int Ncity;
  double *x;
  double *y;

 private:
  std::vector<std::vector<int> > fAdj;
  bool IsAdjacent( int i, int j ) const;
};

#endif

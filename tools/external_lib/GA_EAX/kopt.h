#ifndef __KOPT__
#define __KOPT__

#ifndef __RAND__
#include "rand.h"
#endif

#ifndef __Sort__
#include "sort.h"
#endif

#ifndef __INDI__
#include "indi.h"
#endif

#ifndef __EVALUATOR__
#include "evaluator.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

class TKopt {
public:
  TKopt( int N );
  ~TKopt();
  void SetInvNearList();
  void TransIndiToTree( TIndi& indi );
  void TransTreeToIndi( TIndi& indi );

  void DoIt( TIndi& tIndi );             /* Apply a local search with the 2-opt neighborhood */
  void Sub();
  int GetNext( int t );
  int GetPrev( int t );
  void IncrementImp( int flagRev );
  void CombineSeg( int segL, int segS );

  void CheckDetail();
  void CheckValid();

  void Swap(int &a,int &b);
  int Turn( int &orient );

  void MakeRandSol( TIndi& indi );      /* Set a random tour */


  TEvaluator* eval;

private:
  int fN;

  int fFixNumOfSeg;
  int fNumOfSeg;   
  int **fLink;     
  int *fSegCity;   
  int *fOrdCity;   

  int *fOrdSeg;    
  int *fOrient;    
  int **fLinkSeg;  
  int *fSizeSeg;   
  int **fCitySeg;  

  int *fT;
  int fFlagRev;  
  int fTourLength;

  int *fActiveV;
  int **fInvNearList; 
  int *fNumOfINL;     
  
  int *fArray;
  int *fCheckN;
  int *fGene;
  int *fB;
};

#endif


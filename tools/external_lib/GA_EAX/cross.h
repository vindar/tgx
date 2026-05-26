#ifndef __CROSS__
#define __CROSS__

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


class TCross {
public:
  TCross( int N );
  ~TCross();
  void SetParents( const TIndi& tPa1, const TIndi& tPa2,     /* Set information of the parent tours */
		   int flagC[ 10 ], int numOfKids ); 
  void DoIt( TIndi& tKid, TIndi& tPa2, int numOfKids,        /* Main procedure of EAX */
	     int flagP, int flagC[ 10 ], int** fEdgeFreq ); 
  void SetABcycle( const TIndi& parent1, const TIndi& parent2, /* Step 2 of EAX */
		   int flagC[ 10 ], int numOfKids );
  void FormABcycle( const TIndi& tPa1, const TIndi& tPa2 );     /* Store an AB-cycle found */
  void Swap(int &a,int &b);                             /* Swap */ 
  void ChangeSol( TIndi& tKid, int ABnum, int type );   /* Apply an AB-cycle to an intermediate solution */
  void MakeCompleteSol( TIndi& tKid );                  /* Step 5 of EAX */
  void MakeUnit();                                      /* Step 5-1 of EAX */ 
  void BackToPa1( TIndi& tKid );                        /* Undo the parent p_A */
  void GoToBest( TIndi& tKid );                         /* Modify tKid to the best offspring solution */
  void IncrementEdgeFreq( int **fEdgeFreq );            /* Increment fEdgeFreq[][] */
  int Cal_ADP_Loss( int **fEdgeFreq );                  /* Compute the difference in the averate distance */
  double Cal_ENT_Loss( int **fEdgeFreq );               /* Compute the difference in the edge entropy */

  void SetWeight( const TIndi& parent1, const TIndi& parent2 ); /* Block2 */
  int Cal_C_Naive();                                            /* Block2 */
  void Search_Eset( int num );                                  /* Block2 */
  void Add_AB( int AB_num );                                    /* Block2 */
  void Delete_AB( int AB_num );                                 /* Block2 */

  void CheckValid( TIndi& indi );                               /* For debug */

  int fNumOfGeneratedCh;
  TEvaluator* eval;			 
  int fNumOfPop;
  
private:
  int fFlagImp;         
  int fN;
  TIndi tBestTmp;
  int r,exam;
  int exam_flag;
  int **near_data;
  int *koritsu, *bunki, *kori_inv, *bun_inv;
  int koritsu_many,bunki_many;
  int st,ci,pr,stock,st_appear;
  int *check_koritsu;
  int *fRoute;
  int flag_st,flag_circle,pr_type;
  int ch_dis;
  int *fABcycleL;         // Large
  int *fPosi_ABL;         // Large
  int fSPosi_ABL;         // Large
  int *fPermu;
  int fEvalType;
  int fEsetType;
  int fNumOfABcycleInESet;
  int fNumOfABcycle;
  int fPosiCurr;
  int fMaxNumOfABcycle;

  int *fC;
  int *fJun; 
  int *fOrd1, *fOrd2; 

  // Speed Up Start
  int *fOrder;    
  int *fInv;      
  int **fSegment; 
  int *fSegUnit;  
  		       
  int fNumOfUnit; 
  int fNumOfSeg;  
  int *fSegPosiList;
  int fNumOfSPL;    
  int *LinkAPosi;   
  int **LinkBPosi;  
  int *fPosiSeg;    
  int *fNumOfElementInUnit; 
  int *fCenterUnit;         
  int fNumOfElementInCU;    
  int *fListOfCenterUnit;   
  int fNumOfSegForCenter;   
  int *fSegForCenter;       

  int *fGainAB;             
  int fGainModi;            
  int fNumOfModiEdge;				 
  int fNumOfBestModiEdge;				 
  int **fModiEdge;				 
  int **fBestModiEdge;			
  int fNumOfAppliedCycle;
  int fNumOfBestAppliedCycle;
  int *fAppliedCylce;
  int *fBestAppliedCylce;
  // Speed Up End

  // Block2
  int *fNumOfElementINAB;    
  int **fInEffectNode; 
  int **fWeight_RR; 
  int *fWeight_SR;  
  int *fWeight_C;   
  int *fUsedAB, fNumOfUsedAB;
  int fNum_C, fNum_E;
  int fTmax, fMaxStag;
  int *fMoved_AB;
  int fNumOfABcycleInEset;
  int *fABcycleInEset;
  int fDis_AB;     
  int fBest_Num_C, fBest_Num_E;

  int **fABcycleLOrd;           // Large
  int ***fModiEdgeOrd;          // Large
  int ***fBestModiEdgeOrd;      // Large
};


#endif

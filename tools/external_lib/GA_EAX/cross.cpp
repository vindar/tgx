#ifndef __Cross__
#include "cross.h"
#endif


TCross::TCross( int N )
{
  fMaxNumOfABcycle = 6000; /* Set an appropriate value (6000 usually is enough) */

  fN = N;
  tBestTmp.Define( fN );

  near_data = new int* [ fN ];
  for ( int j = 0; j < fN; ++j ) 
    near_data[j] = new int [ 5 ];

  /*
  fABcycle = new int* [ fMaxNumOfABcycle ];
  for ( int j = 0; j < fMaxNumOfABcycle; ++j ) 
    fABcycle[j] = new int [ fN + 4 ];
  */
  fABcycleL = new int [ fN * 3 ];
  fPosi_ABL = new int [ fN * 3 ];

  koritsu = new int [ fN ];
  bunki = new int [ fN ];
  kori_inv = new int [ fN ];
  bun_inv = new int [ fN ];
  check_koritsu = new int [ fN ];
  fRoute = new int [ 2*fN + 1 ];
  fPermu = new int [ fMaxNumOfABcycle ];

  fC = new int [ 2*fN+4 ];
  fJun = new int[ fN+ 1 ];
  fOrd1 = new int [ fN ];
  fOrd2 = new int [ fN ];

  // Speed Up Start
  fOrder = new int [ fN ];
  fInv = new int [ fN ];
  fSegment = new int* [ fN ];
  for ( int j = 0; j < fN; ++j ) 
    fSegment[ j ] = new int [ 2 ];
  fSegUnit = new int [ fN ]; 
  fSegPosiList = new int[ fN ];
  LinkAPosi = new int [ fN ];
  LinkBPosi = new int* [ fN ];
  for ( int j = 0; j < fN; ++j ) 
    LinkBPosi[ j ] = new int [ 2 ];
  fPosiSeg = new int [ fN ];
  fNumOfElementInUnit = new int [ fN ]; 
  fCenterUnit = new int [ fN ]; 
  for ( int j = 0; j < fN; ++j ) 
    fCenterUnit[ j ] = 0;
  fListOfCenterUnit = new int [ fN+2 ]; 
  fSegForCenter = new int [ fN ]; 
  fGainAB = new int [ fN ]; 
  fModiEdge = new int* [ fN ]; 				 
  for ( int j = 0; j < fN; ++j ) 
    fModiEdge[ j ] = new int [ 4 ]; 				 
  fBestModiEdge = new int* [ fN ]; 				 
  for ( int j = 0; j < fN; ++j ) 
    fBestModiEdge[ j ] = new int [ 4 ]; 				 
  fAppliedCylce = new int [ fN ];
  fBestAppliedCylce = new int [ fN ];
  // Speed Up End

  // Block2
  fNumOfElementINAB = new int [ fMaxNumOfABcycle ];
  fInEffectNode = new int* [ fN ];
  for( int i = 0; i < fN; ++i )
    fInEffectNode[ i ] = new int [ 2 ];
  fWeight_RR = new int* [ fMaxNumOfABcycle ];
  for( int i = 0; i < fMaxNumOfABcycle; ++i )
    fWeight_RR[ i ] = new int [ fMaxNumOfABcycle ];
  fWeight_SR = new int [ fMaxNumOfABcycle ];
  fWeight_C = new int [ fMaxNumOfABcycle ];
  fUsedAB = new int [ fN ];
  fMoved_AB = new int [ fN ];
  fABcycleInEset = new int [ fMaxNumOfABcycle ];

  /*
  fABcycleOrd = new int** [ fMaxNumOfABcycle ];          // Large
  for ( int j = 0; j < fMaxNumOfABcycle; ++j ){ 
    fABcycleOrd[ j ] = new int* [ fN + 4 ];
    for ( int j2 = 0; j2 < fN+4; ++j2 ) 
      fABcycleOrd[j][j2] = new int [ 2 ];
  }
  */
  fABcycleLOrd = new int* [ fN * 3 ];          // Large
  for ( int j = 0; j < fN * 3; ++j ){ 
    fABcycleLOrd[ j ] = new int [ 2 ];
  }

  fModiEdgeOrd = new int** [ fN ];          // Large
  for ( int j = 0; j < fN; ++j ){ 
    fModiEdgeOrd[ j ] = new int* [ 4 ];
    for ( int j2 = 0; j2 < 4; ++j2 ) 
      fModiEdgeOrd[j][j2] = new int [ 2 ];
  }

  fBestModiEdgeOrd = new int** [ fN ];          // Large
  for ( int j = 0; j < fN; ++j ){ 
    fBestModiEdgeOrd[ j ] = new int* [ 4 ];
    for ( int j2 = 0; j2 < 4; ++j2 ) 
      fBestModiEdgeOrd[j][j2] = new int [ 2 ];
  }
}

TCross::~TCross()
{
  delete [] koritsu;
  delete [] bunki;
  delete [] kori_inv;
  delete [] bun_inv;
  delete [] check_koritsu;
  delete [] fRoute;
  delete [] fPermu;

  for ( int j = 0; j < fN; ++j ) 
    delete[] near_data[ j ];
  delete[] near_data;

  delete[] fABcycleL;
  delete[] fPosi_ABL;

  delete [] fC;
  delete [] fJun; 
  delete [] fOrd1; 
  delete [] fOrd2; 


  // Speed Up Start
  delete [] fOrder;
  delete [] fInv;

  for ( int j = 0; j < fN; ++j ) 
    delete[] fSegment[ j ];
  delete[] fSegment;
  delete[] fSegUnit;
  delete [] fSegPosiList;
  delete [] LinkAPosi;
  for ( int j = 0; j < fN; ++j ) 
    delete[] LinkBPosi[ j ];
  delete [] LinkBPosi;
  delete [] fPosiSeg;
  delete [] fNumOfElementInUnit; 
  delete [] fCenterUnit;
  delete [] fListOfCenterUnit;
  delete [] fSegForCenter;
  delete [] fGainAB;

  for ( int j = 0; j < fN; ++j ) 
    delete[] fModiEdge[ j ];
  delete [] fModiEdge;
  for ( int j = 0; j < fN; ++j ) 
    delete[] fBestModiEdge[ j ];
  delete [] fBestModiEdge;
  delete [] fAppliedCylce;
  delete [] fBestAppliedCylce;
  // Speed Up End

  // Block2
  delete [] fNumOfElementINAB;
  for ( int j = 0; j < fN; ++j ) 
    delete [] fInEffectNode[ j ];
  delete [] fInEffectNode;
  for( int i = 0; i < fMaxNumOfABcycle; ++i )
    delete [] fWeight_RR[ i ];
  delete [] fWeight_RR;
  delete [] fWeight_SR;
  delete [] fWeight_C;
  delete [] fUsedAB;
  delete [] fMoved_AB;
  delete [] fABcycleInEset;


  for ( int j = 0; j < fN * 3 ; ++j )  // Large
    delete[] fABcycleLOrd[ j ];
  delete[] fABcycleLOrd;

  for ( int j = 0; j < fN; ++j ){   // Large
    for ( int j2 = 0; j2 < 4; ++j2 ){ 
      delete[] fModiEdgeOrd[ j ][ j2 ];
    }
    delete [] fModiEdgeOrd[ j ];
  }
  delete[] fModiEdgeOrd;

  for ( int j = 0; j < fN; ++j ){   // Large
    for ( int j2 = 0; j2 < 4; ++j2 ){ 
      delete[] fBestModiEdgeOrd[ j ][ j2 ];
    }
    delete [] fBestModiEdgeOrd[ j ];
  }
  delete[] fBestModiEdgeOrd;
}


 void TCross::SetParents( const TIndi& tPa1, const TIndi& tPa2, int flagC[ 10 ], int numOfKids )
{
  this->SetABcycle( tPa1, tPa2, flagC, numOfKids ); 
  
  fDis_AB = 0;   

  int curr, next, st, pre;
  st = 0;
  curr=-1;
  next = st;
  for( int i = 0; i < fN; ++i )
  {
    pre=curr;
    curr=next;
    if( tPa1.fLink[curr][0] != pre ) 
      next = tPa1.fLink[ curr ][ 0 ];
    else 
      next=tPa1.fLink[curr][1];

    if( tPa2.fLink[ curr ][ 0 ] != next && tPa2.fLink[ curr ][ 1 ] != next ) 
      ++fDis_AB; 
    
    fOrder[ i ] = curr;
    fInv[ curr ] = i;
  }

  assert( next == st );

  if( flagC[ 1 ] == 2 ){           /* Block2 */
    fTmax = 10;                    /* Block2 */
    fMaxStag = 20;                 /* Block2 (1:Greedy LS, 20:Tabu Search) */
    this->SetWeight( tPa1, tPa2 ); /* Block2 */
  }
}


void TCross::DoIt( TIndi& tKid, TIndi& tPa2, int numOfKids, int flagP, int flagC[ 10 ], int **fEdgeFreq )
{
  int Num;    
  int jnum, centerAB; 
  int gain;
  int BestGain;  
  double pointMax, point;
  double DLoss;

  fEvalType = flagC[ 0 ];              /* 1:Greedy, 2:---, 3:---, 4:Entropy */
  fEsetType = flagC[ 1 ];              /* 1:Single-AB, 2:Block2 */

  if( fEvalType == 3 ){
    printf( "Distance preserving is not supported \n" );
    exit( 1 );
  }
  assert( fEvalType == 1 || fEvalType == 4 );
  assert( fEsetType == 1 || fEsetType == 2 );

  if ( numOfKids <= fNumOfABcycle ) 
    Num = numOfKids;
  else 
    Num = fNumOfABcycle;

  if( fEsetType == 1 ){         /* Single-AB */
    tRand->Permutation( fPermu, fNumOfABcycle, fNumOfABcycle ); 
  }
  else if( fEsetType == 2 ){    /* Block2 */
    for( int k =0; k< fNumOfABcycle; ++k )
      fNumOfElementINAB[ k ] = fABcycleL[ fPosi_ABL[k] + 0 ]; // fABcycle[ k ][ 0 ];
    tSort->Index_B( fNumOfElementINAB, fNumOfABcycle, fPermu, fNumOfABcycle );
  }


  fNumOfGeneratedCh = 0;
  pointMax = 0.0;
  BestGain = 0;       
  fFlagImp = 0;  

  for( int j =0; j < Num; ++j )
  { 
    fNumOfABcycleInEset = 0;
    if( fEsetType == 1 ){         /* Single-AB */
      jnum = fPermu[ j ];
      fABcycleInEset[ fNumOfABcycleInEset++ ] = jnum; 
    }
    else if( fEsetType == 2 ){    /* Block2 */
      jnum = fPermu[ j ];
      centerAB = jnum;
      for( int s = 0; s < fNumOfABcycle; ++s ){ 
	if( s == centerAB )
	  fABcycleInEset[ fNumOfABcycleInEset++ ] = s; 
	else{
	  // if( fWeight_RR[ centerAB ][ s ] > 0 && fABcycle[ s ][ 0 ] < fABcycle[ centerAB ][ 0 ] ){
	  if( fWeight_RR[ centerAB ][ s ] > 0 && 
	      fABcycleL[ fPosi_ABL[s] + 0 ] < fABcycleL[ fPosi_ABL[centerAB] + 0 ] ){
	    if( rand() %2 == 0 )
	      fABcycleInEset[ fNumOfABcycleInEset++ ] = s; 
	  }
	}
      }
      this->Search_Eset( centerAB );    
    }


    fNumOfSPL = 0;          
    gain = 0;               
    fNumOfAppliedCycle = 0; 
    fNumOfModiEdge = 0;	    

    fNumOfAppliedCycle = fNumOfABcycleInEset; 
    for( int k = 0; k < fNumOfAppliedCycle; ++k ){   
      fAppliedCylce[ k ] = fABcycleInEset[ k ]; 
      jnum = fAppliedCylce[ k ];
      this->ChangeSol( tKid, jnum, flagP );
      gain += fGainAB[ jnum ];                       
    }

    this->MakeUnit();                                   
    this->MakeCompleteSol( tKid );                      
    gain += fGainModi;                                  
    
    ++fNumOfGeneratedCh;

    if( fEvalType == 1 )       /* Greedy */
      DLoss = 1.0;
    else if( fEvalType == 2 ){  
      assert( 1 == 2 );
    }
    else if( fEvalType == 3 ){  
      assert( 1 == 2 );
    }
    else if( fEvalType == 4 )  /* Entropy preservation */
      DLoss = this->Cal_ENT_Loss( fEdgeFreq );             

    if( DLoss <= 0.0 ) DLoss = 0.00000001; 

    point = (double)gain / DLoss;                         
    tKid.fEvaluationValue = tKid.fEvaluationValue - gain; 

    // if( pointMax < point ){
    if( pointMax < point && (2 * fBest_Num_E < fDis_AB || 
			     tKid.fEvaluationValue != tPa2.fEvaluationValue ) ){   
      pointMax = point;
      BestGain = gain;        
      fFlagImp = 1;  

      fNumOfBestAppliedCycle = fNumOfAppliedCycle;
      for( int s = 0; s < fNumOfBestAppliedCycle; ++s )
	fBestAppliedCylce[ s ] = fAppliedCylce[ s ];
      
      fNumOfBestModiEdge = fNumOfModiEdge;	  	
      for( int s = 0; s < fNumOfBestModiEdge; ++s ){
	for( int m = 0; m < 4; ++m ){
	  fBestModiEdge[ s ][ m ] = fModiEdge[ s ][ m ];
	  fBestModiEdgeOrd[ s ][ m ][ 0 ] = fModiEdgeOrd[ s ][ m ][ 0 ]; // Large
	  fBestModiEdgeOrd[ s ][ m ][ 1 ] = fModiEdgeOrd[ s ][ m ][ 1 ]; // Large
	}
      }	
    }

    this->BackToPa1( tKid ); 
    tKid.fEvaluationValue = tKid.fEvaluationValue + gain;
  }

  if( fFlagImp == 1 ){      
    this->GoToBest( tKid ); 
    tKid.fEvaluationValue = tKid.fEvaluationValue - BestGain;
    this->IncrementEdgeFreq( fEdgeFreq );
  }
}


void TCross::SetABcycle( const TIndi& tPa1, const TIndi& tPa2, int flagC[ 10 ], int numOfKids )
{
  fSPosi_ABL = 0;             // Large

  bunki_many=0; koritsu_many=0;
  for( int j = 0; j < fN ; ++j )
  {
    near_data[j][1]=tPa1.fLink[j][0];
    near_data[j][3]=tPa1.fLink[j][1];

    near_data[j][0] = 2;
    
    koritsu[koritsu_many]=j;
    koritsu_many++;

    near_data[j][2]=tPa2.fLink[j][0];
    near_data[j][4]=tPa2.fLink[j][1];
  }
  for(int j = 0; j < fN; ++j ) 
  {
    check_koritsu[j]=-1;
    kori_inv[koritsu[j]]=j;
  }

  /**************************************************/

  fNumOfABcycle=0; 
  flag_st=1;                   
  while(koritsu_many!=0)
  {                                                               
    if(flag_st==1)
    {
      fPosiCurr=0;
      r=rand()%koritsu_many;
      st=koritsu[r];    
      check_koritsu
[st]=fPosiCurr;
      fRoute[fPosiCurr]=st;
      ci=st;
      pr_type=2;
    }
    else if(flag_st==0)
    {
      ci=fRoute[fPosiCurr];   
    }
                        
    flag_circle=0;
    while(flag_circle==0)
    {
      fPosiCurr++;
      pr=ci;
      
      switch(pr_type)
      {
      case 1:          
	ci=near_data[pr][fPosiCurr%2+1];
	break;
      case 2:    
	r=rand()%2;
	ci=near_data[pr][fPosiCurr%2+1+2*r];
	if(r==0) this->Swap(near_data[pr][fPosiCurr%2+1],near_data[pr][fPosiCurr%2+3]);
	break;
      case 3:    
	ci=near_data[pr][fPosiCurr%2+3];
      }

      fRoute[fPosiCurr]=ci;
      
      if(near_data[ci][0]==2)  
      {   
	if(ci==st)             
	{        
	  if(check_koritsu[st]==0) 
	  {        
	    if((fPosiCurr-check_koritsu[st])%2==0)  
	    {                  
	      if(near_data[st][fPosiCurr%2+1]==pr)
	      {
		this->Swap(near_data[ci][fPosiCurr%2+1],near_data[ci][fPosiCurr%2+3]); 
	      }
	      st_appear = 1;
	      this->FormABcycle( tPa1, tPa2 );
	      if( flagC[ 1 ] == 1 && fNumOfABcycle == numOfKids ) goto LLL;
	      if( fNumOfABcycle == fMaxNumOfABcycle ) goto LLL;

	      flag_st=0;
	      flag_circle=1;
	      pr_type=1; 
	    }
	    else
	    {
	      this->Swap(near_data[ci][fPosiCurr%2+1],near_data[ci][fPosiCurr%2+3]); 
	      pr_type=2;
	    }
	    check_koritsu[st]=fPosiCurr;
	  } 
	  else
	  {         
	    st_appear = 2;
	    this->FormABcycle( tPa1, tPa2 );
	    if( flagC[ 1 ] == 1 && fNumOfABcycle == numOfKids ) goto LLL;
	    if( fNumOfABcycle == fMaxNumOfABcycle ) goto LLL;

	    flag_st=1;
	    flag_circle=1;
	  }
	}
	else if(check_koritsu[ci]==-1)
	{
	  check_koritsu[ci]=fPosiCurr;
	  if(near_data[ci][fPosiCurr%2+1]==pr)
	  {
	    this->Swap(near_data[ci][fPosiCurr%2+1],near_data[ci][fPosiCurr%2+3]); 
	  }
	  pr_type=2;
	}
	else if(check_koritsu[ci]>0)  
	{
	  this->Swap(near_data[ci][fPosiCurr%2+1],near_data[ci][fPosiCurr%2+3]); 
	  if((fPosiCurr-check_koritsu[ci])%2==0)  
	  {
	    st_appear = 1;
	    this->FormABcycle( tPa1, tPa2 );
	    if( flagC[ 1 ] == 1 && fNumOfABcycle == numOfKids ) goto LLL;
	    if( fNumOfABcycle == fMaxNumOfABcycle ) goto LLL;

	    flag_st=0;
	    flag_circle=1;
	    pr_type=1;
	  }
	  else
	  {
	    this->Swap(near_data[ci][(fPosiCurr+1)%2+1],near_data[ci][(fPosiCurr+1)%2+3]); 
	    pr_type=3;
	  }  
	}
      }
      else if(near_data[ci][0]==1)    
      {
	if(ci==st)                    
        {
	  st_appear = 1;
	  this->FormABcycle( tPa1, tPa2 );
	  if( flagC[ 1 ] == 1 && fNumOfABcycle == numOfKids ) goto LLL;
	  if( fNumOfABcycle == fMaxNumOfABcycle ) goto LLL;

	  flag_st=1;
	  flag_circle=1;
	}
	else pr_type=1;
      }
    }
  }
                                       
  while(bunki_many!=0)
  {            
    fPosiCurr=0;   
    r=rand()%bunki_many;
    st=bunki[r];
    fRoute[fPosiCurr]=st;
    ci=st;
    
    flag_circle=0;
    while(flag_circle==0)
    { 
      pr=ci; 
      fPosiCurr++;
      ci=near_data[pr][fPosiCurr%2+1]; 
      fRoute[fPosiCurr]=ci;
      if(ci==st)                       
      {
	st_appear = 1;
	this->FormABcycle( tPa1, tPa2 );
	if( flagC[ 1 ] == 1 && fNumOfABcycle == numOfKids ) goto LLL;
	if( fNumOfABcycle == fMaxNumOfABcycle ) goto LLL;

	flag_circle=1;
      }
    }
  }

LLL: ;

  if( fNumOfABcycle == fMaxNumOfABcycle ){
    printf( "fMaxNumOfABcycle(%d) must be increased\n", fMaxNumOfABcycle );
    exit( 1 );
  }
  
}


void TCross::FormABcycle( const TIndi& tPa1, const TIndi& tPa2 )
{
  int j;
  int st_count;
  int edge_type;
  int st,ci, stock;
  int cem;                     
  int diff;
  int p, c, n, k;
 
  if(fPosiCurr%2==0) edge_type=1;  
  else edge_type=2;                
  st=fRoute[fPosiCurr];
  cem=0;
  fC[cem]=st;    

  st_count=0;
  while(1)
  {
    cem++;
    fPosiCurr--;
    ci=fRoute[fPosiCurr];
    if(near_data[ci][0]==2)
    {
      koritsu[kori_inv[ci]]=koritsu[koritsu_many-1];
      kori_inv[koritsu[koritsu_many-1]]=kori_inv[ci];
      koritsu_many--;
      bunki[bunki_many]=ci;
      bun_inv[ci]=bunki_many;
      bunki_many++;
    }
    else if(near_data[ci][0]==1)
    {
      bunki[bun_inv[ci]]=bunki[bunki_many-1];
      bun_inv[bunki[bunki_many-1]]=bun_inv[ci];
      bunki_many--;
    }
             
    near_data[ci][0]--;
    if(ci==st) st_count++;
    if(st_count==st_appear) break;
    fC[cem]=ci;  
  }

  if(cem==2) 
    return;

  
  fPosi_ABL[ fNumOfABcycle ] = fSPosi_ABL; // Large
  fABcycleL[ fSPosi_ABL + 0 ] = cem;       // fABcycle[fNumOfABcycle][0]=cem;     

  if(edge_type==2)
  {
    stock=fC[0];
    for( int j=0;j<cem-1;j++) fC[j]=fC[j+1];
    fC[cem-1]=stock;
  }

  for( int j=0;j<cem;j++) 
    fABcycleL[ fSPosi_ABL + j+2 ] = fC[j]; // fABcycle[fNumOfABcycle][j+2]=fC[j];
    
  fABcycleL[ fSPosi_ABL + 1 ] = fC[cem-1]; // fABcycle[fNumOfABcycle][1]=fC[cem-1];
  fABcycleL[ fSPosi_ABL + cem+2 ] = fC[0]; // fABcycle[fNumOfABcycle][cem+2]=fC[0];
  fABcycleL[ fSPosi_ABL + cem+3 ] = fC[1]; // fABcycle[fNumOfABcycle][cem+3]=fC[1];
  

  for( int j = 2; j <= cem+2; ++j )       // Large Large Large
  {
    p = fABcycleL[ fSPosi_ABL + j-1 ]; //  fABcycle[fNumOfABcycle][j-1]; 
    c = fABcycleL[ fSPosi_ABL + j ];   // fABcycle[fNumOfABcycle][j];
    n = fABcycleL[ fSPosi_ABL + j+1 ]; // fABcycle[fNumOfABcycle][j+1];
    
    if( j % 2 == 0 ){
      if( tPa2.fLink[ c ][ 0 ] == p )
	// fABcycleOrd[fNumOfABcycle][ j ][ 0 ] = tPa2.fOrder[ c ][ 0 ]; 
	fABcycleLOrd[ fSPosi_ABL + j ][ 0 ] = tPa2.fOrder[ c ][ 0 ]; 
      else 
	// fABcycleOrd[fNumOfABcycle][ j ][ 0 ] = tPa2.fOrder[ c ][ 1 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 0 ] = tPa2.fOrder[ c ][ 1 ]; 

      if( tPa1.fLink[ c ][ 0 ] == n )
	// fABcycleOrd[fNumOfABcycle][ j ][ 1 ] = tPa1.fOrder[ c ][ 0 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 1 ] = tPa1.fOrder[ c ][ 0 ]; 
      else 
	// fABcycleOrd[fNumOfABcycle][ j ][ 1 ] = tPa1.fOrder[ c ][ 1 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 1 ] = tPa1.fOrder[ c ][ 1 ]; 
    }

    if( j % 2 == 1 ){
      if( tPa1.fLink[ c ][ 0 ] == p )
	// fABcycleOrd[fNumOfABcycle][ j ][ 0 ] = tPa1.fOrder[ c ][ 0 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 0 ] = tPa1.fOrder[ c ][ 0 ]; 
      else 
	// fABcycleOrd[fNumOfABcycle][ j ][ 0 ] = tPa1.fOrder[ c ][ 1 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 0 ] = tPa1.fOrder[ c ][ 1 ]; 

      if( tPa2.fLink[ c ][ 0 ] == n )
	// fABcycleOrd[fNumOfABcycle][ j ][ 1 ] = tPa2.fOrder[ c ][ 0 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 1 ] = tPa2.fOrder[ c ][ 0 ]; 
      else 
	// fABcycleOrd[fNumOfABcycle][ j ][ 1 ] = tPa2.fOrder[ c ][ 1 ];
	fABcycleLOrd[ fSPosi_ABL + j ][ 1 ] = tPa2.fOrder[ c ][ 1 ]; 
    }
  }
  
  fC[ cem ] = fC[ 0 ]; 
  fC[ cem+1 ] = fC[ 1 ]; 
  diff = 0;
  for( j = 0; j < cem/2; ++j ) 
  {
    c = fABcycleL[ fSPosi_ABL + 2*j+2 ]; // fABcycle[fNumOfABcycle][ 2*j+2 ];     // Large Large Large
    k = fABcycleLOrd[ fSPosi_ABL + 2*j+2 ][ 1 ]; // fABcycleOrd[fNumOfABcycle][ 2*j+2 ][ 1 ];
    if( k != -1 )
      diff += eval->fEdgeDisOrder[ c ][ k ];
    else 
      diff += eval->Direct( fC[2*j], fC[1+2*j] );

    c = fABcycleL[ fSPosi_ABL + 2*j+3 ]; // fABcycle[fNumOfABcycle][ 2*j+3 ];
    k = fABcycleLOrd[ fSPosi_ABL + 2*j+3 ][ 1 ]; // fABcycleOrd[fNumOfABcycle][ 2*j+3 ][ 1 ];
    if( k != -1 )
      diff -= eval->fEdgeDisOrder[ c ][ k ];
    else 
      diff -= eval->Direct( fC[2*j+1], fC[2*j+2] ); 
  }
  fGainAB[fNumOfABcycle] = diff;
  ++fNumOfABcycle;
  fSPosi_ABL += (cem + 4);               // Large
}


void TCross::Swap(int &a,int &b)
{
  int s;
  s=a;
  a=b;
  b=s;
}


void TCross::ChangeSol( TIndi& tKid, int ABnum, int type )
{
  int j;
  int cem,r1,r2,b1,b2;
  int po_r1, po_r2, po_b1, po_b2; 
  int posi;

  posi = fPosi_ABL[ABnum];  

  cem = fABcycleL[ posi + 0 ];   // fABcycle[ABnum][0];  
  fC[0] = fABcycleL[ posi + 0 ]; // fABcycle[ABnum][0];

  if(type==2) 
  {
    for(j=0;j<cem+3;j++) fC[cem+3-j] = fABcycleL[ posi + j+1 ]; // fABcycle[ABnum][j+1];
  }
  else for(j=1;j<=cem+3;j++) fC[j] = fABcycleL[ posi + j ];// fABcycle[ABnum][j];

  if( type == 1 )
  {
    for(j=0;j<cem/2;j++)
    {                           
      r1 = fABcycleL[ posi + 2+2*j ];// fABcycle[ABnum][2+2*j];
      r2 = fABcycleL[ posi + 3+2*j ];// fABcycle[ABnum][3+2*j];
      b1 = fABcycleL[ posi + 1+2*j ];// fABcycle[ABnum][1+2*j];
      b2 = fABcycleL[ posi + 4+2*j ];// fABcycle[ABnum][4+2*j];

      if( tKid.fLink[r1][0] == r2 ){
	tKid.fLink[r1][0] = b1;
	tKid.fOrder[r1][0] = fABcycleLOrd[ posi + 2+2*j ][ 0 ]; // fABcycleOrd[ABnum][2+2*j][0]; // Large
      }
      else{ 
	tKid.fLink[r1][1]=b1;
	tKid.fOrder[r1][1] = fABcycleLOrd[ posi + 2+2*j ][ 0 ]; // fABcycleOrd[ABnum][2+2*j][0];  // Large
      }
      if( tKid.fLink[r2][0] == r1 ){ 
	tKid.fLink[r2][0] = b2;
	tKid.fOrder[r2][0] = fABcycleLOrd[ posi + 3+2*j ][ 1 ]; // fABcycleOrd[ABnum][3+2*j][1];  // Large
      }
      else{
	tKid.fLink[r2][1] = b2;   
	tKid.fOrder[r2][1] = fABcycleLOrd[ posi + 3+2*j ][ 1 ]; // fABcycleOrd[ABnum][3+2*j][1];  // Large
      }

      po_r1 = fInv[ r1 ]; 
      po_r2 = fInv[ r2 ]; 
      po_b1 = fInv[ b1 ]; 
      po_b2 = fInv[ b2 ]; 
    
      if( po_r1 == 0 && po_r2 == fN-1 )
	fSegPosiList[ fNumOfSPL++ ] = po_r1;
      else if( po_r1 == fN-1 && po_r2 == 0 )
	fSegPosiList[ fNumOfSPL++ ] = po_r2;
      else if( po_r1 < po_r2 )
	fSegPosiList[ fNumOfSPL++ ] = po_r2;
      else if( po_r2 < po_r1 )
	fSegPosiList[ fNumOfSPL++ ] = po_r1;
      else
	assert( 1 == 2 );
      
      LinkBPosi[ po_r1 ][ 1 ] = LinkBPosi[ po_r1 ][ 0 ];
      LinkBPosi[ po_r2 ][ 1 ] = LinkBPosi[ po_r2 ][ 0 ];
      LinkBPosi[ po_r1 ][ 0 ] = po_b1; 
      LinkBPosi[ po_r2 ][ 0 ] = po_b2; 
    }
  }
  else if( type == 2 )
  {   
    for(j=0;j<cem/2;j++)
    {                           
      r1 = fABcycleL[ posi + cem+4-(2+2*j) ]; // fABcycle[ABnum][cem+4-(2+2*j)];
      r2 = fABcycleL[ posi + cem+4-(3+2*j) ]; // fABcycle[ABnum][cem+4-(3+2*j)];
      b1 = fABcycleL[ posi + cem+4-(1+2*j) ]; // fABcycle[ABnum][cem+4-(1+2*j)];
      b2 = fABcycleL[ posi + cem+4-(4+2*j) ]; // fABcycle[ABnum][cem+4-(4+2*j)];

      if( tKid.fLink[r1][0] == r2 ){
	tKid.fLink[r1][0] = b1;
	tKid.fOrder[r1][0] = fABcycleLOrd[posi + cem+4-(2+2*j)][1]; // fABcycleOrd[ABnum][cem+4-(2+2*j)][1]; // Large
      }
      else{ 
	tKid.fLink[r1][1] = b1;
	tKid.fOrder[r1][1] = fABcycleLOrd[posi + cem+4-(2+2*j)][1]; // fABcycleOrd[ABnum][cem+4-(2+2*j)][1]; // Large
      }
      if( tKid.fLink[r2][0] == r1 ){ 
	tKid.fLink[r2][0] = b2;
	tKid.fOrder[r2][0] = fABcycleLOrd[posi + cem+4-(3+2*j)][0]; // fABcycleOrd[ABnum][cem+4-(3+2*j)][0]; // Large
      }
      else{
	tKid.fLink[r2][1] = b2;   
	tKid.fOrder[r2][1] = fABcycleLOrd[posi + cem+4-(3+2*j)][0]; // fABcycleOrd[ABnum][cem+4-(3+2*j)][0]; // Large
      }

      po_r1 = fInv[ r1 ];
      po_r2 = fInv[ r2 ];
      po_b1 = fInv[ b1 ];
      po_b2 = fInv[ b2 ];
    
      if( po_r1 == 0 && po_r2 == fN-1 )
	fSegPosiList[ fNumOfSPL++ ] = po_r1;
      else if( po_r1 == fN-1 && po_r2 == 0 )
	fSegPosiList[ fNumOfSPL++ ] = po_r2;
      else if( po_r1 < po_r2 )
	fSegPosiList[ fNumOfSPL++ ] = po_r2;
      else if( po_r2 < po_r1 )
	fSegPosiList[ fNumOfSPL++ ] = po_r1;
      else
	assert( 1 == 2 );
      
      LinkBPosi[ po_r1 ][ 1 ] = LinkBPosi[ po_r1 ][ 0 ];
      LinkBPosi[ po_r2 ][ 1 ] = LinkBPosi[ po_r2 ][ 0 ];
      LinkBPosi[ po_r1 ][ 0 ] = po_b1; 
      LinkBPosi[ po_r2 ][ 0 ] = po_b2; 
    }
  }
}


void TCross::MakeCompleteSol( TIndi& tKid )
{
  int j,j1,j2,j3;
  int st,ci,pre,curr,next,a,b,c,d,aa,bb,a1,b1;
  int city_many;
  int remain_unit_many;
  int ucm;
  int unit_num;
  int min_unit_city; 
  int near_num;
  int unit_many; 
  int center_un; 
  int select_un; 
  int diff,max_diff;
  int count;     
  int nearMax;
  int dis_ab[2], dis_ac, dis_cd, dis_bd, b_0, b_1;
  int k_0, k_1, k_cd, k_bd;

  fGainModi = 0;         

  while( fNumOfUnit != 1 )
  {    
    min_unit_city = fN + 12345;
    for( int u = 0; u < fNumOfUnit; ++u ) 
    {
      if( fNumOfElementInUnit[ u ] < min_unit_city )
      {
	center_un = u;
        min_unit_city = fNumOfElementInUnit[ u ];
      }
    }  

    st = -1;
    fNumOfSegForCenter = 0;   
    for( int s = 0; s < fNumOfSeg; ++s ){
      if( fSegUnit[ s ] == center_un ){
	int posi = fSegment[ s ][ 0 ];
	st = fOrder[ posi ];    
	fSegForCenter[  fNumOfSegForCenter++ ] = s; 
      }
    } 
    assert( st != -1 );

    curr = -1;
    next = st;
    fNumOfElementInCU = 0;
    while(1){ 
      pre = curr;
      curr = next;
      fCenterUnit[ curr ] = 1;     
      fListOfCenterUnit[ fNumOfElementInCU ] = curr;
      ++fNumOfElementInCU;

      if( tKid.fLink[ curr ][ 0 ] != pre )
	next = tKid.fLink[ curr ][ 0 ];
      else 
	next = tKid.fLink[ curr ][ 1 ]; 

      if( next == st ) break;
    }       
    fListOfCenterUnit[ fNumOfElementInCU ] = fListOfCenterUnit[ 0 ];
    fListOfCenterUnit[ fNumOfElementInCU+1 ] = fListOfCenterUnit[ 1 ];

    assert( fNumOfElementInCU == fNumOfElementInUnit[ center_un ] );

    max_diff = -999999999;
    a1 = -1; b1 = -1;
    nearMax = 10; /* N_near (see Step 5.3 in Section 2.2 of the Online Supplement) */
    /* nearMax must be smaller than or equal to eva->fNearNumMax (kopt.cpp ) */

  RESTART:;
    for( int s = 1; s <= fNumOfElementInCU; ++s )  
    { 
      a = fListOfCenterUnit[ s ];

      b_0 = fListOfCenterUnit[ s-1 ];         // Large 
      b_1 = fListOfCenterUnit[ s+1 ];         // Large 
      if( tKid.fLink[ a ][ 0 ] == b_0 ){     
	k_0 = tKid.fOrder[ a ][ 0 ];
	k_1 = tKid.fOrder[ a ][ 1 ];
      }
      else{
	k_0 = tKid.fOrder[ a ][ 1 ];
	k_1 = tKid.fOrder[ a ][ 0 ];
      }
      if( k_0 != -1 )
	dis_ab[0] = eval->fEdgeDisOrder[ a ][ k_0 ];
      else
	dis_ab[0] = eval->Direct( a, b_0 ); 
      if( k_1 != -1 )
	dis_ab[1] = eval->fEdgeDisOrder[ a ][ k_1 ];
      else
	dis_ab[1] = eval->Direct( a, b_1 ); 

      for( near_num = 1; near_num < nearMax; ++near_num )   
      {
	c = eval->fNearCity[ a ][ near_num ];
	dis_ac = eval->fEdgeDisOrder[ a ][ near_num ];  // Large

	if( fCenterUnit[ c ] == 0 )   
	{
	  for( j1 = 0; j1 < 2; ++j1 )
	  {
	    b = fListOfCenterUnit[ s-1+2*j1 ];

            for( j2 = 0; j2 < 2; ++j2 )
	    {
	      d = tKid.fLink[ c ][ j2 ];
	      k_cd = tKid.fOrder[ c ][ j2 ];    // Large
	      if( k_cd != -1 )
		dis_cd = eval->fEdgeDisOrder[ c ][ k_cd ];
	      else
		dis_cd = eval->Direct( c, d ); 

	      diff = dis_ab[j1] + dis_cd - dis_ac - eval->Direct(b, d);  // Large

	      if( diff > max_diff ) 
	      { 
	        aa = a; bb = b; a1 = c; b1 = d;
	        max_diff = diff;
	      }

	      diff =  dis_ab[j1] + dis_cd -
		eval->Direct(a, d) - eval->Direct(b, c);     // Large
	      if( diff > max_diff ) 
	      {
	        aa = a; bb = b; a1 = d; b1 = c;
	        max_diff = diff;
	      } 
	    }
	  }
	}
      }
    }

    if( a1 == -1 && nearMax == 10 ){ /* This value must also be changed if nearMax is chenged above */
      nearMax = 50;
      goto RESTART;
    }    
    else if( a1 == -1 && nearMax == 50  )
    {       
      int r = rand() % ( fNumOfElementInCU - 1 );
      a = fListOfCenterUnit[ r ];
      b = fListOfCenterUnit[ r+1 ];
      for( j = 0; j < fN; ++j )
      {
	if( fCenterUnit[ j ] == 0 )
        {
	  aa = a; bb = b;
	  a1 = j;
	  b1 = tKid.fLink[ j ][ 0 ];
	  break;
	}
      }
      max_diff = eval->Direct(aa, bb) + eval->Direct(a1, b1) -
	eval->Direct(a, a1) - eval->Direct(b, b1);          // Large
    }  

    // Large Large Large [1]
    fModiEdgeOrd[ fNumOfModiEdge ][0][1] = eval->GetOrd( aa, a1 ); // Large
    fModiEdgeOrd[ fNumOfModiEdge ][1][1] = eval->GetOrd( bb, b1 ); // Large
    fModiEdgeOrd[ fNumOfModiEdge ][2][1] = eval->GetOrd( a1, aa ); // Large
    fModiEdgeOrd[ fNumOfModiEdge ][3][1] = eval->GetOrd( b1, bb ); // Large

    if( tKid.fLink[aa][0] == bb ){
      tKid.fLink[aa][0] = a1;
      fModiEdgeOrd[ fNumOfModiEdge ][0][0] = tKid.fOrder[aa][0]; // Large
      tKid.fOrder[aa][0] = fModiEdgeOrd[ fNumOfModiEdge ][0][1]; // Large
    }
    else{
      tKid.fLink[aa][1] = a1;
      fModiEdgeOrd[ fNumOfModiEdge ][0][0] = tKid.fOrder[aa][1]; // Large
      tKid.fOrder[aa][1] = fModiEdgeOrd[ fNumOfModiEdge ][0][1]; // Large
    }
    if( tKid.fLink[bb][0] == aa ){
      tKid.fLink[bb][0] = b1;
      fModiEdgeOrd[ fNumOfModiEdge ][1][0] = tKid.fOrder[bb][0]; // Large
      tKid.fOrder[bb][0] = fModiEdgeOrd[ fNumOfModiEdge ][1][1]; // Large
    }
    else{
      tKid.fLink[bb][1] = b1;   
      fModiEdgeOrd[ fNumOfModiEdge ][1][0] = tKid.fOrder[bb][1]; // Large
      tKid.fOrder[bb][1] = fModiEdgeOrd[ fNumOfModiEdge ][1][1]; // Large
    }
    if( tKid.fLink[a1][0] == b1 ){ 
      tKid.fLink[a1][0] = aa;
      fModiEdgeOrd[ fNumOfModiEdge ][2][0] = tKid.fOrder[a1][0]; // Large
      tKid.fOrder[a1][0] = fModiEdgeOrd[ fNumOfModiEdge ][2][1]; // Large
    }
    else{
      tKid.fLink[a1][1] = aa;
      fModiEdgeOrd[ fNumOfModiEdge ][2][0] = tKid.fOrder[a1][1]; // Large
      tKid.fOrder[a1][1] = fModiEdgeOrd[ fNumOfModiEdge ][2][1]; // Large
    }
    if( tKid.fLink[b1][0] == a1 ){
      tKid.fLink[b1][0] = bb;
      fModiEdgeOrd[ fNumOfModiEdge ][3][0] = tKid.fOrder[b1][0]; // Large
      tKid.fOrder[b1][0] = fModiEdgeOrd[ fNumOfModiEdge ][3][1]; // Large
    }
    else{ 
      tKid.fLink[b1][1] = bb; 
      fModiEdgeOrd[ fNumOfModiEdge ][3][0] = tKid.fOrder[b1][1]; // Large
      tKid.fOrder[b1][1] = fModiEdgeOrd[ fNumOfModiEdge ][3][1]; // Large
    }
    //

    fModiEdge[ fNumOfModiEdge ][ 0 ] = aa;
    fModiEdge[ fNumOfModiEdge ][ 1 ] = bb;
    fModiEdge[ fNumOfModiEdge ][ 2 ] = a1;
    fModiEdge[ fNumOfModiEdge ][ 3 ] = b1;
    ++fNumOfModiEdge;

    fGainModi += max_diff;
    
    int posi_a1 = fInv[ a1 ];
    select_un = -1;
    for( int s = 0; s < fNumOfSeg; ++s ){
      if( fSegment[ s ][ 0 ] <= posi_a1 && posi_a1 <=  fSegment[ s ][ 1 ] ){
	select_un = fSegUnit[ s ];       
	break;
      }
    } 
    assert( select_un != -1 );

    for( int s = 0; s < fNumOfSeg; ++s ){
      if( fSegUnit[ s ] == select_un )
	fSegUnit[ s ] = center_un;
    }
    fNumOfElementInUnit[ center_un ] += fNumOfElementInUnit[ select_un ];
    
    for( int s = 0; s < fNumOfSeg; ++s ){
      if( fSegUnit[ s ] == fNumOfUnit - 1 )
	fSegUnit[ s ] = select_un;
    }
    fNumOfElementInUnit[ select_un ] = fNumOfElementInUnit[ fNumOfUnit - 1 ];
    --fNumOfUnit;

    for( int s = 0; s < fNumOfElementInCU; ++s ){
      c = fListOfCenterUnit[ s ];
      fCenterUnit[ c ] = 0;
    }
  }
}  


void TCross::MakeUnit()             
{
  int flag = 1; 
  for( int s = 0; s < fNumOfSPL; ++s ){
    if( fSegPosiList[ s ] == 0 ){
      flag = 0;
      break;
    }
  }
  if( flag == 1 ) 
  {
    fSegPosiList[ fNumOfSPL++ ] = 0;

    LinkBPosi[ fN-1 ][ 1 ]  = LinkBPosi[ fN-1 ][ 0 ];
    LinkBPosi[ 0 ][ 1 ] = LinkBPosi[ 0 ][ 0 ];
    LinkBPosi[ fN-1 ][ 0 ] = 0; 
    LinkBPosi[ 0 ][ 0 ] = fN-1;

  }

  tSort->Sort( fSegPosiList, fNumOfSPL );     


  fNumOfSeg = fNumOfSPL;
  for( int s = 0; s < fNumOfSeg-1; ++s ){
    fSegment[ s ][ 0 ] = fSegPosiList[ s ];
    fSegment[ s ][ 1 ] = fSegPosiList[ s+1 ]-1;
  }

  fSegment[ fNumOfSeg-1 ][ 0 ] = fSegPosiList[ fNumOfSeg-1 ];
  fSegment[ fNumOfSeg-1 ][ 1 ] = fN - 1;


  for( int s = 0; s < fNumOfSeg; ++s ){
    LinkAPosi[ fSegment[ s ][ 0 ] ] = fSegment[ s ][ 1 ];
    LinkAPosi[ fSegment[ s ][ 1 ] ] = fSegment[ s ][ 0 ];
    fPosiSeg[ fSegment[ s ][ 0 ] ] = s;
    fPosiSeg[ fSegment[ s ][ 1 ] ] = s;
  }

  for( int s = 0; s < fNumOfSeg; ++s )
    fSegUnit[ s ] = -1;
  fNumOfUnit = 0; 

  int p_st, p1, p2, p_next, p_pre; 
  int segNum;

  while(1)
  {
    flag = 0;
    for( int s = 0; s < fNumOfSeg; ++s ){
      if( fSegUnit[ s ] == -1 ){
	p_st = fSegment[ s ][ 0 ]; 
	p_pre = -1;
	p1 = p_st;
	flag = 1;
	break;
      }
    }
    if( flag == 0 )
      break;
    
    while(1)
    {
      segNum = fPosiSeg[ p1 ];
      fSegUnit[ segNum ] = fNumOfUnit;

      p2 = LinkAPosi[ p1 ];
      p_next = LinkBPosi[ p2 ][ 0 ];
      if( p1 == p2 ){
	if( p_next == p_pre )
	  p_next = LinkBPosi[ p2 ][ 1 ];
      } 
      
      if( p_next == p_st ){
	++fNumOfUnit;
	break;
      }

      p_pre = p2;
      p1 = p_next;
    }
  }
  
  for( int s = 0; s < fNumOfUnit; ++s )
    fNumOfElementInUnit[ s ] = 0; 
  
  int unitNum = -1;
  int tmpNumOfSeg = -1;
  for( int s = 0; s < fNumOfSeg; ++s ){
    if( fSegUnit[ s ] != unitNum ){
      ++tmpNumOfSeg;
      fSegment[ tmpNumOfSeg ][ 0 ] = fSegment[ s ][ 0 ];
      fSegment[ tmpNumOfSeg ][ 1 ] = fSegment[ s ][ 1 ];
      unitNum = fSegUnit[ s ];
      fSegUnit[ tmpNumOfSeg ] = unitNum;
      fNumOfElementInUnit[ unitNum ] += 
	fSegment[ s ][ 1 ] - fSegment[ s ][ 0 ] + 1;
    }
    else
    {
      fSegment[ tmpNumOfSeg ][ 1 ] = fSegment[ s ][ 1 ];
      fNumOfElementInUnit[ unitNum ] += 
	fSegment[ s ][ 1 ] - fSegment[ s ][ 0 ] + 1;
    }
  }
  fNumOfSeg = tmpNumOfSeg + 1; 
}


void TCross::BackToPa1( TIndi& tKid )
{
  int aa, bb, a1, b1; 
  int jnum;

  for( int s = fNumOfModiEdge -1; s >= 0; --s ){ 
    aa = fModiEdge[ s ][ 0 ];
    bb = fModiEdge[ s ][ 1 ];  
    a1 = fModiEdge[ s ][ 2 ];  
    b1 = fModiEdge[ s ][ 3 ];

    if( tKid.fLink[aa][0] == a1 ){
      tKid.fLink[aa][0] = bb;
      tKid.fOrder[aa][0] = fModiEdgeOrd[ s ][0][0];
    }
    else{
      tKid.fLink[aa][1] = bb;
      tKid.fOrder[aa][1] = fModiEdgeOrd[ s ][0][0];
    }
    if( tKid.fLink[a1][0] == aa ){
      tKid.fLink[a1][0] = b1;
      tKid.fOrder[a1][0] = fModiEdgeOrd[ s ][2][0];
    }
    else{
      tKid.fLink[a1][1] = b1;   
      tKid.fOrder[a1][1] = fModiEdgeOrd[ s ][2][0];
    }
    if( tKid.fLink[b1][0] == bb ){
      tKid.fLink[b1][0] = a1;
      tKid.fOrder[b1][0] = fModiEdgeOrd[ s ][3][0];
    }
    else{
      tKid.fLink[b1][1] = a1; 
      tKid.fOrder[b1][1] = fModiEdgeOrd[ s ][3][0];
    }
    if( tKid.fLink[bb][0] == b1 ){
      tKid.fLink[bb][0] = aa;
      tKid.fOrder[bb][0] = fModiEdgeOrd[ s ][1][0];
    }
    else{
      tKid.fLink[bb][1] = aa;
      tKid.fOrder[bb][1] = fModiEdgeOrd[ s ][1][0];
    }
  }
  
  for( int s = 0; s < fNumOfAppliedCycle; ++s ){
    jnum = fAppliedCylce[ s ];
    this->ChangeSol( tKid, jnum, 2 );
  }
}


void TCross::GoToBest( TIndi& tKid )
{
  int aa, bb, a1, b1; 
  int jnum;

  for( int s = 0; s < fNumOfBestAppliedCycle; ++s ){
    jnum = fBestAppliedCylce[ s ];
    this->ChangeSol( tKid, jnum, 1 );
  }


  for( int s = 0; s < fNumOfBestModiEdge; ++s )
  { 
    aa = fBestModiEdge[ s ][ 0 ];
    bb = fBestModiEdge[ s ][ 1 ];   
    a1 = fBestModiEdge[ s ][ 2 ];   
    b1 = fBestModiEdge[ s ][ 3 ];

    if( tKid.fLink[aa][0] == bb ){
      tKid.fLink[aa][0] = a1;
      tKid.fOrder[aa][0] = fBestModiEdgeOrd[ s ][0][1]; // Large
    }
    else{
      tKid.fLink[aa][1] = a1;
      tKid.fOrder[aa][1] = fBestModiEdgeOrd[ s ][0][1]; // Large
    }
    if( tKid.fLink[bb][0] == aa ){
      tKid.fLink[bb][0] = b1;
      tKid.fOrder[bb][0] = fBestModiEdgeOrd[ s ][1][1]; // Large
    }
    else{
      tKid.fLink[bb][1] = b1;   
      tKid.fOrder[bb][1] = fBestModiEdgeOrd[ s ][1][1]; // Large
    }
    if( tKid.fLink[a1][0] == b1 ){
      tKid.fLink[a1][0] = aa;
      tKid.fOrder[a1][0] = fBestModiEdgeOrd[ s ][2][1]; // Large
    }
    else{
      tKid.fLink[a1][1] = aa;
      tKid.fOrder[a1][1] = fBestModiEdgeOrd[ s ][2][1]; // Large
    }
    if( tKid.fLink[b1][0] == a1 ){
      tKid.fLink[b1][0] = bb;
      tKid.fOrder[b1][0] = fBestModiEdgeOrd[ s ][3][1]; // Large
    }
    else{ 
      tKid.fLink[b1][1] = bb; 
      tKid.fOrder[b1][1] = fBestModiEdgeOrd[ s ][3][1]; // Large
    }
  }
}


void TCross::IncrementEdgeFreq( int **fEdgeFreq )
{
  int j, jnum, cem;
  int r1, r2, b1, b2;
  int aa, bb, a1;
  int k;
  int posi;
  
  // AB-cycleによる更新 
  for( int s = 0; s < fNumOfBestAppliedCycle; ++s ){
    jnum = fBestAppliedCylce[ s ];

    posi = fPosi_ABL[jnum];      // Large
    cem = fABcycleL[ posi + 0 ]; // cem = fABcycle[ jnum ][ 0 ];  

    for( j = 0; j <cem/2; ++j )
    {                           
      // r1 = fABcycle[ jnum ][2+2*j]; r2 = fABcycle[ jnum ][3+2*j]; 
      // b1 = fABcycle[ jnum ][1+2*j]; b2 = fABcycle[ jnum ][4+2*j]; 
      r1 = fABcycleL[ posi + 2+2*j ]; r2 = fABcycleL[ posi + 3+2*j ]; 
      b1 = fABcycleL[ posi + 1+2*j ]; b2 = fABcycleL[ posi + 4+2*j ]; 

      // r1 - b1 add 
      // r1 - r2 remove
      // r2 - r1 remove
      // r2 - b2 add

      k = fABcycleLOrd[ posi + 2+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 2+2*j ][ 0 ]; // Large Large Large
      if( k != -1 ) 
	++fEdgeFreq[ r1 ][ k ];
      k = fABcycleLOrd[ posi + 2+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 2+2*j ][ 1 ];
      if( k != -1 ) 
	--fEdgeFreq[ r1 ][ k ];
      k = fABcycleLOrd[ posi + 3+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 0 ];
      if( k != -1 ) 
	--fEdgeFreq[ r2 ][ k ];
      k = fABcycleLOrd[ posi + 3+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 1 ];
      if( k != -1 ) 
	++fEdgeFreq[ r2 ][ k ];
    }
  }

  // Modificationによる更新
  for( int s = 0; s < fNumOfBestModiEdge; ++s )
  { 
    aa = fBestModiEdge[ s ][ 0 ];
    bb = fBestModiEdge[ s ][ 1 ];   
    a1 = fBestModiEdge[ s ][ 2 ];   
    b1 = fBestModiEdge[ s ][ 3 ];

    k = fBestModiEdgeOrd[ s ][0][0];          // Large Large Large
    if( k != -1 ) --fEdgeFreq[ aa ][ k ];
    k = fBestModiEdgeOrd[ s ][0][1];
    if( k != -1 ) ++fEdgeFreq[ aa ][ k ];

    k = fBestModiEdgeOrd[ s ][1][0];
    if( k != -1 ) --fEdgeFreq[ bb ][ k ];
    k = fBestModiEdgeOrd[ s ][1][1];
    if( k != -1 ) ++fEdgeFreq[ bb ][ k ];

    k = fBestModiEdgeOrd[ s ][2][0];
    if( k != -1 ) --fEdgeFreq[ a1 ][ k ];
    k = fBestModiEdgeOrd[ s ][2][1];
    if( k != -1 ) ++fEdgeFreq[ a1 ][ k ];

    k = fBestModiEdgeOrd[ s ][3][0];
    if( k != -1 ) --fEdgeFreq[ b1 ][ k ];
    k = fBestModiEdgeOrd[ s ][3][1];
    if( k != -1 ) ++fEdgeFreq[ b1 ][ k ];
  }
}


double TCross::Cal_ENT_Loss( int **fEdgeFreq ) // Large Large Large
{
  int j, jnum, cem;
  int r1, r2, b1, b2;
  int aa, bb, a1;
  double DLoss; 
  double h1, h2;
  int k;
  int posi;
  
  DLoss = 0;
  // AB-cycle
  for( int s = 0; s < fNumOfAppliedCycle; ++s ){
    jnum = fAppliedCylce[ s ];
    
    posi = fPosi_ABL[jnum];   // Large
    cem = fABcycleL[ posi + 0 ]; // fABcycle[ jnum ][ 0 ];  

    for( j = 0; j <cem/2; ++j )
    {                           
      // r1 = fABcycle[ jnum ][2+2*j]; r2 = fABcycle[ jnum ][3+2*j];  
      // b1 = fABcycle[ jnum ][1+2*j]; b2 = fABcycle[ jnum ][4+2*j]; 
      r1 = fABcycleL[ posi + 2+2*j]; r2 = fABcycleL[ posi + 3+2*j]; 
      b1 = fABcycleL[ posi + 1+2*j]; b2 = fABcycleL[ posi + 4+2*j]; 

      // r1 - b1 add   
      // r1 - r2 remove
      // r2 - r1 remove
      // r2 - b2 add

      // Remove
      k = fABcycleLOrd[ posi + 2+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 2+2*j ][ 1 ];
      if( k != -1 ){
	h1 = (double)( fEdgeFreq[ r1 ][ k ] - 1 )/(double)fNumOfPop;
	h2 = (double)( fEdgeFreq[ r1 ][ k ] )/(double)fNumOfPop;
	if( fEdgeFreq[ r1 ][ k ] - 1 != 0 )
	  DLoss -= h1 * log( h1 );
	DLoss += h2 * log( h2 );
	--fEdgeFreq[ r1 ][ k ]; 	
      }
      k = fABcycleLOrd[ posi + 3+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 0 ];      
      if( k != -1 )      
	--fEdgeFreq[ r2 ][ k ]; 	

      // Add
      k = fABcycleLOrd[ posi + 3+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 1 ];
      if( k != -1 ){
	h1 = (double)( fEdgeFreq[ r2 ][ k ] + 1 )/(double)fNumOfPop;
	h2 = (double)( fEdgeFreq[ r2 ][ k ])/(double)fNumOfPop;
	DLoss -= h1 * log( h1 );
	if( fEdgeFreq[ r2 ][ k ] != 0 )
	  DLoss += h2 * log( h2 );
	++fEdgeFreq[ r2 ][ k ]; 
      }
      k = fABcycleLOrd[ posi + 4+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 4+2*j ][ 0 ];            
      if( k != -1 )      
	++fEdgeFreq[ b2 ][ k ]; 
    }
  }

  // Modification
  for( int s = 0; s < fNumOfModiEdge; ++s )
  { 
    aa = fModiEdge[ s ][ 0 ];
    bb = fModiEdge[ s ][ 1 ];   
    a1 = fModiEdge[ s ][ 2 ];   
    b1 = fModiEdge[ s ][ 3 ];

    // Remove
    k = fModiEdgeOrd[ s ][0][0];   
    if( k != -1 ){
      h1 = (double)( fEdgeFreq[ aa ][ k ] - 1 )/(double)fNumOfPop;
      h2 = (double)( fEdgeFreq[ aa ][ k ] )/(double)fNumOfPop;
      if( fEdgeFreq[ aa ][ k ] - 1 != 0 )
	DLoss -= h1 * log( h1 );
      DLoss += h2 * log( h2 );
      --fEdgeFreq[ aa ][ k ];
    }
    k = fModiEdgeOrd[ s ][1][0];   
    if( k != -1 )
      --fEdgeFreq[ bb ][ k ];

    k = fModiEdgeOrd[ s ][2][0];   
    if( k != -1 ){
      h1 = (double)( fEdgeFreq[ a1 ][ k ] - 1 )/(double)fNumOfPop;
      h2 = (double)( fEdgeFreq[ a1 ][ k ] )/(double)fNumOfPop;
      if( fEdgeFreq[ a1 ][ k ] - 1 != 0 )
	DLoss -= h1 * log( h1 );
      DLoss += h2 * log( h2 );
      --fEdgeFreq[ a1 ][ k ];
    }
    k = fModiEdgeOrd[ s ][3][0];   
    if( k != -1 )
      --fEdgeFreq[ b1 ][ k ];

    // Add
    k = fModiEdgeOrd[ s ][0][1];   
    if( k != -1 ){
      h1 = (double)( fEdgeFreq[ aa ][ k ] + 1 )/(double)fNumOfPop;
      h2 = (double)( fEdgeFreq[ aa ][ k ])/(double)fNumOfPop;
      DLoss -= h1 * log( h1 );
      if( fEdgeFreq[ aa ][ k ] != 0 )
	DLoss += h2 * log( h2 );
      ++fEdgeFreq[ aa ][ k ];
    }
    k = fModiEdgeOrd[ s ][2][1];   
    if( k != -1 )
      ++fEdgeFreq[ a1 ][ k ];

    k = fModiEdgeOrd[ s ][1][1];   
    if( k != -1 ){
      h1 = (double)( fEdgeFreq[ bb ][ k ] + 1 )/(double)fNumOfPop;
      h2 = (double)( fEdgeFreq[ bb ][ k ])/(double)fNumOfPop;
      DLoss -= h1 * log( h1 );
      if( fEdgeFreq[ bb ][ k ] != 0 )
	DLoss += h2 * log( h2 );
      ++fEdgeFreq[ bb ][ k ];
    }
    k = fModiEdgeOrd[ s ][3][1];   
    if( k != -1 )
      ++fEdgeFreq[ b1 ][ k ];
  }
  DLoss = -DLoss;  
  
  // restore EdgeFreq
  for( int s = 0; s < fNumOfAppliedCycle; ++s ){
    jnum = fAppliedCylce[ s ];

    posi = fPosi_ABL[jnum];   // Large
    cem = fABcycleL[ posi + 0 ]; // fABcycle[ jnum ][ 0 ];  

    for( j = 0; j <cem/2; ++j )
    {                           
      // r1 = fABcycle[ jnum ][2+2*j]; r2 = fABcycle[ jnum ][3+2*j]; 
      // b1 = fABcycle[ jnum ][1+2*j]; b2 = fABcycle[ jnum ][4+2*j]; 
      r1 = fABcycleL[ posi + 2+2*j ]; r2 = fABcycleL[ posi + 3+2*j ]; 
      b1 = fABcycleL[ posi + 1+2*j ]; b2 = fABcycleL[ posi + 4+2*j ]; 

      k = fABcycleLOrd[ posi + 2+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 2+2*j ][ 1 ];
      if( k != -1 ) ++fEdgeFreq[ r1 ][ k ]; 	
      k = fABcycleLOrd[ posi + 3+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 0 ];      
      if( k != -1 ) ++fEdgeFreq[ r2 ][ k ]; 	     

      k = fABcycleLOrd[ posi + 3+2*j ][ 1 ]; // fABcycleOrd[ jnum ][ 3+2*j ][ 1 ];
      if( k != -1 ) --fEdgeFreq[ r2 ][ k ]; 
      k = fABcycleLOrd[ posi + 4+2*j ][ 0 ]; // fABcycleOrd[ jnum ][ 4+2*j ][ 0 ];            
      if( k != -1 ) --fEdgeFreq[ b2 ][ k ];      
    }
  }

  for( int s = 0; s < fNumOfModiEdge; ++s )
  { 
    aa = fModiEdge[ s ][ 0 ];
    bb = fModiEdge[ s ][ 1 ];   
    a1 = fModiEdge[ s ][ 2 ];   
    b1 = fModiEdge[ s ][ 3 ];

    k = fModiEdgeOrd[ s ][0][0];   
    if( k != -1 ) ++fEdgeFreq[ aa ][ k ];
    k = fModiEdgeOrd[ s ][1][0];   
    if( k != -1 ) ++fEdgeFreq[ bb ][ k ];

    k = fModiEdgeOrd[ s ][2][0];   
    if( k != -1 ) ++fEdgeFreq[ a1 ][ k ];
    k = fModiEdgeOrd[ s ][3][0];   
    if( k != -1 ) ++fEdgeFreq[ b1 ][ k ];

    k = fModiEdgeOrd[ s ][0][1];   
    if( k != -1 ) --fEdgeFreq[ aa ][ k ];
    k = fModiEdgeOrd[ s ][2][1];   
    if( k != -1 ) --fEdgeFreq[ a1 ][ k ];

    k = fModiEdgeOrd[ s ][1][1];   
    if( k != -1 ) --fEdgeFreq[ bb ][ k ];
    k = fModiEdgeOrd[ s ][3][1];   
    if( k != -1 ) --fEdgeFreq[ b1 ][ k ];
  }

  return DLoss;
}


void TCross::SetWeight( const TIndi& tPa1, const TIndi& tPa2 ) 
{
  int cem;
  int r1, r2, v1, v2, v_p;
  int AB_num;
  int posi;

  for( int i = 0; i < fN; ++i ){
    fInEffectNode[ i ][ 0 ] = -1;  
    fInEffectNode[ i ][ 1 ] = -1;
  }

  // Step 1:
  for( int s = 0; s < fNumOfABcycle; ++s ){
    posi = fPosi_ABL[ s ];      // Large
    cem = fABcycleL[ posi + 0 ]; // fABcycle[ s ][ 0 ];  
    for( int j = 0; j < cem/2; ++j ){
      r1 = fABcycleL[ posi + 2*j+2 ]; // fABcycle[ s ][ 2*j+2 ]; 
      r2 = fABcycleL[ posi + 2*j+3 ]; // fABcycle[ s ][ 2*j+3 ]; 

      if( fInEffectNode[ r1 ][ 0 ] == -1 ) fInEffectNode[ r1 ][ 0 ] = s;
      else if ( fInEffectNode[ r1 ][ 1 ] == -1 ) fInEffectNode[ r1 ][ 1 ] = s;
      else assert( 1 == 2 );

      if( fInEffectNode[ r2 ][ 0 ] == -1 ) fInEffectNode[ r2 ][ 0 ] = s;
      else if ( fInEffectNode[ r2 ][ 1 ] == -1 ) fInEffectNode[ r2 ][ 1 ] = s;
      else assert( 1 == 2 );
    }
  }
  
  // Step 2:
  for( int i = 0; i < fN; ++i ){
    if( fInEffectNode[ i ][ 0 ] != -1 && fInEffectNode[ i ][ 1 ] == -1 ){ 
      AB_num = fInEffectNode[ i ][ 0 ];
      v1 = i;

      if( tPa1.fLink[ v1 ][ 0 ] != tPa2.fLink[ v1 ][ 0 ] && tPa1.fLink[ v1 ][ 0 ] != tPa2.fLink[ v1 ][ 1 ] )
	v_p = tPa1.fLink[ v1 ][ 0 ];
      else if( tPa1.fLink[ v1 ][ 1 ] != tPa2.fLink[ v1 ][ 0 ] && tPa1.fLink[ v1 ][ 1 ] != tPa2.fLink[ v1 ][ 1 ] )
	v_p = tPa1.fLink[ v1 ][ 1 ];
      else
	assert( 1 == 2 );

      while( 1 ){
	assert( fInEffectNode[ v1 ][ 0 ] != -1 );
	assert( fInEffectNode[ v1 ][ 1 ] == -1 );
	fInEffectNode[ v1 ][ 1 ] = AB_num;

	if( tPa1.fLink[ v1 ][ 0 ] != v_p )
	  v2 = tPa1.fLink[ v1 ][ 0 ];
	else if( tPa1.fLink[ v1 ][ 1 ] != v_p )
	  v2 = tPa1.fLink[ v1 ][ 1 ];
	else 
	  assert( 1 == 2 );

	if( fInEffectNode[ v2 ][ 0 ] == -1 )
	  fInEffectNode[ v2 ][ 0 ] = AB_num;
	else if( fInEffectNode[ v2 ][ 1 ] == -1 )
	  fInEffectNode[ v2 ][ 1 ] = AB_num;
	else 
	  assert( 1 == 2 );
	
	if( fInEffectNode[ v2 ][ 1 ] != -1 )
	  break;

	v_p = v1;
	v1 = v2;
      }
    }
  }

  // Step 3:
  assert( fNumOfABcycle < fMaxNumOfABcycle );
  for( int s1 = 0; s1 < fNumOfABcycle; ++s1 ){
    fWeight_C[ s1 ] = 0;
    for( int s2 = 0; s2 < fNumOfABcycle; ++s2 ){
      fWeight_RR[ s1 ][ s2 ] = 0;
    }
  }
  
  for( int i = 0; i < fN; ++i ){
    assert( (fInEffectNode[ i ][ 0 ] == -1 && fInEffectNode[ i ][ 1 ] == -1) ||
	    (fInEffectNode[ i ][ 0 ] != -1 && fInEffectNode[ i ][ 1 ] != -1) );

    if( fInEffectNode[ i ][ 0 ] != -1 && fInEffectNode[ i ][ 1 ] != -1 ){
      ++fWeight_RR[ fInEffectNode[ i ][ 0 ] ][ fInEffectNode[ i ][ 1 ] ];
      ++fWeight_RR[ fInEffectNode[ i ][ 1 ] ][ fInEffectNode[ i ][ 0 ] ];
    }
    if( fInEffectNode[ i ][ 0 ] != fInEffectNode[ i ][ 1 ] ){
      ++fWeight_C[ fInEffectNode[ i ][ 0 ] ];
      ++fWeight_C[ fInEffectNode[ i ][ 1 ] ];
    }
    
  }
  for( int s1 = 0; s1 < fNumOfABcycle; ++s1 )
    fWeight_RR[ s1 ][ s1 ] = 0;
  

  for( int i = 0; i < fN; ++i ){
    assert( ( fInEffectNode[ i ][ 0 ] != -1 && fInEffectNode[ i ][ 1 ] != -1 ) ||
	    ( fInEffectNode[ i ][ 0 ] == -1 && fInEffectNode[ i ][ 1 ] == -1 ) );
  }

}


int TCross::Cal_C_Naive() 
{
  int count_C;
  int tt;

  count_C = 0;

  for( int i = 0; i < fN; ++i ){
    if( fInEffectNode[ i ][ 0 ] != -1 && fInEffectNode[ i ][ 1 ] != -1 ){
      tt = 0;
      if( fUsedAB[ fInEffectNode[ i ][ 0 ] ] == 1 )
	++tt;
      if( fUsedAB[ fInEffectNode[ i ][ 1 ] ] == 1 )
	++tt;
      if( tt == 1 )
	++count_C;
    }
  }
  return count_C;
}

void TCross::Search_Eset( int centerAB ) 
{
  int nIter, stagImp;
  int delta_weight, min_delta_weight_nt;
  int flag_AddDelete, flag_AddDelete_nt;
  int selected_AB, selected_AB_nt;
  int t_max;
  int jnum;

  fNum_C = 0;  // Number of C nodes in E-set
  fNum_E = 0;  // Number of Edges in E-set 

  fNumOfUsedAB = 0;
  for( int s1 = 0; s1 < fNumOfABcycle; ++s1 ){
    fUsedAB[ s1 ] = 0;
    fWeight_SR[ s1 ] = 0;
    fMoved_AB[ s1 ] = 0;
  }

  for( int s = 0; s < fNumOfABcycleInEset; ++s )   
  {
    jnum = fABcycleInEset[ s ];
    this->Add_AB( jnum );
  }
  fBest_Num_C = fNum_C;
  fBest_Num_E = fNum_E;
  
  stagImp = 0;
  nIter = 0;
  while( 1 )
  { 
    ++nIter; 

    min_delta_weight_nt = 99999999;  
    flag_AddDelete = 0;
    flag_AddDelete_nt = 0;
    for( int s1 = 0; s1 < fNumOfABcycle; ++s1 )
    {
      if( fUsedAB[ s1 ] == 0 && fWeight_SR[ s1 ] > 0 )
      {
	delta_weight = fWeight_C[ s1 ] - 2 * fWeight_SR[ s1 ];   
	if( fNum_C + delta_weight < fBest_Num_C ){
	  selected_AB = s1;
	  flag_AddDelete = 1;
	  fBest_Num_C = fNum_C + delta_weight;
	}
	if( delta_weight < min_delta_weight_nt && nIter > fMoved_AB[ s1 ] ){
	  selected_AB_nt = s1;
	  flag_AddDelete_nt = 1;
	  min_delta_weight_nt = delta_weight;
	}
      }
      else if( fUsedAB[ s1 ] == 1 && s1 != centerAB )
      {
	delta_weight = - fWeight_C[ s1 ] + 2 * fWeight_SR[ s1 ];   
	if( fNum_C + delta_weight < fBest_Num_C ){
	  selected_AB = s1;
	  flag_AddDelete = -1;
	  fBest_Num_C = fNum_C + delta_weight;
	}
	if( delta_weight < min_delta_weight_nt && nIter > fMoved_AB[ s1 ] ){
	  selected_AB_nt = s1;
	  flag_AddDelete_nt = -1;
	  min_delta_weight_nt = delta_weight;
	}
      }
    }
      
    if( flag_AddDelete != 0 ){
      if( flag_AddDelete == 1 ){
	this->Add_AB( selected_AB );
      }
      else if( flag_AddDelete == -1 )
	this->Delete_AB( selected_AB );
      
      fMoved_AB[ selected_AB ] = nIter + tRand->Integer( 1, fTmax ); 
      assert( fBest_Num_C == fNum_C );
      fBest_Num_E = fNum_E;

      fNumOfABcycleInEset = 0;
      for( int s1 = 0; s1 < fNumOfABcycle; ++s1 ){
	if( fUsedAB[ s1 ] == 1 )
	  fABcycleInEset[ fNumOfABcycleInEset++ ] = s1;
      }
      assert( fNumOfABcycleInEset == fNumOfUsedAB );      
      stagImp = 0;
    }
    else if( flag_AddDelete_nt != 0 ) {
      if( flag_AddDelete_nt == 1 ){
	this->Add_AB( selected_AB_nt );
      }
      else if( flag_AddDelete_nt == -1 )
	this->Delete_AB( selected_AB_nt );

      fMoved_AB[ selected_AB_nt ] = nIter + tRand->Integer( 1, fTmax ); 
    } 

    if( flag_AddDelete == 0 )
      ++stagImp;
    if( stagImp == fMaxStag )
      break;
  }
}


void TCross::Add_AB( int AB_num )  
{
  fNum_C += fWeight_C[ AB_num ] - 2 * fWeight_SR[ AB_num ];   
  fNum_E += fABcycleL[ fPosi_ABL[AB_num] + 0 ] / 2; // fABcycle[ AB_num ][ 0 ] / 2;  

  assert( fUsedAB[ AB_num ] == 0 );
  fUsedAB[ AB_num ] = 1;
  ++fNumOfUsedAB;

  for( int s1 = 0; s1 < fNumOfABcycle; ++s1 ){
    fWeight_SR[ s1 ] += fWeight_RR[ s1 ][ AB_num ];
  }
}


void TCross::Delete_AB( int AB_num )  
{
  fNum_C -= fWeight_C[ AB_num ] - 2 * fWeight_SR[ AB_num ];   
  fNum_E -= fABcycleL[ fPosi_ABL[AB_num] + 0 ] / 2; // fABcycle[ AB_num ][ 0 ] / 2;  

  assert( fUsedAB[ AB_num ] == 1 );
  fUsedAB[ AB_num ] = 0;
  --fNumOfUsedAB;

  for( int s1 = 0; s1 < fNumOfABcycle; ++s1 ){
    fWeight_SR[ s1 ] -= fWeight_RR[ s1 ][ AB_num ];
  }
}


void TCross::CheckValid( TIndi& indi )
{
  int curr, pre, next, st;
  int count;
  int p, c, n;

  st = 0;
  curr = -1;
  next = st;

  count = 0;
  while(1){ 
    pre = curr;
    curr = next;
    ++count;
    if( indi.fLink[ curr ][ 0 ] != pre )
      next = indi.fLink[ curr ][ 0 ];
    else 
      next = indi.fLink[ curr ][ 1 ]; 
    
    if( next == st ) break;

    c = curr;
    p = indi.fLink[ c ][ 0 ];
    n = indi.fLink[ c ][ 1 ];

    if( indi.fOrder[ c ][ 0 ] != -1 ){
      assert( eval->fNearCity[ c ][  indi.fOrder[ c ][ 0 ] ] == p );
    }
    if( indi.fOrder[ c ][ 1 ] != -1 ){
      assert( eval->fNearCity[ c ][  indi.fOrder[ c ][ 1 ] ] == n );
    }

    if( count > fN ){
      printf( "Invalid = %d\n", count );
      break;
    }
  }       
  if( count != fN )
    printf( "Invalid = %d\n", count );
}

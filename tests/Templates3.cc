/*
 =========================================================================
 
   Complile with  g++ -g -o Templates3.g++.out  Templates3.cc
 
   (Also compliles with SC4 and clcc. These compilers do template
   instantiation at link time and do not cause the problem observed
   with g++.)
*/
/*
   -----------------  Templates3.cc      -----------------
*/
#include "Templates3x.h"
 
typedef int (*PF1)(char*);
 
main()
{
    myBiggerClass<int,int, 2>* aBiggerClass = new myBiggerClass<int,int, 2>;
    aBiggerClass->SetX(2);
    aBiggerClass->SetY(4);
    delete aBiggerClass;
 
    myBiggerClass<int,double, 5>* fBiggerClass = new myBiggerClass<int,double, 5>;
    fBiggerClass->SetX(2);
    fBiggerClass->SetY(4.0);
    delete fBiggerClass;
 
    myBiggerClass<short*,PF1, -1>* pf1BiggerClass = new myBiggerClass<short*,PF1, -1>;
    delete pf1BiggerClass;
}


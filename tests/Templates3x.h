/*
   -----------------  Templates3x.h      -----------------
*/
#ifndef Templates3x_H
#define Templates3x_H
 
template <class T> class myClass
{
 public:
    myClass() {}
    virtual void SetX(T );
    virtual ~myClass();
 private:
    T x;
};
 
template<class T, class TT, int I> class myBiggerClass : public myClass<T>
{
 public:
    void SetY(TT tt) { y = tt; }
 private:
    TT y;
    int ints[I>0?I:-I];
};
 
#ifdef __GNUC__
#include "Templates3x.cc"
#endif
 
#endif // Templates3x_H


/* test various types of declarations */
#include <assert.h>
#include <stdio.h>
struct Structure;
typedef struct Structure Structure;
struct Structure {
	char dummy;
};
enum boolean { false, true };
typedef enum boolean bool;
bool b = false;
int b_int = true;
typedef bool Boolean;
struct { int dummy; } S;
struct SS { int dummy; } SS;
typedef struct { int a; } TD;
typedef TD *func_t(void);
func_t *fptr;
TD *myfunc(void) {}
func_t *farry[] = { myfunc };
enum { ok, error };
struct {
	union {
		char c;
		int i;
	} u;
	int type;
} su;
typedef int mytype;
struct ST {
	int mytype;
};
int function_prototype(int mytype);
void oldstyle_function(x,mytyp)				
char x;
int mytyp;					/* hides typedef */
{ 
	assert(x == 'Z');
	assert(mytyp == 5);
	mytyp += 1;
	x = 'A';
}
void newstyle_function(mytype m)		/* typedef used here */
{
	m += 1;
	assert(m == 11);
}
void another_newstyle_function(void)
{
	struct mytype { char mytype[1]; };
	struct mytype mt;
	struct ST st, *stp;

	st.mytype = (mytype)1;			/* .name */
	stp = &st;

	goto mytype;
	stp->mytype = 2;			/* ->name */
mytype:
	assert(stp->mytype == 1);
	{
		int mytype;			/* hides typedef */
		mytype = 1;
		stp->mytype = mytype;
		mytype = mytype ? mytype : mytype;
		mt.mytype[mytype] = '\0';
		assert(mytype == 1);
		assert(mt.mytype[mytype] == '\0');	
	}
	assert(stp == &st);
	assert(stp->mytype == 1);
}
int test_function(void)
{
	enum { mytype = 1 };			/* hides typedef */
	Boolean b = false;
	struct T *t;
	bool c = b;

	{
		typedef char *mytype;		/* hides enum */
		const mytype mt = "hello";	
	}

	goto mytype;

	S.dummy = 1;
	SS.dummy = 2;
	fptr = myfunc;
	fptr = &myfunc;

mytype:
	su.u.c = 'a';

	assert(su.u.c == 'a');
	assert(fptr != myfunc);
	assert(b == false);
	assert(mytype == 1);

	return ok ? true : false;
}
int handler(int dummy) {return 128;}
int (*signal2(int i, int (*fptr)(int)))(int)
{
	assert(i == 10);
	return fptr;
}  
signed int t_func(signed int (*x)(signed int arg)) { return 35; }
signed int t_arg(signed int i) {}
typedef char _x;
void _f(_x a, _x y);
struct _s { int a,_x; };
void _x_func(void) {
	int i=sizeof(_x),_x[sizeof(_x)];
	int j=sizeof(_x);
	_x[0] = 2;
	assert(i == 1);
	assert(_x[0] == 2);
	assert(j == 4);
}
int main()
{
	typedef int MILES, KLICKSP();
	typedef struct { double hi, lo; } range;
	MILES distance;
	extern KLICKSP *metricp;
	range x;
	range z, *zp;
	typedef signed int t;
	typedef int plain;
	struct tag {
		unsigned t:4;
		const t:5;
		plain r:5;
	};
	struct tag T;
	typedef int fv(int), (*pfv)(int);
	extern int (*signal(int, int (*)(int)))(int);
	extern fv *signal2(int, fv *);
	extern pfv signal3(int, pfv);

	distance = 5;
	x.hi = x.lo = 1.0;
	z = x;
	zp = &z;
	T.t = 4;
	T.r = 2;
	
	oldstyle_function('Z',5);
	newstyle_function(10);
	another_newstyle_function();
	test_function();
	assert(distance == 5);
	assert(zp->hi == 1.0 && zp->lo == 1.0);
	assert(T.t == 4 && T.r == 2);
	assert(signal2(10, handler) == handler);
	assert(signal2(10, handler)(1) == 128);

	{
		typedef t t_func_type(t (t));
		t_func_type *fptr = t_func;
		assert(fptr(t_arg) == 35);
	}

        printf("sizeof(struct tmp {int i;}) = %d\n", 
		sizeof(struct tmp {int i;}));

	printf("test_decl2.c Tested OK\n");
}

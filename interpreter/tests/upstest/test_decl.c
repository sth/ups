/* test various types of declarations */
extern;						/* should fail */
int;						/* should fail */
typedef struct Structure Structure;
struct Structure {
	char dummy;
};
enum boolean { false, true };
typedef enum boolean bool;
bool b = false;
typedef int bool;				/* redefinition: should fail */
typedef bool Boolean;
struct { int a, b; };				/* no tag: should fail */
static struct T { int a, b; };			/* static: should fail */
struct { int dummy; } S;
struct SS { int dummy; } SS;
typedef struct SS;				/* no declarator: should fail */
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
void oldstyle_function(x,mytype)				
char x;
int mytype;					/* hides typedef */
{ 
	mytype += 1;
	x = 'A';
}
void newstyle_function(mytype m)		/* typedef used here */
{
	m += 1;
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
	{
		int mytype;			/* hides typedef */
		mytype = 1;
		stp->mytype = mytype;
		mytype = mytype ? mytype : mytype;
		mt.mytype[mytype] = '\0';
	}
}
int main()
{
	enum { mytype = 1 };			/* hides typedef */
	Boolean b = (int)false;
	struct T t;
	bool c = (bool) b;
 	// typedef void *mytype;		/* redeclaration: should fail */

	{
		typedef char *mytype;		/* hides enum */
		const mytype mytype = "hello";	/* redeclaration: should fail */
	}

	goto mytype;
	S.dummy = 1;
	SS.dummy = 2;
	fptr = myfunc;
	fptr = &myfunc;
mytype:
	su.u.c = 'a';
	return (int)ok ? (int)true : (int)false;
}
void test_function(void)
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
	typedef void fv(int), (*pfv)(int);
	extern void (*signal(int, void (*)(int)))(int);
	extern fv *signal2(int, fv *);
	extern pfv signal3(int, pfv);
}
int bool BB;					/* two type specifiers, 
						 * should fail */

/* Cinterpreter.c - implements a front-end to the C Interpreter API */

#include "Cinterpreter.h"

struct linebuf_s {
	char **lb_srclines;		/* Array of source lines */
	int lb_n_srclines;		/* Number of lines */
	int lb_current_line;		/* Current line */
};
typedef struct linebuf_s linebuf_t;

static char **Libvars;

/* local prototypes */
static bool checkarg PROTO((type_t *type, int nargs, int argn, long *p_val));
static int noread PROTO((taddr_t addr, voidptr buf, size_t nbytes));
static int nowrite PROTO((taddr_t addr, constvoidptr buf, size_t nbytes));
static const char *getline PROTO((char *arg));

static bool
checkarg(type, nargs, argn, p_val)
type_t *type;
int nargs, argn;
long *p_val;
{
	*p_val = (long)type;
	return TRUE;
}

static int
noread(addr, buf, nbytes)
taddr_t addr;
voidptr buf;
size_t nbytes;
{
	panic("unexpected readdata");
	return -1;
}

static int
nowrite(addr, buf, nbytes)
taddr_t addr;
constvoidptr buf;
size_t nbytes;
{
	panic("unexpected writedata");
	return -1;
}

static const char *
getline(arg)
char *arg;
{
	linebuf_t *lb;

	lb = (linebuf_t *) arg;
	if (lb->lb_current_line == lb->lb_n_srclines) {
		return NULL;
	}
	return lb->lb_srclines[lb->lb_current_line++];
} 

ci_nametype_t
ci_standard_getaddr(name, p_addr)
const char *name;
taddr_t *p_addr;
{
	ci_nametype_t nametype;

	if ((nametype = cc_getaddr(name, p_addr)) == CI_DATA)
		*p_addr = (taddr_t)Libvars[*p_addr];

	return nametype;
}

void
demangle_name_2(name, len, alloc_id, ptr, func, fil)
char *name;
int len;
alloc_pool_t *alloc_id;
char **ptr;
int func;
fil_t *fil;
{
	*ptr = alloc(alloc_id, strlen(name)+1);
	strcpy(*ptr, name);
}


/* Parse and compile C source into byte-code */
bool
ci_create_interpreter(ci, srclines, getaddr_proc)
Cinterpreter_t *ci;
char **srclines;
ci_getaddrproc_t getaddr_proc;
{
	int n;
	linebuf_t lb;
	size_t n_libfuncs, n_libvars;
	ci_getaddrproc_t mygetaddr;

	for (n = 0; srclines[n] != NULL; n++);
	lb.lb_n_srclines = n;
	lb.lb_current_line = 0;
	lb.lb_srclines = srclines;

	ci->ci_parse_id = NULL;
	ci->ci_machine = NULL;
	ci->ci_linkinfo = NULL;

	/** Dibyendu: New warning messages **/
	ci_add_message_action("Floating point suffix ignored", MA_WARNING_ONLY);
	ci_add_message_action("Integer suffix ignored", MA_WARNING_ONLY);
	ci_add_message_action("Cast ignored in initialisation", MA_WARNING_ONLY);
	ci_add_message_action("Conversion to '", MA_WARNING_ONLY);

	/** Dibyendu: These are now warnings rather than errors **/
	ci_add_message_action("call via old style function expression",
				      MA_WARNING_ONLY);
	ci_add_message_action("No prototype in scope", MA_WARNING_ONLY);
	ci_add_message_action("Implicit declaration of", MA_WARNING_ONLY);

	ci->ci_parse_id = ci_parse_file("<internal file>", NULL,
					CI_DONT_PANIC, 
					cc_report_error, NULL, 
					getline, (char *)&lb);

	if (ci->ci_parse_id == NULL) 
		return FALSE;

	ci->ci_linkinfo = ci_compile("<internal code>", 
			ci->ci_parse_id, cc_report_error,
			checkarg, NULL, CI_CP_DONT_PANIC | CI_CP_ONEFILE); 

	if (ci->ci_linkinfo == NULL) {
		ci_free_parse_id(ci->ci_parse_id);
		ci->ci_parse_id = NULL;
		return FALSE;
	}

	cc_get_libinfo(&n_libfuncs, &n_libvars);
	Libvars = cx_get_libvars(n_libvars);

	if (getaddr_proc != NULL) {
		mygetaddr = getaddr_proc;
	}
	else {
		mygetaddr = ci_standard_getaddr;
	}
	ci->ci_machine = ci_make_machine_from_linkinfo(ci->ci_linkinfo,
				NULL, 0, mygetaddr, NULL, NULL,
				cc_report_error);

	if (ci->ci_machine == NULL)  {
		ci_free_linkinfo(ci->ci_linkinfo);
		ci->ci_linkinfo = NULL;
		ci_free_parse_id(ci->ci_parse_id);
		ci->ci_parse_id = NULL;
		return FALSE;
	}

	ci->ci_machine->ma_libfuncs = cx_get_libfuncs(n_libfuncs);

	ci_push_trap_frame(ci->ci_machine);
	ci_initialise_machine(ci->ci_machine, TRUE, FALSE, NULL, NULL);

	return TRUE;
}

/* Preprocess, parse and compile the source to byte-code */
bool
ci_create_interpreter_from_file(ci, srcfile, args, getaddr_proc)
Cinterpreter_t *ci;
const char *srcfile;
char *args[];
ci_getaddrproc_t getaddr_proc;
{
	ccstate_t *cs = NULL;
	char **arg;
	size_t n_libfuncs, n_libvars;
	ci_getaddrproc_t mygetaddr;

	cs = cc_create_ccstate("", CI_CP_ONEFILE | CI_CP_DONT_PANIC);
	if (args != NULL) {
		for (arg = args; *arg != NULL; ++arg) {
			cc_handle_arg(cs, &arg);
		}
	}

	ci->ci_parse_id = NULL;
	ci->ci_machine = NULL;
	ci->ci_linkinfo = NULL;

	ci->ci_linkinfo = cc_parse_and_compile(cs, srcfile, &ci->ci_parse_id,
		checkarg);
	cc_free_ccstate(cs);
	if (ci->ci_linkinfo == NULL) {
		if (ci->ci_parse_id != NULL)
			ci_free_parse_id(ci->ci_parse_id);
		ci->ci_parse_id = NULL;
		return FALSE;
	}

	cc_get_libinfo(&n_libfuncs, &n_libvars);
	Libvars = cx_get_libvars(n_libvars);

	if (getaddr_proc != NULL)
		mygetaddr = getaddr_proc;
	else
		mygetaddr = ci_standard_getaddr;
	ci->ci_machine = ci_make_machine_from_linkinfo(ci->ci_linkinfo,
				NULL, 0, mygetaddr, NULL, NULL,
				cc_report_error);

	if (ci->ci_machine == NULL) {
		ci_free_linkinfo(ci->ci_linkinfo);
		ci->ci_linkinfo = NULL;
		ci_free_parse_id(ci->ci_parse_id);
		ci->ci_parse_id = NULL;
		return FALSE;
	}

	ci->ci_machine->ma_libfuncs = cx_get_libfuncs(n_libfuncs);

	ci_push_trap_frame(ci->ci_machine);
	ci_initialise_machine(ci->ci_machine, TRUE, FALSE, NULL, NULL);

	return TRUE;
}

/* Run the C Interpreter Virtual Machine */
ci_exec_result_t
ci_execute_interpreter(ci, initialise, call_proc)
Cinterpreter_t *ci;
bool initialise;
ci_indirect_call_proc_t call_proc;
{
	ci_exec_result_t res;

	if (initialise)
		ci_reset_machine(ci->ci_machine);
	ci_initialise_machine(ci->ci_machine, initialise, FALSE, 
		NULL, NULL);

	res = ci_execute_machine(ci->ci_machine, (taddr_t)0, (taddr_t)0, 
			noread, nowrite, call_proc);

	return res;
}

/* Release all resources acquired by the C Interpreter */
void
ci_destroy_interpreter(ci)
Cinterpreter_t *ci;
{
	if (ci->ci_parse_id != NULL) {
		ci_free_parse_id(ci->ci_parse_id);
		ci->ci_parse_id = NULL;
	}
	if (ci->ci_linkinfo != NULL) {
		ci_free_linkinfo(ci->ci_linkinfo);
		ci->ci_linkinfo = NULL;
	}
	if (ci->ci_machine != NULL) {
		ci_reset_machine(ci->ci_machine);
		ci_free_machine(ci->ci_machine);
	}
}

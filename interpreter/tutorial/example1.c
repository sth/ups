/* example1.c - simplified version of cg.c */
#include <mtrprog/ifdefs.h>
#include <local/ukcprog.h>

#include <ups.h>
#include <symtab.h>
#include <ci.h>
#include <xc_opcodes.h>
#include <xc_machine.h>
#include <xc_builtins.h>
#include <cc.h>

static char **Libvars;
static ci_nametype_t
getaddr(name, p_addr)
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
	*ptr = strdup (name);
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

int main(int argc, char *argv[])
{
	ccstate_t *cs;
	char *compile_args[] = { "-oldc", "-D__STDC__", "-U__GNUC__", 
			"-I/usr/include", NULL };
	char **argptr;
	linkinfo_t *li;
	parse_id_t parse_id;
	char *entry_name;
	char *srcfile;
	int res;
	int status = 127;
	size_t n_libfuncs, n_libvars;
	machine_t *ma;
	char *hello_args[] = { "hello.c", NULL };
	char *envp[] = { NULL };

	cs = cc_create_ccstate("", (unsigned)CI_CP_ONEFILE);
	for (argptr = compile_args; *argptr != NULL; ++argptr)
		cc_handle_arg(cs, &argptr);

	if (argc > 1) srcfile = argv[1];
	else srcfile = "hello.c";
	li = cc_parse_and_compile(cs, srcfile, &parse_id, NULL);
	if (li == NULL)
		goto done;

	cc_get_libinfo(&n_libfuncs, &n_libvars);
	Libvars = cx_get_libvars(n_libvars);
	
	ma = ci_make_machine_from_linkinfo(li, "main", 10240, getaddr,
					   (ci_get_regaddr_proc_t)NULL,
					   (char *)NULL, cc_report_error);
	if (ma == NULL)
		goto done;

	ma->ma_libfuncs = cx_get_libfuncs(n_libfuncs);
	ci_initialise_machine(ma, TRUE, FALSE, hello_args, envp);

	res = ci_execute_machine(ma, (taddr_t)0, (taddr_t)0, noread, nowrite,
				 (ci_indirect_call_proc_t)NULL);
	if (res != CI_ER_EXITED && res != CI_ER_TRAP) {
		errf("Error executing %s: %s",
			srcfile, ci_exec_result_to_string(res));
		goto done;
	}

	status = ci_get_exit_status(ma);
	
done:
	if (parse_id != NULL)
		ci_free_parse_id(parse_id);
	if (li != NULL)
		ci_free_linkinfo(li);
	if (ma != NULL) {
		ci_reset_machine(ma);
		ci_free_machine(ma);
	}

	return status;
}

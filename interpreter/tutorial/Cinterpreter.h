/* Cinterpreter.h - Common header file for tutorial examples */

#ifndef INC_CINTERPRETER_H
#define INC_CINTERPRETER_H

#include <mtrprog/ifdefs.h>
#include <local/ukcprog.h>

#include <ups.h>
#include <symtab.h>
#include <ci.h>
#include <xc_opcodes.h>
#include <xc_machine.h>
#include <xc_builtins.h>
#include <cc.h>

struct Cinterpreter_s {
	parse_id_t ci_parse_id;
	machine_t *ci_machine;
	linkinfo_t *ci_linkinfo;
};
typedef struct Cinterpreter_s Cinterpreter_t;

ci_nametype_t 
ci_standard_getaddr PROTO((const char *name, taddr_t *p_addr));

bool
ci_create_interpreter PROTO((Cinterpreter_t *ci, char **source_lines, 
	ci_getaddrproc_t getaddr_proc));

bool
ci_create_interpreter_from_file PROTO((Cinterpreter_t *ci, const char *source_file, 
	char **compiler_options, ci_getaddrproc_t getaddr_proc));

ci_exec_result_t
ci_execute_interpreter PROTO((Cinterpreter_t *ci, bool initialise_static_data, 
	ci_indirect_call_proc_t call_proc));

void
ci_destroy_interpreter PROTO((Cinterpreter_t *ci));

#endif

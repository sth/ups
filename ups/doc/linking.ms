.\" @(#)linking.ms	1.1 22/12/93 (UKC)
.\" -------------------------------------------------------
.\" .bX - print the argument in a fixed width bold font
.de bX
\\$3\&\f(CB\\$1\fP\\$2
..
.\" -------------------------------------------------------
.\" .fX - print the argument in a fixed width font
.de fX
\\$3\&\f(CR\\$1\fP\\$2
..
.\" -------------------------------------------------------
.\" .Fn - print a function name
.de Fn
\\$3\&\f(CR\\$1\fP()\\$2
..
.\" -------------------------------------------------------
.\" .Vs - start example
.de Vs
.in +4m
.ft CR
.nf
..
.\" -------------------------------------------------------
.\" .Ve - end example
.de Ve
.fi
.ft P
.in -4m
.sp 0.3v
..
.\" -------------------------------------------------------
.de En
.IP "\f(CR\\$1\fP \fI\\$2\fP"
..
.\" -------------------------------------------------------
.TL
C interpreter linking and relocation
.AU
Mark Russell
.AI
University of Kent
.NH 1
Introduction
.LP
This document describes the linking and relocation scheme used by the
ups C interpreter.
Read it if you have to maintain or extend the code.
.LP
We first give a brief overview of the system, and then a detailed
description of each step of the compilation, linking, loading and
execution process.
.LP
You will need a copy of the ups source to hand as you read this document.
.NH 1
Overview
.LP
C interpreter linking and relocation is more complex than usual because
we do not build a monolithic linked output file.  Instead the output file
contains references to the .ocx files that were used in the link.  The .ocx
files are read on demand at run time.  A .ocx file is read when a function
defined in it is called.  Some .ocx files may never be loaded on a given
run.
.LP
Data that can only be referenced by functions in a .ocx file is stored there.
This is to reduce the size of the linked cx file.  Relocation on this data
is done at .ocx load time; the relocation information is also stored in
the .ocx file.  In this description we call this data `private'.
.LP
Data that can potentially be referenced by functions outside the .ocx file
is stored in the linked cx file.  It also appears in the .ocx file, but only
so that the linker can copy in to the cx file.  We call this data `public'.
.LP
You might think that variables declared with the C keyword `static' would
always be private, but this is not so.  If the address of such a variable
is used to initialise a non-static variable then the static variable could
be referenced via the non-static one (and not by name).  This effect can
propogate - consider a statically initiailised linked list with all but the
first element declared static.
.LP
As there is no monolithic text area there is more to a function call
than jumping to a text offset.
Instead a function address consists of an index into a table of files
and an offset into the per-file text area.
There is a fair amount of book-keeping involved in dealing with
external function calls.
.NH 1
Phases of compilation
.LP
There are three phases between source text and an executing program:
.IP Compilation
This consists of parsing the source (.c) file and generating code.
The output is written to an object file with the suffix .ocx.
This contains the generated text and initialised data, along with
relocation, linking and optionally debugging information.
.IP Linking
In this phase a specified list of .ocx files is linked to form a
C interpreter `binary'.
This involves resolving external function and data references.
The .ocx file list can include libraries, which are simply
multiple .ocx files concatenated together.
.IP
The result of a successful link appears to the user like an executable,
but in fact it uses the `#!' mechanism to run the loader binary.
The file contains a list of the .ocx file pathnames, and the relocation
information needed to correctly load them as necessary.
.IP Loading
When the user invokes a C interpreter binary the loader is invoked via
the kernel `#!' mechanism.
The loader driver allocates some space for the stack, then reads the
cx.out file.
It then jumps to the entry point (specified in the header).
The text for the initial function is stored in the cx.out file; it
sets up argv and calls main.
.LP
The rest of this document describes this process in detail.
.NH 1
The
.bX text_t
structure
.LP
.Fn compile
takes a pointer to a parse tree for a single .c file and generates
code, data and relocation information.
All this information is stored in the text descriptor
.fX text_t ,
defined in
.fX ci_compile.h .
The fields in a
.fX text_t
include the following:
.En "textword_t *tx_text"
A pointer to the start of the generated code.
The code may contain unresolved references to external functions and data,
as well as unrelocated references to data.
.En "reloc_t *tx_var_relocs"
A list of relocations needed because of references to variables.
Added to by
.Fn ci_add_var_reloc
and processed by
.Fn make_relocs .
.En "reloc_t *tx_string_relocs"
Relocations needed because of references to string literals.
Added to by
.Fn ci_add_string_reloc
and processed by
.Fn make_relocs .
.En "funcreloc_t *tx_func_relocs"
Relocations needed for pointers to functions.
These are needed because a function pointer consists of a 16 bit file
index and a 16 bit function number, and the file indices are not known
until link time.
.En "var_t *tx_varlist"
A list of file-scope variables generated by the parse.
This includes entries generated by
.fX extern
declarations.
.En "vlist_t *tx_vlist"
A list of non-automatic variables that have been referenced.
Unlink
.fX tx_varlist ,
this list includes local static variables and excludes variables declared
.fX extern
that are never referenced.
.NH
Compilation steps
.LP
.Fn ci_compile_for_linking
takes a handle on a parse tree (a
.fX parse_id_t )
and returns a
.fX linkinfo_t
describing the text, data and relocation and symbol table information
needed to write a .ocx file..
.LP
The compilation steps performed by
.Fn ci_compile_for_linking
are:
.IP \(bu
Compile the parse tree, by calling
.Fn compile .
This produces the text area (pointed to by
.fX tx_text )
and a list of relocations in the text area
.fX tx_var_relocs , (
.fX tx_string_relocs ,
and
.fX tx_func_relocs ).
This also adds local static variables to
.fX tx_vlist ,
and adds any static initialisations to
.fX tx_initlist .
.IP \(bu
Set the
.fX VA_EXT_RELOC
(external relocation needed)
flag for any variables referenced in initialisers for variables with
external linkage (this effect can propogate).
.IP \(bu
Assign addresses for all non-automatic variables defined (not just declared)
in this file.
A variable can have internal or external linkage
.fX VA_EXT_RELOC ) (
and go in either data or bss
.fX VA_HAS_INITIALISER ) (
giving four possible locations.
As part of the address assignment we calculate the sizes of the four areas.
.IP \(bu
Add space to the internal and external data areas for string initialisers and
strings referenced from text.
.IP \(bu
Allocate and zero space for the internal and external data areas, then
perform the static initialisations listed in
.fX tx_initlist .
This can add variable, string and function relocations in the internal
or external data area.
.IP \(bu
Allocate the
.fX linkinfo_t
structure that is the output of
.Fn ci_compile_for_linking .
Set the
.fX linkinfo_t
pointers and sizes for the text and internal and external data areas,
and the sizes for the internal and external bss areas.
.IP \(bu
Call
.Fn make_extvars
to build the lists of defined and undefined variables, and adjust the
addresses of internal static bss variables (adding
.fX li_data_size
as internal bss follows internal data).
.IP
Undefined variables go on the
.fX li_undef_vars
list, with
.fX ln_addr
set to zero.
.fX va_addr
is set to the index into the list (this only becomes correct when the list
is reversed later).
.IP
Defined variables go on the
.fX li_vars
list, with a dummy name if the variable is really static but is referenced
by an external variable.
.fX ln_addr
is set to the value of
.fX va_addr ,
with the addresses of bss variables made relative to the
.I end
of external bss rather than the start.
Bss addresses are thus always negative - this is used by
.Fn cvtwords
to distinguish bss from data addresses.
Then
.fX va_addr
is set to the index into the
.fX li_vars
list, offset by the length of the
.fX li_undef_vars
list.
.IP \(bu
Call
.Fn make_relocs
to construct the
.fX linkinfo_t
relocation lists from the
.fX text_t
lists
.fX tx_var_relocs ,
.fX tx_string_relocs
and
.fX tx_func_relocs .
.IP
.fX make_relocs
builds five output lists.
Note that a
.fX reloc_pos_t
contains just a location,
whereas an
.fX ext_reloc_t
contains a location and an index.
.RS
.En "reloc_pos_t *li_extdata_funcrelocs"
References to function pointers (to functions in this file) in external data.
Only the location (relative to the start of external data) is needed, as
the function index is already stored in the data location.
These are resolved by
.Fn cvtwords
before any .ocx files are loaded.
.En "ext_reloc_t *li_extdata_relocs"
References to external variables in external data.
These are also resolved by
.fX cvtwords .
.En "ext_reloc_t *li_ext_relocs"
References to external variables in text or internal data.
The offset (if any) is stored at the location itself, and the relocation
entry contains the index of the variable (as placed in
.fX va_addr
by
.Fn make_extvars ).
These are resolved when a .ocx file is loaded by
.Fn load_text .
.En "reloc_pos_t *li_static_rps"
References to internal variables and strings in text or internal data.
Also resolved by
.Fn load_text .
.En "reloc_pos_t *li_static_funcrelocs"
References to function pointers (to functions in this file) in text or
internal data.
Resolved by
.Fn load_text .
.RE
We do some relocation resolution in
.Fn make_relocs ,
as we know the size of the text and data areas as well as the addresses of
internal variables.
This means, for example that we can store internal relocation as simply
an offset from the start of the text area, with no need to store which
variable is referred to.
We do not need to record whether an internal relocation is in the text or
data area - we just add the size of the text area to the location for
relocations in internal data.
.IP
.Fn make_relocs
also copies string initialisers into the internal or external data areas.
.IP \(bu
Call
.Fn make_funcinfo
to build the table of function addresses and the list of external functions.
The function address table contains offsets from the start of the text area,
and is ultimately used by the virtual machine to do a function call (function
pointers contain an index into this table).
.IP
The list of external functions is used later at link time to resolve
cross-file function calls.
.IP \(bu
Call
.Fn make_symfuncs
to build symbol table information describing functions.
This consists of the name, text address and block pointer of each function,
as well a table for each function mapping text addresses to source file line
numbers.
The block pointer points to the local variable and type information
as produced by the parse.
.IP \(bu
Call
.Fn make_symvars
to build a table of global variable names defined in this file.
This table is (currently) only used to quickly check if a name is defined
in a given source file before loading it to resolve a symbol.
.NH 1
\&.ocx files
.LP
The output of
.Fn ci_compile_for_linking
is a
.Fn linkinfo_t
structure.
This is normally written to a .ocx file, which is similar in function
to a compile .o file.
The structure of a .ocx file is described by the
.fX cx_header_t
structure (defined in
.fX xc_load.h ).
The .ocx file is just a file representation of the information in a
.Fn linkinfo_t .
.LP
\&.ocx files exist to support separate compilation.
They are produced when multiple .c files are compiled.
In subsequent compilations the .ocx file can be used instread of the .c
file.
In this case the
.fX linkinfo_t
structure is read from the file (by
.Fn ci_read_cx_file )
rather than built from source code by
.Fn ci_compile_file .
.LP
.Fn ci_read_cx_file
will read libraries as well as single .ocx files.
A library simply consists of multiple concatenated .ocx files (and indeed
is built with
.I cat (1)).
Libraries are handled just like individual .ocx files, except that a .ocx
file within a library is only loaded if it satisfies an existing
unresolved external variable or function reference.
.NH
Linking
.LP
Unless the user directs otherwise, the .c and .ocx files named on the
command line are linked into a single executable output file.
There are three steps:
.IP \(bu
.Fn ci_make_link_id
builds an
.fX olinkinfo_t .
This holds global link information such as the list of .ocx files,
and a list of defined and undefined external variables.
.IP
.Fn ci_make_link_id
inserts a reference to
.fX main ,
as well as definitions for the compiler builtins
.Fn exit ,
.Fn __cx_setjmp
and
.Fn __cx_longjmp .
.IP \(bu
Each
.fX linkinfo_t
(whether produced from a .c file, a .ocx file or a member of a library)
is passed to
.Fn link_file ,
which updates the global
.fX olinkinfo_t
link information with the link information in the
.fX linkinfo_t .
.IP \(bu
Finally,
.Fn ci_link
checks for unresolved or duplicate external names, and if all is
well writes the executable output file.
.NH 2
Processing
.bX linkinfo_t
structures
.LP
.Fn link_file
takes a
.fX linkinfo_t
and a
.fX cx_header_t
and builds a
.fX fileinfo_t
from it which is pushed onto the
.fX ol_files
list.
The
.fX cx_header_t
is a copy of the structure at the front of a .ocx file; it contains
data copied from the
.fX linkinfo_t
structure.
.LP
.Fn link_file
also updates the
.fX ol_extnames
list of defined and undefined external variables and functions.
.Fn link_file
makes deep copies of everything from the
.fX linkinfo_t
that must be preserved for the final link, because all the data
hanging off the
.fX linkinfo_t
is freed shortly after the
.Fn link_file
call.
.LP
The only
.fX fileinfo_t
fields which are not copied directly from the
.fX linkinfo_t
and
.fX cx_header_t
structures are
.fX fx_funcrefs ,
.fX fx_varrefs
and
.fX fx_vardefs .
These three fields are arrays of pointers to entries in the
.fX ol_extnames
list which are built as a result of doing name resolution.
.LP
First, undefined variables and functions
.fX li_undef_vars "" (
and
.fX li_undef_funcs )
are looked up.
If they not found then they are added to the
.fX ol_extnames
list as undefined.
The resulting pointer to an
.fX extname_t
is added to table of undefined functions and variables
.fX fx_funcrefs "" (
and
.fX fx_varrefs
respectively).
There is very rudimentary consistency checking, in that a function may
not satisfy a variable reference, and vice versa.
.LP
Variables and functions defined in this file
.fX li_vars "" (
and
.fX li_funcs )
are then added to the
.fX ol_extnames
list by calling
.Fn resolve_names .
This fills in the
.fX fx_vardefs
array of pointers into the
.fX ol_extnames
lists.
There is no need for a similar table of functions defined in this file,
[but I currently don't know why].
Functions and variables in the
.fX ol_extnames
list are indicated as undefined by having the
.fX en_file
field set to
.fX NULL ;
if we find such entries we set
.fX en_file
to point at the current
.fX fileinfo_t ,
thus marking the entry as defined.
If we find an entry with
.fX en_file
already set we complain about multiple definitions.
.LP
When all the files have been processed in this way the only
.fX ol_extnames
entries still undefined should be references to builtin functions and
variables (see below).
.NH 2
Writing the executable
.LP
Once all the input files have been passwd to
.Fn link_file ,
the compiler driver calls
.Fn ci_link
to resolve any undefined symbols and write the output file.
.LP
.Fn ci_link
first scans the
.fX ol_extnames
list for undefined symbols (entries with
.fX en_file
set to
.fX NULL ).
Such entries are looked up with the
.Fn get_libaddr
function supplied as an argument to
.Fn link_file .
If this lookup fails, we complain about the undefined symbol and do not
write an executable file.
Otherwise we set
.fX en_addr
to the address returned by
.Fn (*get_libaddr)
and point
.fX en_file
for the entry at a special
.fX fileinfo_t
.fX libfxbuf ) (
with
.fX fx_index
set to -1.
.LP
If all undefined symbols are successfully resolved we go on to write the
output file.
This consists of the following:
.IP \(bu
A header
.fX x_header_t
giving the sizes of the various segments, and some other stuff.
.IP \(bu
An array of .ocx file entries (one for each entry in the
.fX ol_files
list).
.IP \(bu
An array of all the indices into the external names tables.
This consists of the concatenation of the
.fX fx_funcrefs ,
.fX fx_varrefs
and
.fX fx_vardefs
entries for each entry in the
.fX ol_files
list.
.IP \(bu
All the external data (the concatenation of all the
.fX fx_extdata
areas).
.IP \(bu
All the external data relocations.
.IP \(bu
All the external function pointer relocations.
.IP \(bu
The
.fX ol_extnames
list.
Only the file index and address for each entry is saved; the names are
not needed.
.IP \(bu
Optional symbol table information: the concatenation of the
.fX fx_symfuncs
tables,
followed by the concatenation of the
.fX fx_symvars
tables.
.IP \(bu
The strings table.
Strings (such as .ocx file pathnames) are stored as offsets into this
table.
.LP
This file is read back in at execution time by
.Fn ci_load .

.tr *\(**
.TH MINSERT 3 "30 September 1987"
.SH NAME
Minsert \- obtain a menu descriptor for a compiled menu
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
Minsert(menu)
MENU *menu;
.ft P
.fi
.SH DESCRIPTION
The MENU data structure \fImenu\fP is inserted in the table of open menus
and a menu descriptor for it is returned.
The argument \fImenu\fP will usually be a pointer to a structure generated
by compiling an ordinary menu file with the C compiler.
The name of the MENU structure is set by editing the \fBMenu name\fP
field in the menu editor.
Compilation and \fIMinsert\fP provide an alternative to the run-time
loading of menus using \fIMopen\fP(3).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mopen(3), med3(1)
.SH DIAGNOSTICS
-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS

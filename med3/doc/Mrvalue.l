.tr *\(**
.TH MRVALUE 3 "30 September 1987"
.SH NAME
Mrvalue, Mrv_ascii, Mrv_layer \- construct return values
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
Mrvalue(layer,character)
int layer, character;

Mrv_ascii(rval)
int rval;

Mrv_layer(rval)
int rval;
.ft P
.fi
.SH DESCRIPTION
The first of these three macros can be used to construct an \fIMselect\fP(3)
return value from the ASCII character and \fIlayer\fP displayed by the menu editor
\fImed3\fP.
The other two macros are used to extract the ASCII part and \fIlayer\fP part
from a return value.
Since \fIMrvalue\fP is a macro, it will be a constant if its arguments
are constants and can therefore be used as part of a case statement.
For example:
.RS
.sp 0.5
.ft PO
case Mrvalue(1,'q') :  /* Quit now */
.ft P
.sp 0.5
.RE
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mopen(3), Mremove(3), Mdisplay(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS

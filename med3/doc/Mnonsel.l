.tr *\(**
.TH MNONSEL 3 "30 September 1987"
.SH NAME
Mnonsel \- set specified menu fields to be non-selectable
.SH SYNOPSIS
.nf
.ft B
Mnonsel(mdesc,rvstring)
int mdesc;
char *rvstring;
.ft P
.fi
.SH DESCRIPTION
Those menu buttons whose return values are in the string \fIrvstring\fP are
made temporarily unselectable and those
menu buttons which where disabled before the call but which are not
included in the string are enabled.
\fIRvstring\fP should be a NULL terminated string.
After the call, a menu button which conceals a hidden submenu will be
selectable if any of the hidden options are selectable.
.LP
The captions of unselectable buttons will be displayed faded.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mfonts(3).
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS
The way of specifying a list of menu buttons is far from not ideal.

.tr *\(**
.TH MOPEN 3 "30 September 1987"
.SH NAME
Mopen \- open a menu file
.SH SYNOPSIS
.nf
.ft B
Mopen(name)
char *name;
.ft P
.fi
.SH DESCRIPTION
\fIMopen\fP opens a menu file and returns a
\fImenu descriptor\fP which is then used as an argument to other functions
in the menu library.
\fIName\fP should be the pathname of a menu file
as created by \fImed3\fP(1).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
med3(1), Mclose(3), Mdisplay(3), Msize(3), Mplace(3)
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS

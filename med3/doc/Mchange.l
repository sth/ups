.tr *\(**
.TH MCHANGE 3 "30 September 1987"
.SH NAME
Mchange \- dynamically change menu captions
.SH SYNOPSIS
.nf
.ft B
Mchange(mdesc,oldrv,newrv,newcap,fontno)
int mdesc, oldrv, newrv, fontno;
char *newcap;
.ft P
.fi
.SH DESCRIPTION
In the menu with descriptor \fImdesc\fP, any button with return value equal
to \fIoldrv\fP will have its caption, return value and font number
set to the new values.
If \fInewrv\fP is zero then the return value will be left unchanged, if
\fInewcap\fP is NULL then the caption will be unchanged.
\fIFontno\fP should be between 0 and 4 with a value of 0 leaving the
menu's font numbers unchanged.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
med3.1, Mdisplay.3
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise the number of
matching return values will be returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS

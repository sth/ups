.tr *\(**
.TH MPERROR 3 "30 September 1987"
.SH NAME
Mperror \- print a menu package error message
.SH SYNOPSIS
.nf
.ft B
Mperror(string)
char *string;
.ft P
.fi
.SH DESCRIPTION
\fIMperror\fP prints \fIstring\fP followed by a colon, a space and a menu
package error message, to the standard error output.
The error message is determined by the value of the external variable
\fImenerr\fP and so \fIMperror\fP should only be called after another
menu function has returned \-1.
.SH FILES
/usr/local/lib/libMen3.a
.br
/dev/err
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
menus(3)
.SH DIAGNOSTICS
\-1 is returned if the message cannot be output.
.SH BUGS

.tr *\(**
.TH MCLEAR 3 "30 September 1987"
.SH NAME
Mclear \- remove any highlighting from a displayed menu
.SH SYNOPSIS
.nf
.ft B
Mclear(mdesc)
int mdesc;
.ft P
.fi
.SH DESCRIPTION
\fIMclear\fP removes any highlighting from a displayed menu and also
removes pop-up submenus.
.PP
When \fIMselect\fP returns a non zero value to say that a final menu
selection has been made, it leaves the selected box highlighted in order
to give some feedback to the user. 
When the action initiated by the menu selection is complete,
\fIMclear\fP should be used to remove the feedback as well as any
\fIselect-on-drag\fP submenus.
The different sorts of highlighting available are described in
\fIMfmodes\fP(3).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mselect(3), Mfmodes(3), Mreset(3).
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for a failure ( see \fImenus\fP(3) ).
.SH BUGS

.tr *\(**
.TH MSELECT 3 "30 September 1987"
.SH NAME
Mselect \- use a pair of coordinates to make a selection from a displayed menu
.SH SYNOPSIS
.nf
.ft B
#include <menu3.h>
Mselect(x,y,wdesc,mode,ignore)
int x, y, mode, ignore;
\fIWINDOW\fP wdesc;
.ft R
.fi
.SH DESCRIPTION
The currently displayed menus are searched to determine whether the window
position specified by \fIx\fP and \fIy\fP lies within any of them.
If it does then that menu display is updated and the selected value and
menu descriptor, if any, are returned.
.PP
The third argument is used to identify the window in which the coordinates
lie and should be the same as the \fIwdesc\fP argument passed to \fIMdisplay\fP(3).
.PP
Menu descriptors corresponding to set bits in \fIignore\fP, will be
ignored by \fIMselect\fP.
Hence, to select from all the displayed menus except the one with
menu descriptor \fImd\fP, one would use:
.RS
.sp 0.5
.ft PO
ignore = 1 << md;
.ft P
.sp 0.5
.RE
and to ignore all menus except the one with menu descriptor, use:
.RS
.sp 0.5
.ft PO
ignore = ~(1 << md);
.ft P
.sp 0.5
.RE
.PP
The \fImode\fP variable is used to control the way the function behaves
and the sort of feedback given.
If menu selections are to be made using the left mouse button and the menus are
being operated with the usual \fIfeedback-on-button-press\fP,
\fIaction-on-button-release\fP, then \fIMselect\fP should be called
with mode equal to:
.IP MS_PRESSED
When the left mouse button is pressed.
.ne 3
.IP MS_CONTINUE
Repeatedly while the left mouse button is held down.
.IP MS_RELEASED
When the left mouse button is released.
.PP
When \fIMselect\fP is called in this way submenus will be displayed and
removed as needed and menu buttons will be highlighted to give feedback.
Note that if the highlighting is to be updated correctly, the calls to
\fIMselect\fP should be made whether or not the coordinates are in the
menu.
.PP
The value returned by \fIMselect\fP depends on the coordinates and, for
the second two modes, the history of previous calls, but most programs
will only be interested in the value returned when the mode is
MS_RELEASED.
In this case a non zero return value means that a menu button has been
selected and the button's menu descriptor and return value
can be obtained using the macros MS_md and MS_rv defined in the header file.
In fact the value returned by \fIMselect\fP has the menu descriptor in
the upper 16 bits and the menu button return-value in the lower 16
bits.
.LP
The values returned by Mselect for the other values of \fImode\fP are
as follows:
.IP MS_PRESSED
If the coordinates are within a selectable menu button the menu descriptor
are returned as described for MS_RELEASED above.
Otherwise 0 is returned.
.IP MS_CONTINUE
A non zero value is returned if the coordinates have just moved into a
different button than at the last call with MS_PRESSED, but a button
within the same menu.
In this case, if the menu is operating in \fIselect-on-drag\fP mode
the effect will be the same as a button press and the return value
will also be the same.
If the menu is operating in \fIselect-on-press\fP mode the return value
will have a 1 in the lower 16 bits and the menu descriptor in the upper 16
bits.
.PP
For a description of the different sorts of feedback highlighting
available see \fIMfmodes\fP(3).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mdisplay(3), Mfmodes(3), Mclear(3), Mreset(3), Mmake(3).
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful.
The external integer variable \fImenerr\fP can be examined to find the
reason for the failure ( see \fImenus\fP(3) ).
.SH BUGS

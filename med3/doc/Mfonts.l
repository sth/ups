.tr *\(**
.TH MFONTS 3 "30 September 1987"
.SH NAME
Mfonts \- set the fonts used to display a menu's captions
.SH SYNOPSIS
.nf
.ft B
#include <wn.h>

Mfonts(mdesc,font1,font2,font3,font4)
int mdesc;
.br
font_t *font1, *font2, *font3, *font4;
.ft R
.fi
.SH DESCRIPTION
This function can be used to set the fonts which will be used to display
the menu's captions.
The one out of the four fonts which is used to display an individual caption
can be set using the menu editor \fImed3\fP or, if appropriate, the  
\fIfonts\fP argument to \fIMmake\fP(3).
If \fIMfonts\fP is not used to set the fonts then they default to the
menu package built-in standard font as returned by \fIMstdfont\fP(3).
.LP
The font arguments should be pointers to \fIwn\fP font variables. If the
menu package is not being used with \fIwn\fP then a native font can be
converted into a \fIwn\fP font by using the \fIwn\fP function \fIwn_install_font\fI
(see the \fIwn\fP reference below).
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
J D Bovey and M T Russell \fIUsing the wn graphics library\fP
.br
Mopen(3), Mdisplay(3), Mmake(3), Med3(1).
.SH DIAGNOSTICS
\-1 is returned if the call is unsuccessful, otherwise 0 is returned.
The external integer variable \fImenerr\fP can be examined to find the
reason for failure ( see \fImenus\fP(3) ).
.SH BUGS

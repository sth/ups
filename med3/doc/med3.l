.tr *\(**
.TH MED3 1 "30 September 1987"
.SH NAME
med3 \- menu editor
.SH SYNOPSIS
.nf
\fBmed3\fP [ menu-name ]
.fi
.SH DESCRIPTION
The \fImed3\fP menu editor is used by the programmer 
to generate and modify menus.
It is one major component of the menu package, the other being
the library of interface routines which enable user programs to
use the menus created by \fImed3\fP.
.PP
New menus are created by starting with a blank rectangle of the
required size and then successively subdividing it, either vertically
or horizontally, until the required menu is built.
Subdividing a field
creates two sub-fields, which need not be of equal size.
Thus if the end
requirement is three equal sub-fields the original field is first divided
in a 2 to 1 ratio and the bigger sub-field is sub-divided in two.
.SH Command Syntax
.LP
\fIMed3\fP uses the concept of a currently selected menu button,
referred to in the rest of this document as the \fIcurrent button\fP
The current button is always the button whose caption is selected
for editing, and
any command which acts on a menu button will use the current button.
If a command needs other information than a menu button (for example
the position of the button's submenu) then \fImed3\fP switches into a suitable
mode so that the extra information can be entered after the command.
.LP
The \fImed3\fP command syntax is, therefore, basically infix.
The order of operations is:
.IP (i)
Select a button from the edited menu.
.IP (ii)
Select a command from the command menu.
.IP (iii)
Enter any other graphical information.
.LP
For most commands either (i) or (ii) is not needed but all commands except
\fBread\fP, \fBwrite\fP and \fBquit\fP need one or the other.
All the available commands are listed and explained below.
.SH The Mouse Buttons
.LP
\fIMed3\fP tries to use the mouse in a consistent way:
.TP
.B The Middle Button
Used to select text for editing and also to select the current menu button.
Among the text which can be edited are the four font filenames, the menu
filename and menu captions.
The current button (see above) is always the one whose caption is
selected for editing.
This use of the middle button is completely independent of which mode
\fIMed3\fP is in.
.TP
.B The Right Hand Button
Only used for returning to command mode.
.TP
.B The Left Hand Button
Everything else.
As well as being used to select commands and
button attributes this button is used for moving about in the edited menu.
Selecting a button will cause its submenu to be displayed whereas selecting
a point outside the menu will remove any submenus which are visible. 
.SH The File and Menu Names
These two editable fields are beneath the list of fonts.
The contents of the \fImenu file\fP field is used as the file name when a
menu is read or written.
The menu files created by the editor are C source files and the
contents of the \fImenu name\fP field, which can be any legal C identifier,
is used as the name of the MENU data structure in the file.
If the menu is to be loaded with \fIMopen\fP(3) then the menu name
will not be needed, but if the menu is to be compiled and installed
using \fIMinsert\fP(3) then the menu name will be needed as an argument
to \fIMinsert\fP.
.SH Menu Button Attributes
.LP
Each button in the edited menu has a number of invisible attributes like the 
\fIreturn value\fP and \fIfont\fP which are displayed and controlled
using the square buttons in the area above the main editing area.
.TP
.B fonts
Each menu is allowed four fonts with one per caption and the box representing
the font used for the current button is highlighted.
The current button's font is changed simply by pointing at the
box representing the new font.
Font files can be loaded into the editor by editing the font name and
typing RETURN.
Fonts loaded this way are used to see what the menu
would look like if they were used for its captions;
the fonts used when the menu is actually used need to be set using the
\fIMfonts\fP(3) library function.
.IP
.B Note.
The menu package now has a built in font which is the default default font
used by both the editor and the library.
.TP
.B Return values
Each menu button has a \fIreturn value\fP which is the value which will be
returned by \fIMselect\fP(3) when the button is selected by the user.
The return values can be chosen from the set of 256 values,
chosen by selecting one of the
64 ASCII values displayed in the grid and one of the the four layer buttons.
For most applications 64 return values will be more than enough and
the default layer will not need to be changed.
A return value can be constructed from a layer and an ASCII character
by using the \fIMrvalue\fP(3) macro defined in the header file.
As with fonts the return value of the current button is highlighted and
can be changed simply by selecting a different one.
Although there is nothing to prevent the same return value being used for more
than one button, to help in selecting non-clashing values,
the cells representing those return values currently in use are marked by
a small black square in the upper left hand corner.
.ne 4
.TP
.B Submenu discipline
These are used to display and set a number of flags which effect the way the
menu will behave when used.
The alternatives available are:
.TP
.B Press \- Drag
By default a submenu will operate in \fIselect on press\fP mode \-
In order to select a submenu button the mouse button has to be
pressed and released with the cursor in the same button.
If a menu button has the \fBDrag\fP option set then it any submenus
it has will act in \fIselect on drag\fP mode \-
a submenu button can be selected by dragging the cursor into it
with the mouse button held down and then releasing the mouse button.
.TP
.B Resize \- Fixed
This option is used to determine whether will be scaled in proportion
or left the same size when the main menu has its size changed by
\fIMsize\fP(3).
.TP
.B Static \- C/Rel
By default a submenu will appear at a fixed position relative to the
main menu \-
Selecting \fBC/Rel\fP causes the submenu to appear positioned relative
to the cursor coordinates when the mouse button is pressed.
When used, a cursor relative submenu will appear positioned such that,
relative to the submenu, the cursor is in the same place as
the bottom left hand corner of its parent button when the menu
is displayed by the editor.
.TP
.B Line colour
These two buttons are used to set the line colour of the division lines
of the buttons submenu.
At present the colours available are Foreground and Background (i.e. invisible)
To set the line colour of the main menu you should cover the whole menu
with a button, set the button's line colour and then remove it again with
\fBuncover\fP.
This last way of setting line colour works only with the main menu.
.SH The Commands.
.TP
.B quit
Exit from the editor
.TP
.B write
Write out the currently edited file to a menu file.
The file name used is the editable string headed \fBMenu file:\fP.
.TP
.B new
Create the outline box for a new menu.
The edited menu will be in the top left hand corner of the main editing
area and the user is prompted for the lower right hand corner.
It is possible to position the corner fairly precisely by pressing
the left hand mouse button
with the cursor near the desired position and then dragging the corner
into the correct position.
If the \fBnew\fP command is aborted before the left button is pressed then the
previous menu will be restored.
.TP
.B read
Read in an existing menu file in order to edit it.
The file name used is the editable string headed \fBMenu file:\fP.
.TP
.B cut
Remove the currently selected button and its submenu from the menu.
Buttons which have been cut from the menu are saved in a stack and can
be pasted back into the same or another menu.
.IP
Although the \fBcut\fP command is only applied to individual buttons
it can be used on a rectangular group of buttons by first using
the \fBcover\fP command to convert them into a submenu.
If it is pasted back in, the covering button can be removed with
\fBuncover\fP to reveal the group of buttons in its new position.
.TP
.B paste
Replace the current button by the most recently \fBcut\fP button.
The caption, return value and any submenu of the current button
are irrevocably destroyed.
.TP
.B equal
This command is used to adjust a sequence of adjacent rectangles and
make them all the same size.
After selecting the command, the cursor should be used to drag a rubber box
surrounding the rectangles which are to be made equal sized.
While the box is being dragged, the group of rectangles which will be
adjusted are highlighted by shading them grey.
The rectangles to be adjusted will be the sequence which makes up the
whole of the highlighted area and so it is necessary to position
the rubber box round the desired rectangles but no others.
.TP
.B move
Pick up and move the lines used to subdivide the menu.
If the cursor is positioned over the line to be moved it can be picked
up using the left mouse button and dragged to its new position.
The line under the cursor is highlighted by removing it from the display.
.TP
.B cover
Create a new button covering a rectangular area of the menu.
The area to be covered is delimited by positioning a rubber box round
it and the area which will be covered is highlighted by shading it grey,
as with the \fBequal\fP command.
The covered area becomes a submenu of the new button and so this command
provides a way of creating submenus, but its main use is in conjunction
with \fBcut\fP, \fBpaste\fP and \fBdivide\fP.
.TP
.B uncover
Remove a covering button.
This command is only meaningful if the current button was created using
the \fBcover\fP command.
.TP
.B divide
This is the command used to subdivide the menu and hence create new menu
buttons.
New subdivisions are inserted in the current button and are positioned
using the left mouse button.
The direction of the division, vertical or horizontal, is determined by the
position of the cursor within the current button when the mouse button
is pressed but the new division can be dragged into its desired position
with the mouse button held down.
.IP
There is a change from earlier versions of \fImed\fP in that the two
buttons created by the subdivision have equal status and dividing a
button which has a caption does not create a submenu.
The current button, along with any attributes, caption and submenu, will
be transferred to the new button which was largest when the mouse button was
pressed.
.IP
The \fBdivide\fP command departs from the usual \fImed3\fP syntax in that
the current button can be selected (with middle mouse button) after selecting
the command and a new current button can be selected between inserting
divisions.
.TP
.B submenu
This command has two sub-options \fBbound\fP and \fBfree\fP but they are
different only in the  positioning of the submenu.
A \fIbound\fP submenu has to be positioned to coincide with a rectangular
group of buttons from the existing menu and is positioned by drawing
a rubber box round the desired area.
A \fIfree\fP submenu can be positioned anywhere within the editing area and
is positioned using a rubber box.
.SH FILES
/usr/local/lib/libMen3.a
.SH SEE ALSO
J D Bovey \fIA tool for providing programs with menus\fP
.br
Mclear.3, Mclose.3, Mdisplay.3, Mdup.3, Menus.3, Mfmodes.3, Mnewcap.3,
Mopen.3, Mperror.3, Mplace.3, Mpushsu.3, Mredraw.3, 
Mremove.3, Mreset.3, Mselect.3, Msize.3, Msopen.3
.SH DIAGNOSTICS
.SH BUGS

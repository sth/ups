/* ui_menu.c - menu display and input handling */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)ui_menu.c	1.27 9/4/95 (UKC) */
char ups_ui_menu_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/menu3.h>

#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>
#include "objtypes.h"
#include "obj_util.h"
#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "srcwin.h"
#include "ui.h"
#include "ui_priv.h"
#include "exec.h"
#include "obj_target.h"
#include "obj_bpt.h"
#include "reg.h"
#include "tdr.h"
#include "state.h"
#include "menudata.h"
#include "cursors.h"

static void set_cur_objtype PROTO((int type));
static void sync_dynamic_menu_with_cur_objtype PROTO((void));
static void dump_all_objects PROTO((void));
static void dump_selected_objects PROTO((void));
static int show_in_outwin PROTO((char *arg, int level, char *line));
static void null_ofunc PROTO((const char *s));


static void
null_ofunc(s)
const char *s;
{
}

typedef struct newselst {
	objid_t ns_obj;
	struct newselst *ns_next;
} newsel_t;

static newsel_t *New_selection = NULL;
static enum {NO_RAISE, RAISE_ON_BREAK, LOWER_ON_RUN, ICONIFY_ON_RUN} raise_on_break = NO_RAISE;
static int log_messages = FALSE;
/* static Outwin* lower_win = NULL; */
static unsigned lower_time0 = 1500000;
static unsigned lower_time = 1500000;

static const char *help_level_00[]=	/* saving state */
{
"                      SAVING STATE\n",
"\n",
"     When you quit ups, it normally forgets  things  like  break-\n",
"     point  locations  and  the way your variables are displayed.\n",
"     This can be a nuisance, especially if you have added  inter-\n",
"     preted  code,  or  typed  long  expressions into the display\n",
"     area.  To preserve these settings, ups will optionally  save\n",
"     state information when you quit it, and reload this informa-\n",
"     tion when you start it again.\n",
"\n",
"     In addition, a state  file  can  be  explicitly  created  or\n",
"     loaded  at  any time by selecting the target name at the top\n",
"     of the display region. This will create a  menu  with  three\n",
"     captions:  Write  target  core,  Save state, and Load state.\n",
"     The Write target core prompts you for a file name  in  which\n",
"     ups  will  save  the  current  image of the target as a core\n",
"     file.  Selecting Save state prompts you for a file  name  in\n",
"     which  to  save  the  current debugger state. Selecting Load\n",
"     state asks for a state file name which will be then loaded.\n",
"\n",
"     This section describes how this state is saved and restored.\n",
"\n",
"     If the directory ups-state exists in the current  directory,\n",
"     ups will use it to store state information between sessions.\n",
"     This includes  breakpoint  locations  (and  the  interpreted\n",
"     code, if any), and the state of the variables display.\n",
"\n",
"     The saved state is used in several ways:\n",
"\n",
"     o    After starting ups, you can  select  Restore  from  the\n",
"          Breakpoints  menu  to put breakpoints back as they were\n",
"          from the previous session. Ups tries to put breakpoints\n",
"          back  in  the  right  places, but it can be defeated by\n",
"          major changes to the source code.\n",
"\n",
"     o    The default for the Expand option for stack  trace  and\n",
"          source  file  entries  is Like before.  This adds vari-\n",
"          ables as they were in the last time you looked at them.\n",
"          If  the  ups-state  directory  exists, the state of the\n",
"          variables display is remembered  across  different  ups\n",
"          sessions.\n",
"\n",
"     o    When you add a variable the display format (hex,  octal\n",
"          etc)  is  taken  from  the  way  it  was  last time you\n",
"          displayed the variable.\n",
"\n",
"     o    The attributes for signal handling are restored  (i.e.,\n",
"          whether accepted, ignored, cause the process to stop).\n",
"\n",
"     State is saved to the file ups-state/xxx.state, where xxx is\n",
"     the  last  component  of the path of the file you are debug-\n",
"     ging.  You can also  create  the  file  ups-state/xxx.config\n",
"     (perhaps  by copying ups-state/xxx.state).  The .config file\n",
"     is read at startup by ups, but not  written.   Also,  break-\n",
"     points  in  the  .config  file are automatically restored on\n",
"     startup.\n",
"\n",
"     In future releases you will be able to use the .config  file\n",
"     to  add  environment  variable settings, etc.  Currently the\n",
"     only directive (other than  things  specifying  breakpoints,\n",
"     saved variable and signal state) is auto-start which takes a\n",
"     single argument yes or no.  The line:\n",
"\n",
"             auto-start yes\n",
"\n",
"     in the xxx.config file means start  the  target  running  as\n",
"     soon as ups has started up.\n",
"\n",
"     Ups also looks for saved state in the file $HOME/.upsrc  and\n",
"     .upsrc  (in  the  current  directory).  Thus the full set of\n",
"     files is:\n",
"\n",
"         $HOME/.upsrc\n",
"         .upsrc\n",
"         ups-state/xxx.config\n",
"         ups-state/xxx.state\n",
"     Files later in the sequence can override earlier settings.\n",
0,
};

static const char *help_level_0A[]=	/* Web site */
{
"     The URL for the ups web site is:\n",
"\n",
"           http://ups.sourceforge.net/\n",
"\n",
"     It is maintained by Ian Edwards (ian@concerto.demon.co.uk).\n",
"     It includes a FAQ,  html man pages, site listings where ups\n",
"     can be found, supported architectures, a history of changes\n",  
"     between versions, and other information.\n",
0,
};

static const char *help_level_0B[]=	/* cut and paste */
{
"                     CUT AND PASTE\n",
"\n",
"     You can select text with highlighting by pressing  the  left\n",
"     mouse  button and dragging.  Releasing the left mouse button\n",
"     sets the X selection.\n",
"\n",
"     You can paste text into an edit with control-Y or by  click-\n",
"     ing  the  middle mouse button on the Enter Button (the small\n",
"     region to the right of the typing line with the \"<<\" image).\n",
"\n",
"     Another useful trick is to define  paste  strings  by  using\n",
"     control characters in a custom menu: e.g. define\n",
"\n",
"          setenv UPS_F1_STR \"^e^u^y\"\n",
"\n",
"          setenv UPS_F2_STR \"^y\"\n",
"\n",
"     then the right mouse button  will invoke a menu:  the  first\n",
"     item clears the current text no matter where the text cursor\n",
"     is, and does a paste; the second just does a  paste  at  the\n",
"     current  location.  See the section on custom menus for more\n",
"     details. \n",
"\n",
"     In the source window there are some extra shortcuts:\n",
"\n",
"     o    Pressing and releasing the left mouse  button  (without\n",
"          dragging)  adds  a  variable  name to the display as in\n",
"          previous versions of ups.  Only if you move  the  mouse\n",
"          to  a  different character with the left button down do\n",
"          you get a plain X selection.\n",
"\n",
"     o    Doing a press-left-and-drag selection  with  the  SHIFT\n",
"          key  pressed  automatically pastes the selected text as\n",
"          an expression into the appropriate place in  the  stack\n",
"          trace.   It  is  equivalent  to  selecting  some  text,\n",
"          selecting `add expr' for the appropriate entry  in  the\n",
"          stack  trace, pressing ^Y to paste the text and hitting\n",
"          RETURN.\n",
"\n",
"     o    If you hold the shift key down, the press  and  release\n",
"          the  left  mouse  button  without moving the mouse, ups\n",
"          adds the expression under  the  mouse  to  the  display\n",
"          area.   It makes a reasonable attempt to select what to\n",
"          display.  Try it out to see what I mean.\n",
"\n",
"     In the display  window,  left  button  selects  objects  for\n",
"     operations,  and  one can pan vertically to select groups of\n",
"     objects. Selecting objects for some  operation  is  distinct\n",
"     from  making an X text selection.  However, while the button\n",
"     is down, if the horizontal distance from the original  click\n",
"     exceeds  a  certain  value, the window shifts from selecting\n",
"     objects to making an text selection. So one can easily  just\n",
"     pan  right to select a string for instance.  The pixel value\n",
"     is 30  by  defaults,  but  the  SelectionThreshold  resource\n",
"     change be used to change it.\n",
0,
};

static const char *help_level_1A[]=	/* in source */
{
"                     BREAKPOINTS IN SOURCE\n",
"\n",
"    o Move the mouse over the highlighted source line, press  and\n",
"     hold down the right hand mouse button then release it.\n",
"\n",
"          When you pressed the mouse button you will have seen  a\n",
"          popup menu with the captions Add breakpoint, Execute to\n",
"          here, Jump to here, and Edit  source.   You  will  also\n",
"          have  seen an arrow to the left of the menu pointing at\n",
"          the source line you pressed the mouse over.\n",
"\n",
"          When you release the mouse button a breakpoint is added\n",
"          just  before  the  source  line.  You will see the text\n",
"          \"#stop;\" appear.\n",
"\n",
"          This is the simplest and  most  common  way  of  adding\n",
"          breakpoints in ups.  The normal sequence of actions is:\n",
"\n",
"          o    Type the name of the function you  are  interested\n",
"               in  (or  enough of it to uniquely identify it) and\n",
"               hit ESC (the escape key).  The source of the func-\n",
"               tion is displayed in the source window.\n",
"\n",
"          o    Scroll the source to make visible the  line  where\n",
"               you want to add a breakpoint.\n",
"\n",
"          o    Add a breakpoint by clicking the right mouse  but-\n",
"               ton over the source line.\n",
0,
};

static const char *help_level_1B[]=	/* by name */
{
"                     BREAKPOINTS BY NAME\n",
"\n",
"    o Move the mouse cursor over the Breakpoints caption  in  the\n",
"     upper region, and press and release the left mouse button.\n",
"\n",
"          You should  see  two  things  happen:  the  caption  is\n",
"          inverted  to  show  that  it  is  selected,  and a menu\n",
"          appears near the top of the window  with  the  captions\n",
"          Add new breakpoint and Remove all breakpoints.\n",
"\n",
"          All the objects in the display area (except  Functions)\n",
"          can  be  selected  like  this and have their own menus.\n",
"          Selecting an object (or many objects) and  clicking  on\n",
"          one  of the commands in its associated menu is the pri-\n",
"          mary way of issuing commands to ups.\n",
"\n",
"    o Click (press and release) the left mouse  button  over  the\n",
"     Add new breakpoint menu caption.\n",
"\n",
"          You should see a line  below  the  Breakpoints  caption\n",
"          looking like:\n",
"\n",
"              Function:|                    line:0\n",
"\n",
"          The vertical line is a marker bar - it indicates  where\n",
"          characters will appear when you type.\n",
"\n",
"    o Type the function name to break in  (input accepts wildcards\n",
"      and regular expression syntax).\n",
"\n",
"          You can use the delete key as you would expect  to  fix\n",
"          typos.   There are various other useful control charac-\n",
"          ters - see the EDITABLE FIELDS section for details.\n",
"\n",
"    o Press ESC (the escape key).\n",
"\n",
"          This confirms the edit.  You should see the text\n",
"\n",
"              #stop;\n",
"\n",
"          appear at the start of the function in the source region.\n",
"\n",
"          Ups represents breakpoints as this fragment  of  pseudo\n",
"          C.   You  can  edit breakpoints to do things other than\n",
"          just stop (e.g. call a target function or only stop  if\n",
"          a certain condition is true). \n",
"\n",
"     o    Typing \"%b function\" will enter  a  breakpoint  at  the\n",
"          entry to `function', the same as clicking on the break-\n",
"          point header, selecting \"add breakpoint\", and  entering\n",
"          the function name.\n",
"\n",
"     o    Typing \"%d address\" will dump 16 bytes of memory at the\n",
"          given  address  to  the  output window. See the section\n",
"          DUMPING MEMORY for more on the %d command.\n",
 0,
};

static const char *help_level_1C[]=	/* loading and saving breakpoints */
{
"                      LOADING AND SAVING BREAKPOINTS\n",
"\n",
"     You can explicitly load and save breakpoints to  files.   To\n",
"     save  breakpoints,  select  one or more in the display area,\n",
"     then select `save' from the menu.  You will be prompted  for\n",
"     a  file  name.  If the file already exists you will be asked\n",
"     whether you want to cancel the save, overwrite the  file  or\n",
"     append to it.\n",
"\n",
"     Saved breakpoints can be reloaded by selecting Load from the\n",
"     Breakpoints header menu.\n",
0,
};

static const char *help_level_1D[]=	/* C++ syntax */
{
"                     BREAKPOINTS IN C++\n",
"\n",
"   o When breakpoints are set on overloaded methods in C++  code,\n",
"     ups  queries  whether  to  place breakpoints on all matching\n",
"     names.\n",
"\n",
"     You can set breakpoints on \"cout\" statements in C++ code  in\n",
"     ups by breaking on the overloaded \"<<\" operator. The general\n",
"     form of setting breakpoints in the display window of ups is:\n",
" \n",
"         <filename>:function\n",
"\n",
"     The simplest way of breaking on cout statements in C++  code\n",
"     is:\n",
" \n",
"         ostream::operator<<\n",
 "\n",
"    o There is a useful trick for listing all methods on a class.\n",
"     For a class `obj', type in `obj::' then Shift-ESC, in either\n",
"     the top typing line or at a breakpoint line in  the  display\n",
"     window. This will list all strings that match, which in this\n",
"     case means all methods of the class.  If there is  only  one\n",
"     method,  which  may be overloaded, there will be no listing,\n",
"     but the source window will move to show the code, and in the\n",
"     case  of  entering  a  breakpoint  name,  the  name  will be\n",
"     expanded to its full form.  It is just an example  of  using\n",
"     the listing key as detailed later on in this document.\n",
0,
};

static const char *help_level_1E[]=	/* auto completion */
{
"                     BREAKPOINT NAME COMPLETION\n",
"\n",
"    o When typing in a breakpoint, pressing ESC does partial name\n",
"     completion  whenever  possible. So if a program has just two\n",
"     routines, `process_key()' and  `process_cmd()',  typing  `p'\n",
"     then ESC will expand the line to `process_' and in the third\n",
"     line will be a message like\n",
"\n",
"          `process_' matches `process_key' and `process_cmd'.\n",
"\n",
"     Then  typing  `k'  then  ESC  will  complete  the  line   to\n",
"     `process_key'\n",
0,
};

static const char *help_level_1F[]=	/* Shift-ESC listing */
{
"                     BREAKPOINTS - LISTING MATCHES\n",
"\n",
"     Pressing Shift-ESC, or Shift-RETURN when  setting  a  break-\n",
"     point lists matching functions in the output window.  So `*'\n",
"     matches all function names, and `file.c:*' matches all func-\n",
"     tion names for `file.c'.  ESC states how many matching func-\n",
"     tions there are, while holding the Shift key down lists them\n",
"     in  the  output  window.  To list all functions in a program\n",
"     (and there may be many thousands) enter `*' then Shift-ESC.\n",
"\n",
"     The full path names of source files are given  when  listing\n",
"     symbols with the Shift-ESC keys.\n",
0,
};

static const char *help_level_1G[]=	/* activation */
{
"                     BREAKPOINT ACTIVATION\n",
"\n",
"     By selecting a breakpoint, the Activate and Inactivate, cap-\n",
"     tions  control  whether  the  breakpoint is either active or\n",
"     inactive. Active code is executed normally, whereas inactive\n",
"     code is ignored. The activation state is set by the two cap-\n",
"     tions labeled Activate  and  Inactivate  that  appear  after\n",
"     selecting  the  breakpoint object. The current state appears\n",
"     to the right of the breakpoint line number.\n",
"\n",
"     By selecting the Breakpoints  header  object,  the  captions\n",
"     labeled  Enable and Disable allow global control of process-\n",
"     ing of breakpoints.  When globally enabled,  all  breakpoint\n",
"     code  is examined, and if the breakpoint is active, the code\n",
"     is executed. Conversely, when breakpoint  code  is  globally\n",
"     disabled,  no breakpoint code is executed, regardless of its\n",
"     activation state. The current enabled  state  is  implicitly\n",
"     shown  by  the  shaded  caption - so after pressing Disable,\n",
"     that caption is shaded and the Enable caption  becomes  nor-\n",
"     mal.\n",
"\n",
"     When globally  disabled  or  individually  inactivated,  the\n",
"     breakpoint code will still exist in the source, but the code\n",
"     will be ignored until re-enabled.\n",
"\n",
"     See the \"accelerators\" section for shortcuts on this topic.\n",
0,
};

static const char *help_level_1H[]=	/* execute */
{
"                     BREAKPOINT EXECUTION\n",
"\n",
"    o When a breakpoint is selected in the top window, there is a\n",
"     new  button  labeled  `execute'.  This button can be used to\n",
"     execute breakpoint code whenever the target  context  allows\n",
"     it.  The  most  common  use for this is to repeatedly call a\n",
"     function, such as the Purify API functions purify_describe()\n",
"     or  purify_all_leaks()  without  having to enter the call at\n",
"     every line in the source where it may be required.\n",
"\n",
"     It can also be used to bump variables that are used in expr-\n",
"     essions in the display window.\n",
0,
};

static const char *help_level_1I[]=	/* duplicates */
{
"                     BREAKPOINTS - DUPLICATES\n",
"\n",
"     Ups works with for multiple linked files.  For  such  files,\n",
"     at least two versions of the code exist in the target.\n",
"\n",
"     Ups makes all duplicate files visible, with automatic repli-\n",
"     cation  in the duplicate file of any setting or modification\n",
"     of a multiple linked function breakpoint.  In  this  manner,\n",
"     all  breakpoint  code  is  kept  identical between duplicate\n",
"     files, and all breakpoints should be honored.\n",
"\n",
"     What this means in practice when using UPS is that  multiple\n",
"     linked  function  files  are listed twice in the source file\n",
"     listing. Either one can be used to set a breakpoint,  and  a\n",
"     second  breakpoint  will  be  created  automatically  in the\n",
"     duplicate file.\n",
"\n",
"     The following actions maintain this automatic shadowing:\n",
"\n",
"          1. Selecting a file and setting a breakpoint  with  the\n",
"          right mouse button in the source window.\n",
"\n",
"          2. Setting a breakpoint by selecting the  `Breakpoints'\n",
"          header  and  then pressing the `add new bpt' button (or\n",
"          SHIFT left accelerator on the `Breakpoints' header).\n",
"\n",
"          3. Deleting a breakpoint.\n",
"\n",
"          4. Renaming a breakpoint  (works  for  all  transitions\n",
"          between single instance and duplicate functions).\n",
"\n",
"          5. Adding or modifying breakpoint code (code  typed  in\n",
"          at the breakpoint in the source file).\n",
"\n",
"          6. Changing the line number of a breakpoint.\n",
"\n",
"          7. Changing the activation state of a breakpoint.\n",
"\n",
"          8. Selecting a file and using the  right  mouse  button\n",
"          menu in the source window to `execute to here'.\n",
0,
};

static const char *help_level_1J[]=	/* conditional */
{
"                     CONDITIONAL BREAKPOINTS\n",
"\n",
"     The default breakpoint action - stopping  the  target  -  is\n",
"     represented as a fragment of pseudo C code.  This is the\n",
" \n",
"         #stop;\n",
"\n",
"     line that appears in the source region when you add a break-\n",
"     point.\n",
" \n",
"     You can change this to a fragment of  C  code,  editing  the\n",
"     text in the usual way by clicking the middle mouse button to\n",
"     position a marker bar.  You can use the RETURN key to  enter\n",
"     multi-line  code  fragments.  As with other editable fields,\n",
"     you end the edit with ESC.  If there  is  an  error  in  the\n",
"     code, an error message is given and the marker is positioned\n",
"     at the point of the error.\n",
" \n",
"     Note that a breakpoint will not stop the target  unless  the\n",
"     special  keyword #stop is executed.  You can use this to set\n",
"     conditional breakpoints, like:\n",
" \n",
"         if (i == 72)\n",
"              #stop;\n",
"\n",
"     or, to use a more sophisticated example:\n",
" \n",
"         if (strcmp(p->p_name, \"foo\") == 0)\n",
"              #stop;\n",
"\n",
"     You can declare your own variables in breakpoint code.  This\n",
"     is  useful  when  you want only want to stop at a breakpoint\n",
"     after it has been hit a given number of times.  A code frag-\n",
"     ment to do this would look something like:\n",
" \n",
"         {\n",
"              static int count = 0;\n",
"              if (++count == 74)\n",
"                   #stop;\n",
"         }\n",
"\n",
"     This would stop the target the 74th time the breakpoint  was\n",
"     encountered.   Static variables are reinitialized every time\n",
"     the target is started.  Automatic variables  are  uninitial-\n",
"     ized  and do not preserve their values between separate exe-\n",
"     cutions of breakpoint code.\n",
0,
};


static const char *help_level_2A[]=	/* accelerators */
{
"                     KEYBOARD ACCELERATORS\n",
"\n",
"     A SHIFT - left click or double left click does the most com-\n",
"     monly used actions for the object listed below:\n",
"\n",
"     env header\n",
"          Toggle between expanding and compressing  the  environ-\n",
"          ment display.\n",
"\n",
"     signal header\n",
"          Toggle between expanding  and  compressing  the  signal\n",
"          display.\n",
"\n",
"     source header\n",
"          Toggle between expanding  and  compressing  all  source\n",
"          files.  Files  are  expanded to `like before state'. If\n",
"          some files have  variables  or  expressions  displayed,\n",
"          such  files  will  remain  visible when the source file\n",
"          list is compressed. Use the menu item  \"completely\"  to\n",
"          remove all source files.\n",
"\n",
"     source files\n",
"          Toggle between expanding and compressing all the global\n",
"          variables of the file\n",
"\n",
"     file add expr\n",
"          See the actions under \"variable\".\n",
" \n",
"     function\n",
"          Toggle between expanding and compressing all the outer-\n",
"          most  local  variables. Variables are expanded to `like\n",
"          before state'.\n",
" \n",
"     function block\n",
"          Toggle between expanding and compressing all the  local\n",
"          variables of the block\n",
" \n",
"     func add expr\n",
"          See the actions under \"variable\".\n",
" \n",
"     variable\n",
"          If the variable is a struct or union, or pointer to   a\n",
"          struct or union, toggle between expanding and compress-\n",
"          ing the current level of the variable.  If the variable\n",
"          is  a  pointer  to  of  integer  type,  dereference the\n",
"          pointer one level.  If the variable is of integer type,\n",
"          or  fully  dereferenced  pointer  to a such a variable,\n",
"          toggle between unsigned hex and signed decimal formats.\n",
"          Note  that a double  click on a  variable in the source\n",
"          window will also alternately expand and compress it.\n",
" \n",
"      bpt header\n",
"          By default, toggle the global breakpoint  enable  flag.\n",
"          The BreakPointHeaderAcceleratorAction X resource may be\n",
"          set to request that ups prompt  for  a  new  breakpoint\n",
"          instead.\n",
" \n",
"     breakpoint\n",
"          By default, toggle the current breakpoint  between  the\n",
"          active  an  inactive  states.   The BreakPointAccelera-\n",
"          torAction X resource may be set  to  request  that  the\n",
"          breakpoint  be  removed instead.  Double clicking ( but\n",
"          not SHIFT-clicking) on the  breakpoint  in  the  source\n",
"          window  selects  has  the  same  effect  of toggling or\n",
"          removing  breakpoint  according  to  the  BreakPointAc-\n",
"          celeratorAction X resource.\n",
0,
};

static const char *help_level_2B[]=	/* typing line shortcuts */
{
"                    TYPING LINE SHORTCUTS\n",
"\n",
"     A few frequent requests can be invoked by typing input as an\n",
"     alternative to mouse operations.\n",
" \n",
"     o    Typing \"%g name\" will display the  global  variable  or\n",
"          function `name', skipping the sometimes slow search for\n",
"          `name' as a local variable.\n",
" \n",
"     o    Typing \"%l file\" will list 'file', the same as  expand-\n",
"          ing  the  list of source files clicking on a file name.\n",
"          It is only necessary to enter the  final  component  of\n",
"          the file name.\n",
" \n",
"     o    Typing \"%b function\" will enter  a  breakpoint  at  the\n",
"          entry to `function', the same as clicking on the break-\n",
"          point header, selecting \"add breakpoint\", and  entering\n",
"          the function name.\n",
" \n",
"     o    Typing \"/pattern\" or \"?pattern\" will do  a  forward  or\n",
"          backward search for pattern.  The search may be contin-\n",
"          ued in the same direction  by  hitting  RETURN,  or  in\n",
"          either direction using the Search pulldown menu.\n",
0,
};


static const char *help_level_3A[]=	/* C++ - general */
{
"                     C++ - GENERAL\n",
"\n",
"     Ups provides reasonable support for debugging C++ code  com-\n",
"     piled  with  cfront  version 3, and the Sun SC4 and g++ com-\n",
"     pilers.\n",
"\n",
"     For C++ code, classes are shown as structs with the  methods\n",
"     invisible.  Function  and variable names are demangled.  The\n",
"     function   stack   and   the   breakpoint   list   use   the\n",
"     `class::method' syntax. A restriction is that it is not pos-\n",
"     sible to debug class templates.  To debug executable code in\n",
"     header  files,  such  as  accessor  functions defined in the\n",
"     class definition, turn off in-lining when compiling.\n",
"\n",
"     When you click on symbols in C++ source, ups searches  local\n",
"     variables,  and  if  that  fails,  it searches fields in the\n",
"     `this' pointer, and finally it  searches  for  globals.   In\n",
"     addition it searches in all unions for matching components -\n",
"     that is, the union tag does not have appear in  the  source.\n",
"     When  breakpoints are set on overloaded methods in C++ code,\n",
"     ups queries whether to place  breakpoints  on  all  matching\n",
"     names.\n",
"\n",
"    o There is a new command line  argument  `-nodemangle'.  When\n",
"     specified, UPS will do no demangling on function or variable\n",
"     names. This should result is slightly faster invocation time\n",
"     for  pure  C code. It is still possible to debug C++ code in\n",
"     this mode, although the names need some mental  deciphering.\n",
"     A  unique  feature of this version of UPS, is that even when\n",
"     C++ names are shown mangled, you can generally  still  click\n",
"     on  variables  in the source window, and UPS will still find\n",
"     the name to display, albeit in a mangled state.\n",
0,
};


static const char *help_level_3B[]=	/* C++ - step button */
{
"                     C++ - STEP BUTTON\n",
"\n",
"    o Method calls can generally be stepped into, and breakpoints\n",
"     placed  directly in the source work reliably.\n",
"\n",
"    o The tags facility described below can usually  be  used  to\n",
"     move  to  the code to be stepped into, where a breakpoint or\n",
"     `execute to here' could be used.\n",
0,
};


static const char *help_level_3C[]=	/* C++ - casting */
{
"                     C++ - CASTING\n",
"\n",
"     In `add expr' and  breakpoint  code  you  can  cast  to  un-\n",
"     typdef'ed  structs,  including  classes in C++. Examples are\n",
"     \"(Class*)name\"   where    name    is    a    variable,    or\n",
"     \"(Class*)0x765678\"  for  a  numeric  address. If you want to\n",
"     cast to a class that is undefined in  the  current  context,\n",
"     try  an `add expr' for the source file where the constructor\n",
"     is defined. This generally works, at the file level, but not\n",
"     always.  It  should  always  work for `add expr' in the con-\n",
"     structor though. If you don't know which file the  construc-\n",
"     tor  is  defined  in,  enter the constructor name in the top\n",
"     typing line and press <RET> or <ESC>. The file name will  be\n",
"     displayed just above the source window.  You can then expand\n",
"     the source file list to find the file, select it, and  enter\n",
"     the `add expr' code.\n",
0,
};


static const char *help_level_3D[]=	/* C++ - tags */
{
"                     C++ - TAGS\n",
"\n",
"    o The `tags' operation works fairly well for C++  code.  This\n",
"     is  the  facility that allows easy navigation through source\n",
"     code by simply clicking on a function name. You can click on\n",
"     methods  and  it  will  usually  take you to the source. The\n",
"     `back' button returns to  the  original  code.   The  target\n",
"     needs  to be running to get it to work so it can look up the\n",
"     classes of variables. It  won't  always  get  it  right  for\n",
"     inherited  or  virtual  methods, but it is still a quick and\n",
"     useful way of navigating the code. You  may  have  to  click\n",
"     twice  sometimes  to  first  get  it read a variable. If the\n",
"     source is in a library with no debug information,  the  mes-\n",
"     sage line will show the method name in the library.\n",
0,
};


static const char *help_level_3E[]=	/* C++ - breakpoint code */
{
"                     C++ - BREAKPOINT CODE\n",
" \n",
"    o The built-in C interpreter now handles  simple  C++  method\n",
"     calls,  but  no  other  C++  constructs  are  supported. For\n",
"     instance, you can call methods,  including  destructors  and\n",
"     operators,  from  breakpoint  code  with the `class::method'\n",
"     syntax.  In such cases the `this' pointer has to  be  expli-\n",
"     citly  given  in the breakpoint code. To enable binding with\n",
"     overloaded methods, the code currently links with the  first\n",
"     matching  function that UPS finds, so use this with caution.\n",
"     As   an   example,   suppose   there   is   a    constructor\n",
"     `object::object',  and  within  the object class, there is a\n",
"     member called message of class Message. To  call  the  `get'\n",
"     method on this member use:\n",
"\n",
"         Message::get(&this->message);\n",
0,
};



static const char *help_level_3G[]=	/* C++ - Class Hierarchy */
{
"           C++ BASE CLASSES AND VECTOR TABLES\n",
"\n",
"     When the object selected in the display window is  a  class,\n",
"     clicking  on  ::  recursively  adds all base classes for the\n",
"     object to the display, and  displays  the  vector  table(s),\n",
"     showing  the  address  symbolically  when possible.  This is\n",
"     useful if you want to look at  a  member  of  a  base  class\n",
"     several  levels deep in the class hierarchy, without expand-\n",
"     ing everything in between.\n",
"\n",
"     In addition, it is usually possible to tell from the name of\n",
"     the  vector  table  which  subclass of the current class the\n",
"     object is \"really\" a member of.\n",
"\n",
"     It is useful to know that C++ distructors alter  the  vector\n",
"     table  of  the  object being destroyed, so that any function\n",
"     calls from the destructor go methods  for  the  class  being\n",
"     destroyed  rather  than  a  subclass whose distructor is now\n",
"     complete.  The result is that if an object has been  deleted\n",
"     looking  at  its  virtual  function table will show that its\n",
"     class is at the root of the class hierarchy rather than  any\n",
"     subclass.   When  debugging,  this is often a handy check to\n",
"     determine if an object has been deleted.\n",
0,
};


static const char *help_level_4[]=	/* source files */
{
"                     SOURCE FILES\n",
"\n",
"     o By default ups looks for source files in the directory  of\n",
"     the  target.  You can specify alternative source directories\n",
"     by giving a list of directories separated by `:' characters.\n",
"     An  empty initial path (i.e. a leading `:') means the direc-\n",
"     tory of the target.  On Suns running SunOS 4, the C compiler\n",
"     includes  directory paths for source files, so ups will nor-\n",
"     mally find source files in other  directories  even  without\n",
"     the source path argument.\n",
"\n",
"    o Search paths can be given to UPS at any time  during  debug\n",
"     by selecting the `Source Files' header in the display window\n",
"     and pressing `add source path'. The typing line will  prompt\n",
"     for input. It will display the last entry in the search path\n",
"     list as a default, if a list exists. Multiple paths  may  be\n",
"     entered  at once by entering a colon separated list. This is\n",
"     the same syntax as the  UPS  command  line  arguments.  This\n",
"     enables  the  paths for source files to be found without the\n",
"     need to back out of the debugger and add the search path  to\n",
"     the  command line. Note that this will work only if the file\n",
"     is already an entry in the source file list, but  cannot  be\n",
"     listed.  When this condition occurs, pressing the new `path'\n",
"     button will display the assumed path  for  the  file,  which\n",
"     must  be  in  error.  The  program may have to be statically\n",
"     linked to find all source file names.\n",
"\n",
"    o Binaries created with Purelink have two  entries  for  each\n",
"     incrementally  changed file in the source files listing. The\n",
"     first file entry is the real one - the second is an artifact\n",
"     of the incremental link.\n",
"\n",
"    o Source paths can also be set in the UPS init file.\n",
"\n",
"     When a source file or a function  is  selected,  the  button\n",
"     labeled `path' in the display area brings up a sub menu with\n",
"     `used',  `assumed', `rematch',  `reload'  and  `file  dates'\n",
"     items.  The  `used'  item  shows what file is actually being\n",
"     displayed in the source window. The `assumed' item  displays\n",
"     the  assumed  path name of the selected file as suggested by\n",
"     the target binary. If the file could not be found under  the\n",
"     `assumed' name, the `used' name will be the first good match\n",
"     in the source path list.  For C code, there is  normally  no\n",
"     problem  in  finding  the  source  and  hence the `used' and\n",
"     `assumed' paths will be the same. For  Centerline C++  code,\n",
"     the  two  are usually different. UPS uses symbol table func-\n",
"     tion line number information to find the most likely  match.\n",
"     This  also  allows  different  files  of the same name to be\n",
"     located correctly.\n",
"\n",
"     This feature removes the need to  constantly  rearrange  the\n",
"     `use'  paths in  the  UPS init file to accommodate debugging\n",
"     different targets. UPS outputs a message for C++ files indi-\n",
"     cating what file was matched.\n",
"\n",
"     When no symbols are available for  a  function,  both  items\n",
"     print the library name for that function.\n",
"\n",
"     If for some reason the match process  described  above  gets\n",
"     the wrong file, it is possible to find the next match in the\n",
"     search path list. To replace the file with the  next  match,\n",
"     select the `rematch' item in the `path' menu.\n",
"\n",
"     The `reload' item will reload the currently selected file. A\n",
"     situation where this may be useful is, when in the middle of\n",
"     a debug session, it becomes apparent that  the  debugger  is\n",
"     using  version of a file that differs from the build version\n",
"     of the file. If the current version is newer than the object\n",
"     code,  or  the target, the file will appear in reverse video\n",
"     as a warning.  To correct such a problem, restore the  file,\n",
"     and  reload.  This  will reload the text and also retest the\n",
"     file dates and remove the reverse video if appropriate.\n",
"\n",
"     The `file dates' item shows the full names and dates of  the\n",
"     source file, shared library if used, and target binary.\n",
"\n",
"     There is a menu associated with the file name  field,  which\n",
"     lies  above the source window. The mousehole and cursor also\n",
"     indicate that the right mouse button invokes a menu over the\n",
"     region  The  menu  has options to edit the source, show used\n",
"     and assumed file paths, rematch and reload the file, and  to\n",
"     show  file dates. The latter is useful for an explanation of\n",
"     why UPS may be showing reverse video for a file. In the bot-\n",
"     tom output window, it list the source file date, the associ-\n",
"     ated shared library date if applicable, and the target  file\n",
"     date.  The menu is a convenient way to get information about\n",
"     a file without having to find the file in  the  source  file\n",
"     list.  For  breakpoints,  when  the source is displayed, the\n",
"     menu provides a quick way to get  at  full  file  names  and\n",
"     dates.\n",
0,
};

static const char *help_level_5A[]=	/* general  */
{
"                     GENERAL\n",
"\n",
"     When you add a variable to the display it is displayed in  a\n",
"     default  format.  If you click the left button over the line\n",
"     for the variable, a menu appears in the top part of the win-\n",
"     dow.   You  can  use this menu to set the display format for\n",
"     the variable (Format), to change the  level  of  indirection\n",
"     for  pointers  (*  and &), to show all the members of struc-\n",
"     tures and unions (Expand and Collapse), to choose the format\n",
"     variables  are  displayed  in Format, to duplicate or delete\n",
"     entries for variables (dup and del) and to  control  whether\n",
"     typedefs are used in displayed variables (Decl).\n",
"\n",
"    o The  UPS_FORMATS Environment Variable: Most  software  pro-\n",
"     jects  of any size develop formal or informal naming conven-\n",
"     tions that make it possible to specify  the  desired  format\n",
"     for  a  variable  based on the name and, possibly, the type,\n",
"     regardless of the context in which  it  appears.   If,  like\n",
"     most  software engineers, you spend much of your time debug-\n",
"     ging the same code, you can set up a UPS_FORMATS  string  in\n",
"     your  environment  to  specify  the desired formats for fre-\n",
"     quently examined variables.\n",
"\n",
"     The following is an example of a UPS_FORMATS string:\n",
"\n",
"         export UPS_FORMATS=\"                                  \\\n",
"              unsigned : UHEX;   /* Default unsigned to hex */ \\\n",
"              unsigned *any_int[NTLW] : UDML;                  \\\n",
"              char abyte: OCT;                                 \\\n",
"              *bits* : UBIN;                                   \\\n",
"              auto \"\n",
"\n",
"     The first line of this format string causes  UPS  to  format\n",
"     unsigned  variables  in hex rather than decimal.  The format\n",
"     string accepts C-style comments to allow for  more  readable\n",
"     .login or .cshrc files.\n",
"\n",
"     The second line specifies that any unsigned  variable  whose\n",
"     name matches the string \"*any_int[NTLW]\" is an exception and\n",
"     should be formatted in decimal.  Pattern matching is  as  in\n",
"     shells such as  sh, csh, and bash.\n",
"\n",
"         o It is important that the exception come after the gen-\n",
"          eral rule specified in the first line.\n",
"\n",
"         o Note that in the UPS_FORMATS string, \"char *foo\" means\n",
"          any variable of type char whose name ends in \"foo\", not\n",
"          a variable named \"foo\" of type char*.\n",
"\n",
"     The third line specifies that any variable of type char  and\n",
"     name \"abyte\" should be formatted in octal.\n",
"\n",
"     The forth line specifies that any variable of any basic type\n",
"     with  the  string  \"bits\" in its name should be formatted in\n",
"     binary.\n",
"\n",
"     The last line specifies that any time the you change a  for-\n",
"     mat  UPS  should automatically save the change and use it as\n",
"     the default format for any variable of  the  same  name  and\n",
"     type.\n",
"\n",
"     The syntax of the UPS_FORMATS string is\n",
"\n",
"         format_string ::= format_spec [ ; format_string] [;]\n",
"\n",
"         format_spec ::= format_request | auto_save_request\n",
"\n",
"         format_request ::= [\"unsigned\"] [type] [ pattern ]\n",
"                              : [format]\n",
"\n",
"         type ::= \"char\" | \"short\" | \"int\" | \"long\"\n",
"\n",
"         pattern ::= < any C identifier with wild cards\n",
"                      '*', '?' or \"[]\" >\n",
"\n",
"         format ::=  \"UHEX\" | \"UOCT\" | \"UDML\" | \"UBIN |\n",
"                 | \"HEX\" | \"OCT\" | \"DML\" | \"ASCII\" | \"STRING\"\n",
"\n",
"         auto_save_request ::= \"auto\"\n",
"\n",
"     In the format request, if the type is  omitted  then  either\n",
"     all  unsigned  basic  types or all basic types regardless of\n",
"     sign are selected.  If the pattern is  omitted  the  default\n",
"     pattern  is  \"*\" so that all variables of the specified type\n",
"     are selected.\n",
0,
};

static const char *help_level_5B[]=	/* pointers  */
{
"                     POINTERS\n",
"\n",
"     The default for a pointer variable is  simply  to  show  the\n",
"     pointer  value in hex.  To take a common example, if you add\n",
"     a variable of type pointer to pointer to char called argv to\n",
"     the display, you will get a line like:\n",
"\n",
"         char **<argv>       0x7fffe184\n",
"\n",
"     The angle brackets separate the type  from  the  value.   In\n",
"     this  example,  what is shown is the value of argv, which is\n",
"     of type char **.\n",
"\n",
"     If you now click with the left mouse button  on  this  line,\n",
"     and select `*' (the leftmost caption) in the variables menu,\n",
"     the format of the line changes to something like:\n",
"\n",
"         char *<argv{0}>          0x7fffe1d0\n",
"\n",
"     This says that what is shown is the value of argv[0],  which\n",
"     is  of  type  char  *.  The braces (`{' and `}') are used to\n",
"     distinguish a dereferenced pointer from a true array.\n",
"\n",
"     A second click on the `*' menu option changes the line to:\n",
"\n",
"         char <argv{0}{0}>        \"foo\"\n",
"\n",
"     This is a special case in ups - variables of type  char  are\n",
"     displayed  as  strings  if  they  are indirected pointers or\n",
"     members of arrays.\n",
"\n",
"     The `&' menu option is the opposite of `*' -  it  drops  one\n",
"     level  of  indirection.  You can only use this on indirected\n",
"     pointers.  Use an expression if you want to see the  address\n",
"     of a variable (see EXPRESSIONS item).\n",
0,
};

static const char *help_level_5C[]=	/* arrays */
{
"                     ARRAYS\n",
"\n",
"     Arrays are initially displayed  with  all  subscripts  zero.\n",
"     You  can  edit the subscript to another value by clicking on\n",
"     it with the middle mouse button.  A marker bar appears,  and\n",
"     you  can  use the delete key to delete the old subscript and\n",
"     type a new one.  When you hit ESC,  the  value  of  the  new\n",
"     array element is shown.\n",
"\n",
"     Often you wish to quickly scan through all the  elements  of\n",
"     an array.  You can do this using either the arrow key or the\n",
"     `>' and '<' keys.  When editing an array subscript, the  '>'\n",
"     key  adds  one  to  the subscript value and displays the new\n",
"     element.  Similarly, the '<' key subtracts one from the sub-\n",
"     script  value.   Using these keys you can rapidly scan up or\n",
"     down an array.\n",
"\n",
"     Emacs users can use ^P and ^N as synonyms for '<'  and  '>'.\n",
"     Vi users can use 'k' and 'j' similarly.\n",
"\n",
"     The arrow (or whatever) keys actually act on  the  digit  to\n",
"     the left of the cursor, so by moving the cursor left you can\n",
"     step by tens, hundreds etc.\n",
0,
};

static const char *help_level_5D[]=	/* structures */
{
"                     STRUCTURES\n",
"\n",
"     Note: in this section `structures' also include unions: they\n",
"     are  simply treated as structures with all members having an\n",
"     offset of zero.\n",
"\n",
"     Variables that are structures or pointers to structures  are\n",
"     initially  displayed  with just the address in hex.  You can\n",
"     use the Expand command in the variables menu to add all  the\n",
"     members  of  a  structure  to  the  display.   The structure\n",
"     members are indented to make it clear which  structure  they\n",
"     belong to.   Note that a double  click on a  variable in the\n",
"     source window will also alternately expand and compress it.\n",
"\n",
"     If a structure element is itself a structure or a pointer to\n",
"     a  structure,  it  can  be  expanded in turn to show all its\n",
"     members.   In  this  way  linked  data  structures  can   be\n",
"     explored.   For  a  more selective way of exploring a linked\n",
"     data structure, see the  LINKED  DATA item below.\n",
"\n",
"     To remove all the members of a structure from  the  display,\n",
"     use  the Collapse command in the variables menu.  This has a\n",
"     submenu with the options First level  and  Completely.   The\n",
"     first of these removes all members except expanded ones; the\n",
"     second recursively collapses all expanded  structures  below\n",
"     the selected one.\n",
0,
};

static const char *help_level_5E[]=	/* linked data */
{
"                     LINKED DATA\n",
"\n",
"     Ups has several facilities that  are  useful  for  examining\n",
"     linked  data structures.  Firstly, you can expand structures\n",
"     or structure pointers.  By repeatedly  expanding  structures\n",
"     you can follow down a linked list or tree.\n",
"\n",
"     Often this adds too much information to the display, as  you\n",
"     are  probably  not interested in all the structure elements.\n",
"     There is a more selective  method  of  expanding  lists  and\n",
"     trees which lets you easily see just the elements you want.\n",
"\n",
"     Suppose you have a structure declaration like this:\n",
"\n",
"         struct linkst {\n",
"              struct linkst *li_prev, *li_next;\n",
"              int li_key;\n",
"         };\n",
"\n",
"     Suppose also that you  have  a  variable  linkptr  displayed\n",
"     which is a pointer to this structure.\n",
"\n",
"     If you type in a `.' followed by the name of  element,  such\n",
"     as  li_prev,  that  element  of  any  selected structures or\n",
"     structure pointers will be added to the display and selected\n",
"     when you hit ESC.\n",
"\n",
"     Assume  linkptr  in  the  example  above  is  displayed  and\n",
"     selected.   Typing  .li_next  followed  by  ESC will add the\n",
"     li_next field of linkptr to the display and select  it,  and\n",
"     deselect  linkptr.   Typing ESC again will add the next ele-\n",
"     ment of the list.  Thus by repeatedly  typing  ESC  you  can\n",
"     easily walk down a linked list.\n",
"\n",
"     You can give many structure elements  separated  by  spaces.\n",
"     Thus the line\n",
"\n",
"         .li_key .li_next\n",
"\n",
"     would add both fields to the display.  In this way  you  can\n",
"     walk  down  a linked list with members of interest displayed\n",
"     as well as the links.\n",
"\n",
"     One problem with this way of looking at lists  is  that  the\n",
"     indentation  of  structure  elements  tends to make the list\n",
"     wander off the right hand side  of  the  display  area.   To\n",
"     avoid this you can say `@member' rather than `.member'.  The\n",
"     `@' character means do not indent - this is the only differ-\n",
"     ence between it and `.'.  Thus to get a nicely laid out list\n",
"     in the example above you could enter the line:\n",
"\n",
"         .li_key @li_next\n",
"\n",
"     and keep typing ESC to walk down the list.\n",
"\n",
"     One last wrinkle: if you add `#nnn' to the end of the typing\n",
"     line,  where  `nnn' is a decimal number, the effect is as if\n",
"     you had pressed ESC that number of times.  This is handy  if\n",
"     you  want  to  see  all of a 500 element linked list without\n",
"     having to type ESC 500 times.\n",
"\n",
"     In C interpreter code (described in  the  previous  section)\n",
"     you  can  scan  through a linked list as if it were an array\n",
"     using the (non-standard) `->[count]' operator.   This  is  a\n",
"     shorthand  for  applying the `->' operator count times.  You\n",
"     can use ^N and ^P as described in the  previous  section  to\n",
"     bump  the  count  parameter  up  or  down and step through a\n",
"     linked list one element at a time.\n",
"\n",
"     Thus in the example above, adding the  following  expression\n",
"     to the display area:\n",
"\n",
"         linkptr->[0]li_next\n",
"\n",
"     would just show the value of linkptr  (the  ->  operator  is\n",
"     being applied zero times).  You can expand the structure and\n",
"     add and delete elements to get the display  set  up  as  you\n",
"     like.  Then you can edit the `0' to `1' to see the next ele-\n",
"     ment of the list, and so on.\n",
0,
};

static const char *help_level_5F[]=	/* changing values */
{
"                     CHANGING VALUES\n",
"\n",
"     You can change the value of a displayed variable  simply  by\n",
"     editing the displayed value (i.e. by clicking on it with the\n",
"     middle mouse button and editing in  the  new  value).   This\n",
"     works  for  C pointers and integral types (including enums),\n",
"     floating point values and strings.\n",
"\n",
"     You can use any of the integer display formats for  the  new\n",
"     value (decimal, hex, octal, binary or ASCII character).  You\n",
"     can use enum constant names for new enum values,  and  func-\n",
"     tion  names  for function pointers.  When editing strings or\n",
"     characters you can use the standard C notation  for  special\n",
"     characters (`\\n', `\\b', `\\007' etc).\n",
"\n",
"     Normally ups will not let you edit extra characters  into  a\n",
"     string as this would overwrite whatever was stored in memory\n",
"     just after the string.  If space  is  known  to  exist  (for\n",
"     example  if  the  string is stored in an array of known size\n",
"     and there are unused bytes) then you can add as many charac-\n",
"     ters  as will fit.  If you know you want to overwrite memory\n",
"     beyond the end of the string you can force ups to  accept  a\n",
"     long  value by putting `>>' before the leading quote charac-\n",
"     ter of the string.\n",
"\n",
"     Normally a trailing NULL ('\\0') is added to the edited string\n",
"     in the normal C way.  If you delete the trailing quote char-\n",
"     acter then this is omitted.\n",
0,
};

static const char *help_level_5G[]=	/* typedefs */
{
"                     TYPEDEFS\n",
"\n",
"     If a structure, union or enum has a typedef  name  then  ups\n",
"     will  use it in the display area.  Thus if you have the fol-\n",
"     lowing in a function:\n",
"\n",
"         typedef struct foo_s {\n",
"              int x;\n",
"              int y;\n",
"         } foo_t;\n",
"\n",
"         foo_t *f;\n",
"     then clicking on variable f will add a line like:\n",
"\n",
"              foo_t *<f>          0x40ec\n",
"\n",
"     to the display area.  Typedefs are not used if they  hide  a\n",
"     level  of  indirection or an array, or if the typedefed type\n",
"     is not a struct, union or enum.\n",
"\n",
"     If you want to see the non-typedef type for  a  variable  in\n",
"     the  display  area,  select  the variable and press and hold\n",
"     down the left mouse button over  the  Decl  command  in  the\n",
"     variables  menu.   This  produces a popup menu with the cap-\n",
"     tions Use typedefs and Ignore typedefs.  Release  the  mouse\n",
"     over Ignore typedefs and you will be shown the non-typedefed\n",
"     type for all the selected variables.\n",
0,
};

static const char *help_level_5H[]=	/* expressions */
{
"                     EXPRESSIONS\n",
"\n",
"     You can add C  expressions  as  well  as  variables  to  the\n",
"     display  area.   This  is  useful if you wish to see what an\n",
"     expression in the source code evaluates to.  It also  allows\n",
"     you  to  use casts when you know better than the source code\n",
"     what the type of a given variable is.\n",
"\n",
"     To add an expression, select a function in the  stack  trace\n",
"     and  click  on  Add expr in the function menu.  A marker bar\n",
"     appears, ready for you to enter  an  expression.   When  you\n",
"     have  finished  type ESC, and if the expression is legal the\n",
"     value will be displayed.   If  there  is  an  error  in  the\n",
"     expression  you will get an error message and the marker bar\n",
"     will be repositioned at the point of the error.\n",
"\n",
"     In an expression you can use any  variable  name,  structure\n",
"     tag  or  typedef  name that is in scope in the function.  If\n",
"     you want to add expressions using a  variable  in  an  inner\n",
"     block,  you will have to add the expression to the appropri-\n",
"     ate inner block.  The easiest way to  get  the  inner  block\n",
"     object  displayed  is  to  click  on a variable in the inner\n",
"     block in the source region.  Once it is displayed select the\n",
"     block header and click on Add expr in its menu.\n",
"\n",
"     You can `bump' numbers in expressions in a  similar  way  to\n",
"     array  subscripts.   Hitting  the  down arrow (or control-N)\n",
"     over a number while  editing  an  expression  increases  the\n",
"     digit  to  the  left  of the marker bar and displays the new\n",
"     value  of  the  expression.   Similarly  the  up  arrow  (or\n",
"     control-P) decreases the digit to the left of the marker bar\n",
"     and redisplays the expression value.\n",
"\n",
"     Expressions are reevaluated like variable values every  time\n",
"     the  target  stops.  They also have the same menu associated\n",
"     with them as variables, and you can  have  both  expressions\n",
"     and  variables in the same selection.  All the menu commands\n",
"     work as they do on variables.  This means in particular that\n",
"     if  you  add an expression whose type is `pointer to struct'\n",
"     (or union) you can use Expand to  show  the  structure  ele-\n",
"     ments.  You can also use Format to change the format used to\n",
"     display the expression value.\n",
"\n",
"     You can call target functions in expressions, but you  can't\n",
"     modify target data in a display area expression (thus opera-\n",
"     tors like `++' are illegal).\n",
0,
};

static const char *help_level_5I[]=	/* expressions */
{
"                     DUMPING MEMORY\n",
"\n",
"\n",
"     The contents of raw memory may be dumped to the output  win-\n",
"     dow  in two ways.  If the memory to be dumped is the address\n",
"     of a simple variable or structure, or is  pointed  to  by  a\n",
"     pointer variable, select the variable of interest, press the\n",
"     Expand option then select Dump  Memory  from  the  resulting\n",
"     popup  menu.  This option displays the contents of memory at\n",
"     the selected address.  The length of memory  displayed,  and\n",
"     the  grouping  (as  bytes, shorts, or longs) is based on the\n",
"     type of object selected.  This option  also  prints  in  the\n",
"     output  window the equivalent typing line command, which you\n",
"     may copy to the typing line and edit as required.\n",
"\n",
"     The commands to display memory through the typing line begin\n",
"     with \"%d\" (for dump):\n",
"\n",
"         %d  address [size|.. end_address]\n",
"              dump size bytes of memory at address\n",
"         %db address [size|.. end_address]\n",
"              dump size bytes of memory as bytes at address\n",
"         %ds address [size|.. end_address]\n",
"              dump size bytes of memory as shorts at address\n",
"         %dl address [size|.. end_address]\n",
"              dump size bytes of memory as longs at address\n",
"\n",
"     If the size is omitted,  16  bytes  are  displayed.  If  the\n",
"     grouping  is  unspecified,  bytes  or  shorts  are  selected\n",
"     depending on target endianness.\n",
0,
};

static const char *help_level_6A[]=	/* editable fields */
{
"                     EDITABLE FIELDS\n",
"\n",
"     All editable fields in ups work in the same way.   To  start\n",
"     editing  you click the middle mouse button over the editable\n",
"     text.  A vertical marker bar appears - characters  that  you\n",
"     type appear to to the left of the marker bar.  You can repo-\n",
"     sition the marker bar by clicking in the new  position  with\n",
"     the middle mouse button.\n",
"\n",
"     Clicking the left or right button confirms the edit.  Click-\n",
"     ing  the  middle mouse button outside the editable text area\n",
"     also confirms the edit.  In both cases the  mouse  click  is\n",
"     then  interpreted  as normal - this means that to confirm an\n",
"     edit you can simply move on to another activity.  The  final\n",
"     way  to  confirm  an edit is to type ESC (the escape key) or\n",
"     click the left mouse button on the Enter Button  (the  small\n",
"     region to the right of the typing line with the \"<<\" image).\n",
"\n",
"     To paste the current window system cut buffer, use Control-y\n",
"     or click the middle mouse button on the Enter Button.\n",
"\n",
"     When you try to confirm an edit  ups  checks  that  the  new\n",
"     field  value is reasonable.  If not you get an error message\n",
"     and you are left in the edit.  An immediate  second  attempt\n",
"     to  quit  abandons  the edit and restores the original field\n",
"     value.\n",
"\n",
};
static const char *help_level_6B[]=	/* control and meta chars */
{
"     The following special characters are recognized while  edit-\n",
"     ing text:\n",
"\n",
"     ^C (control-C)\n",
"          Cancel the edit and restore the original text.\n",
"\n",
"     DEL (the delete key)\n",
"          Delete the character just before the marker bar.\n",
"\n",
"     ^U   Delete the text from the  start  of  the  line  to  the\n",
"          marker bar.\n",
"\n",
"     ESC (the escape key)\n",
"          Confirm the edit.\n",
"\n",
"     ^H (backspace), Left-arrow  (R10) and ^B\n",
"          Move the marker bar back one character.\n",
"\n",
"     ^L, Right-arrow (R12) and ^F \n",
"          Move the marker bar forward one character.\n",
"\n",
"     ^Y   Insert the X selection.\n",
"\n",
"     ^A   Go to the beginning of line.\n",
"\n",
"     ^E   Go to the end of line.\n",
"\n",
"     ^K   Delete character from the right of the marker  bar\n",
"          to the end of line.\n",
"\n",
"  Meta-m  Move to first non-whitespace  character\n",
"\n",
"  Meta-@, M-SPC\n",
"          Set mark\n",
"\n",
"     ^W   Delete text between mark and point\n",
"\n",
"     ^P, Up-arrow\n",
"          Retrieve the next previous item in the history buffer.\n",
"\n",
"     ^N, Down-arrow\n",
"          Retrieve the next later item in the history buffer.\n",
"\n",
" Meta-B   Move backwards one word\n",
"\n",
" Meta-F   Move forward one word\n",
"\n",
" Meta-D   Delete word starting at cursor\n",
"\n",
" Meta-DEL Delete word before cursor\n",
"\n",
0,
};
static const char *help_level_6C[]=	/* edit history */
{
"                     EDIT HISTORY\n",
"\n",
"     Most of the editable fields in Ups have their own history of\n",
"     recently typed commands.  For example, there is a history of\n",
"     typing line commands, a history of breakpoint code  entered,\n",
"     and a history of variable values changed.\n",
"\n",
"     Pressing the Left mouse button on the  History  Button,  the\n",
"     small  region  to the right of the typing line with the tri-\n",
"     angular image, pops up a menu of recently entered  data  for\n",
"     that field.\n",
"\n",
"     When editing most single line fields,  a  control-P,  or  up\n",
"     arrow  moves the history pointer back one entry and replaces\n",
"     the current text with the previous entry. Typing a  control-\n",
"     N,  or  down  arrow,  moves  the history pointer forward one\n",
"     entry.\n",
"\n",
"     Edit histories are saved between sessions  of  Ups  in  ups-\n",
"     state/editHistory,  if  you use the ups-state feature, or in\n",
"     the file ~/.upsEditHistory if not.\n",
0
};


static const char *help_level_7[]=	/* command arguments */
{
"                     COMMAND ARGUMENTS\n",
"\n",
"     This section gives a complete  description  of  the  command\n",
"     line arguments accepted by ups.  The command line syntax is:\n",
"\n",
"          ups  target  [corefile|pid]  [[:]srcdir[:srcdir]]   [-a\n",
"          target-args]\n",
"\n",
"     The only mandatory argument is the name  of  the  executable\n",
"     file containing the program to be debugged (the target).\n",
"\n",
"     If a corefile argument is given it is taken to be  the  name\n",
"     of a core image dumped from target.  If no corefile argument\n",
"     is given and there is a core image file called `core' in the\n",
"     directory of the target then that is taken as the core file.\n",
"     Old core files, and core files which weren't dumped from the\n",
"     target, are silently ignored unless you give the name of the\n",
"     core file explicitly (in which case ups  will  use  it,  but\n",
"     give a warning message).\n",
"\n",
"     If the corefile argument consists solely of  digits,  it  is\n",
"     taken  to  be the process id of the target.  This allows you\n",
"     to attach ups to an already running process on machines with\n",
"     the  necessary support (currently only Suns).  If you subse-\n",
"     quently quit ups  while  still  attached  in  this  way,  it\n",
"     detaches from the target, allowing the target to continue.\n",
"\n",
"     By default ups looks for source files in  the  directory  of\n",
"     the  target.  You can specify alternative source directories\n",
"     by giving a list of directories separated by `:' characters.\n",
"     An  empty initial path (i.e. a leading `:') means the direc-\n",
"     tory of the target.  On Suns running SunOS 4, the C compiler\n",
"     includes  directory paths for source files, so ups will nor-\n",
"     mally find source files in other  directories  even  without\n",
"     the source path argument.\n",
"\n",
"     You can specify the arguments  that  the  target  should  be\n",
"     invoked  with  by giving the -a option, followed by a single\n",
"     argument.  You can give multiple arguments for the target by\n",
"     enclosing  the list of arguments in single or double quotes.\n",
"     Ups will itself interpret metacharacters like `*' and `>'  -\n",
"\n",
"     The second line of the display area shows the  command  line\n",
"     arguments  that  will be given to the target when it is next\n",
"     started.  The arguments shown include the  zero'th  argument\n",
"     which is initially set to the name of the target.\n",
"\n",
"     You can specify an initial set of arguments for  the  target\n",
"     with  the  -a  option when you start ups.  If you don't give\n",
"     the -a option and you are debugging from a  core  file,  ups\n",
"     attempts to extract the command line arguments from the core\n",
"     file.  Otherwise the  command  line  contains  no  arguments\n",
"     other than the name of the target.\n",
"\n",
"     Ups parses the command line in a similar way to  the  shell.\n",
"     It  supports Bourne shell type redirection (>, >>, <, >&dig,\n",
"     etc.) as well as the csh forms >& and >>&.  Ups also  under-\n",
"     stands  most csh metacharacters - globbing with `*', `?' and\n",
"     `[xyz]', the `~',  `~user'  and  `{a,b,c}'  shorthands,  and\n",
"     quoting  with  single  or  double quotes and backslash.  The\n",
"     current version of ups does  not  support  $var  type  shell\n",
"     variable substitution.\n",
"\n",
"     You can edit the command line at any time to change the com-\n",
"     mand  line  arguments  (although  the changes will only take\n",
"     effect when you next start the target).\n",
"\n",
"     The command name shown is just the zero'th argument and  can\n",
"     be  edited  just  like  the other arguments.  This is useful\n",
"     with programs which use the zero'th argument as  a  sort  of\n",
"     hidden  flag.   Changing  the  command name only affects the\n",
"     arguments given to the target - it  does  not  change  which\n",
"     program is being debugged.\n",
0,
};


static const char *help_level_8[]=	/* custom menu */
{
"                     CUSTOM MENU\n",
"\n",
"     It is possible to insert pre-defined  strings  when  editing\n",
"     text.   This  applies to all editing: in the typing line, in\n",
"     the display area, in breakpoint code and in the output  win-\n",
"     dow.   The   RMB  invokes  a  menu  of  strings  defined  by\n",
"     environment variables of name `UPS_F*_STR' where  `*'  is  a\n",
"     number from 1 through 12. When the cursor is over the typing\n",
"     line or output window, the mousehole shows \"(menu)\" for  the\n",
"     right  button  as  an  indication  that a custom menu may be\n",
"     available.\n",
"\n",
"     The UPS_F*_STR strings  accept  control,  meta,  and  escape\n",
"     characters as follows:\n",
"\n",
"          \\n, \\r, or \\e: Enter an escape character  to  terminate\n",
"          the edit\n",
"\n",
"          ^A, ^B, etc.: Enter the corresponding  control  charac-\n",
"          ter.\n",
"\n",
"          @f, @b, etc.: Enter the corresponding  meta  character.\n",
"          This allows movement by words.\n",
"\n",
"          \\\\ or \\^ or \\@: Override the special  meaning of '\\',\n",
"          '^', or '@'.\n",
"\n",
"     As an example, it is often nice to have skeleton strings for\n",
"     `printf'  or  `cout'  statements  in  breakpoint  code, or a\n",
"     directive for expanding linked lists for the typing line, or\n",
"     a string for setting breakpoints on `cout' statements in C++\n",
"     code. Yet another string can be used to call  strcmp  for  a\n",
"     conditional  breakpoint.  The  F6  string  pastes  in the X-\n",
"     windows selection and the F7 string  sets  a  breakpoint  in\n",
"     purified code.\n",
" \n",
"     To do this, put the following in your environment:\n",
" \n",
"          setenv UPS_F1_STR '$printf(\"\\n\");'\n",
"          setenv UPS_F2_STR 'if (strcmp(, \"\"))'\n",
"          setenv UPS_F3_STR '@name .next'\n",
"          setenv UPS_F4_STR \"ostream::operator<<\"\n",
"          setenv UPS_F5_STR 'ostream::operator<<(&cout, \"\");'\n",
"          setenv UPS_F6_STR \"^e^u^y\\n\"\n",
"          setenv UPS_F7_STR '%b purify_stop_here\\n'\n",
0,
};


static const char *help_level_9[]=	/* init file  */
{
"                     INIT FILE\n",
"\n",
"     The debugger reads an initialization file of name `.upsinit'\n",
"     in the users home directory upon invocation.\n",
"\n",
"     There are four basic commands that can be specified  in  the\n",
"     file.  These  are  use  <directory> , load <string> , noload\n",
"     <string>  ,  and  break  function  libraries  to  load;   or\n",
"     libraries not to load when reading symbols; and functions in\n",
"     which to set breakpoints.\n",
"\n",
"     By only loading the minimum  standard  libraries,  ups  will\n",
"     start a lot faster. Other libraries can be loaded on the fly\n",
"     by selecting \"Target\", then \"Load library\", or by  selecting\n",
"     an unloaded library name in the stack trace.\n",
"\n",
"         An example is:\n",
"\n",
"         load *libc.so*\n",
"         load *libC.so*\n",
"         load /usr/lib/lib*\n",
"         load /usr/openwin/*\n",
"         load /usr/platform/*\n",
"         load /usr/dt/lib/*\n",
"         # for target specific libraries:\n",
"         load ./*\n",
"         load ../*\n",
"\n",
"     Note that \"./\" has the meaning of the directory of the  tar-\n",
"     get, not of where ups was started.\n",
"\n",
"     The initialization file is also convenient  for  adding  new\n",
"     source paths or loading additional libraries on the fly.\n",
"\n",
"     The source paths are needed for Centerline C++ because  clcc\n",
"     creates  c  files  in  temporary directories, and the symbol\n",
"     tables suggest that this is where the parent C++  files  are\n",
"     too.  This  is  not  a  problem for SC4 or g++ however.  The\n",
"     `use' command is equivalent to the colon separated  list  of\n",
"     directories that can be given on the command line for invok-\n",
"     ing ups, or during debugging with the add source  path  cap-\n",
"     tion.\n",
"\n",
"     The load/noload allow you  to  just  load  symbols  for  the\n",
"     debugging area of interest, instead of always loading every-\n",
"     thing.\n",
"\n",
"     You can see all the symbol table names that  are  loaded  by\n",
"     doing a \"setenv VERBOSE 1\" before calling UPS.\n",
"\n",
"     You can specify breakpoints with  the  syntax  \"break  func-\n",
"     tion\".  Unlike  the  `-record'  style  syntax, this does not\n",
"     require a file or a line number, hence the same  breakpoints\n",
"     will  work  on different versions of the target source, pro-\n",
"     vided the functions exist. UPS  will  silently  skip  break-\n",
"     points  that  it  cannot  set.   The  limitation is that the\n",
"     breakpoint will always be set at the beginning of the  func-\n",
"     tion.\n",
"\n",
0,
};


static const char *help_level_10[]=	/* attach / detach */
{
"                     ATTACH AND DETACH\n",
"\n",
"    o See the COMMAND ARGUMENTS for attaching to a target via the\n",
"     command line.\n",
"\n",
"     When ups has been attached to a target, it  is  possible  to\n",
"     detach  without quitting the debugger by pressing the Detach\n",
"     caption at the bottom of the  display  window.  At  a  later\n",
"     time,  ups  can then be attached to the same instance of the\n",
"     target, or to a new instance of  the  target  by  using  the\n",
"     attach caption described below.\n",
"\n",
"     On an attach, the debugger will reload any shared  libraries\n",
"     that  have changed, as well as any new shared libraries that\n",
"     the target uses.  If ups has been detached from  the  target\n",
"     as described above, or if the target terminates for any rea-\n",
"     son, it is possible to attach to the same or a  new  invoca-\n",
"     tion of the target without quitting the debugger. The advan-\n",
"     tage of this is that it may take several minutes for ups  to\n",
"     initially  come  up,  but  once  the symbol tables have been\n",
"     read, the time to reattach will be at most, of the order  of\n",
"     tens  of  seconds,  and often just a few seconds. All break-\n",
"     points and even  breakpoint  code  will  still  work.  After\n",
"     pressing  Attach  you  will  be  prompted  to  enter the PID\n",
"     number. The PID of the last attached process is displayed as\n",
"     a  default. If the new invocation of the target has changed,\n",
"     the reattached session may not work correctly if  statically\n",
"     linked  object  files  have  changed.  ups  will re-read any\n",
"     changed shared libraries when attaching.\n",
"\n",
"     A very handy use of  the  attach  button  is  for  debugging\n",
"     spawned  processes  that can timeout unless a communications\n",
"     handshake or license check  is  performed  quickly.  If  the\n",
"     spawned  process  is stopped at a pause while UPS is invoked\n",
"     from scratch, the process may well timeout and  exit  before\n",
"     UPS  can  read  all  the  necessary symbol information.  The\n",
"     solution is to first invoke UPS on the target without a  PID\n",
"     in  the  command  line.  After  the  symbols have been read,\n",
"     breakpoints can be set, then the real process to be debugged\n",
"     is spawned. Then press `attach' and quickly enter the PID of\n",
"     the spawned process to debug.\n",
"\n",
"     The Kill command kills off the current instance target  pro-\n",
"     cess.   You  can  then use Start or Execute to here to start\n",
"     the target again.  Quitting ups also kills the  target  pro-\n",
"     cess (unless you attached ups to a running process, in which\n",
"     case ups detaches from the process and leaves it to continue\n",
"     unmolested).\n",
0,
};

static const char *help_level_11a[]=	/* About X */
{
"                    ABOUT X RESOURCES\n",
"\n",
"     UPS can be configured to meet individual preferences through\n",
"     X resources.\n",
"\n",
"     As with any X application, you specify an X resource in  the\n",
"     file  .Xdefaults  in  your home directory.  Typical lines in\n",
"     .Xdefaults look like:\n",
"\n",
"         *.Font: fixed            /* default font*/\n",
"         Ups.Geometry: 650x550    /* Geometry for Ups */\n",
"\n",
"     The first example, with the initial '*', specifies your pre-\n",
"     ferred font for any X application.  The second example, with\n",
"     the initial \"Ups\", specifies the width and height ( in  pix-\n",
"     els)  of the main window for ups only.  Other X applications\n",
"     may look for a Geometry resource, but will not pick  up  the\n",
"     one specified for Ups.\n",
"\n",
"     If  you want to specify not only width and height, but exact\n",
"     position, use the following form of the Geometry resource:\n",
"\n",
"         Ups.Geometry: 650x550+20+30  /* Geometry for Ups */\n",
"\n",
"     This example will place the main UPS window 20 pixels in and\n",
"     30  down  from the upper left corner of the screen.  Replace\n",
"     the + signs with - for a position relative to the right side\n",
"     or bottom of the screen.\n",
"\n",
"     Your .Xdefaults file is normally processed when you start X.\n",
"     After  editing your .Xdefaults file, run \"xrdb -load ~/.Xde-\n",
"     faults\" to process the file so that X applications will  see\n",
"     your changes.\n",
"\n",
"     Many X resources used by UPS have command line  equivalents.\n",
"     The  font  may  be  specified  on the command line with \"-fn\n",
"     <font>\", and the geometry with \"-geometry  <geometry_spec>\".\n",
"     You can to use the command line versions to experiment, then\n",
"     set up the resource in you .Xdefaults file if you  like  the\n",
"     result.\n",
0
};
static const char *help_level_11b[]=	/* fonts & colors */
{
"                        FONTS AND COLORS\n",
"\n",
"     In addition to  the  standard  X  resources  for  fonts  and\n",
"     colors,  UPS  allows for special fonts in breakpoint code or\n",
"     as menu text; and UPS has extensive options for  setting  up\n",
"     colors.   These  are  completely  documented  in the UPS man\n",
"     page, so we do not reproduce them here.\n",
"\n",
"     The standard UPS color scheme is set up by the  file  \"Ups\",\n",
"     contained  in  the  UPS distribution.  If UPS comes up black\n",
"     and white, it is probably because that  file  has  not  been\n",
"     properly    installed.     It   should   be   installed   as\n",
"     /usr/lib/X11/app-defaults/Ups.\n",
"\n",
"     If for some reason you can not place the Ups  file  in  that\n",
"     directory, set up one of the following environment variables\n",
"     to specify the directory containing the Ups file:\n",
"\n",
"         XAPPLRESDIR - directory containing Ups\n",
"         XUSERFILESEARCHPATH -\n",
"                  Colon separated list of directories to search\n",
"\n",
"     Simple fonts may be specified by aliases such as \"8x13\"  for\n",
"     a  font  that  is  8  pixels  wide and 13 high. See the file\n",
"     /usr/lib/X11/fonts/misc/fonts.alias   for   available   font\n",
"     aliases.\n",
0
};
static const char *help_level_11c[]=	/* Split Screens */
{
"                     SPLIT SCREENS\n",
"\n",
"     The standard UPS configuration is as a single  top  level  X\n",
"     window with three main regions: the display window ( display\n",
"     of stack, variables, etc.), the source display, and the out-\n",
"     put  window (output of $printf, help, etc.). Three alternate\n",
"     configurations are possible.\n",
"\n",
"     To split off the combined source and  output  windows  as  a\n",
"     separate top level window, use the following X resource:\n",
"\n",
"         Ups.WantSplitWindows: yes\n",
"\n",
"     or start UPS with -split on the command line.\n",
"\n",
"     To split off the output alone window as a separate top level\n",
"     window, use the following X resource:\n",
"\n",
"         Ups.WantSplitOutputWindows: yes\n",
"\n",
"     or start UPS with -splitoutwin on the command line.\n",
"\n",
"     To split off both the source and output windows, use both  X\n",
"     resources, or use both command line options.\n",
"\n",
"     If your X display has more than one screen, you can  arrange\n",
"     that the  multiple UPS windows come up on different screens.\n",
"     See the UPS manual page for details.\n",
"\n",
"     To specify the geometry of the source  and  output  windows,\n",
"     use     the     X     resources     Ups.Src.Geometry     and\n",
"     Ups.Output.Geometry.\n",
0
};
static const char *help_level_11d[]=	/* Raise/Lower */
{
"                 AUTOMATIC RAISE AND LOWER\n",
"\n",
"     You can arrange that UPS will  automatically  raise  itself,\n",
"     becoming  the  foremost  X window, when the debugged process\n",
"     stops.  This can be convenient  if  the  application  window\n",
"     hides the UPS window so you don't realize that it's stopped.\n",
"     You can also arrange that UPS lowers itself when the process\n",
"     goes  back  into run.  This can be convenient in putting the\n",
"     debugged application back in the front where  you  can  work\n",
"     with  it.   Or you can arrange that UPS iconifies itself; on\n",
"     some window managers that might be more convient the  lower-\n",
"     ing  itself since it puts the UPS icon where you can quickly\n",
"     find it.\n",
"\n",
"     You can change these settings at any  time  with  the  popup\n",
"     menu  under  the \"Windows\" button at the top of the UPS win-\n",
"     dow.  To control the initial setting, set  the  following  X\n",
"     resources or use the following command line options:\n",
"\n",
"         To raise on a break\n",
"            Ups.WantRaiseOnBreak: yes    or -raise_on_break\n",
"\n",
"         To raise on a break and lower on run\n",
"            Ups.WantLowerOnRun: yes      or -lower_on_run\n",
"\n",
"         To raise on a break and iconify on run\n",
"            Ups.WantIconifyOnRun: yes    or -iconify_on_run\n",
"\n",
"     For the lower and iconify options, you can adjust the period\n",
"     UPS waits before lowering or iconifying itself with the fol-\n",
"     lowing X resource:\n",
"\n",
"         Ups.LowerOnRunTime: 1000     /* delay time in milliseconds */\n",
"\n",
"     The default delay is 1500, or 1.5 seconds.\n",
0
};
static const char *help_level_11e[]=	/* Breakpoint Menu */
{
"                BREAKPOINT MENU\n",
"\n",
"     When you make a selection with the source window popup  menu\n",
"     UPS  remembers  your  selection and makes it the default the\n",
"     next time you use that menu.  This can be convenient if  you\n",
"     are going to use \"Execute to here\" several times in a row to\n",
"     step through a section of code.  But it can be a problem  if\n",
"     you  forget  that  your last selection was \"Execute to here\"\n",
"     and try to set a breakpoint.\n",
"\n",
"     You can make the menu default stay fixed on \"Add breakpoint\"\n",
"     by setting the following X resource\n",
"\n",
"         Ups.SourceMenuDefault: AddBreakPoint\n",
"\n",
"     You can also use this resource to select any  of  the  other\n",
"     menu  options  as  the  fixed default.  Also, can arrange to\n",
"     have an alternate default selected by pressing the shift key\n",
"     and  right  mouse button, or you can use that combination to\n",
"     deliberately change the default.  See the  UPS  manual  page\n",
"     for details.\n",
0
};
static const char *help_level_11f[]=	/* Scrolling */
{
"                     SCROLLING\n",
"\n",
"     UPS scroll bars work differently from most  X  applications.\n"
"     In  UPS,  you  move the mouse up to move the display up, and\n"
"     down to move the display down.    To  reverse  these  direc-\n"
"     tions, use the following X resource:\n"
"\n",
"         Ups.ScrollbarType: MOTIF\n",
"\n",
"\n",
"     You may find that UPS scrolls too fast.  To  introduce  some\n",
"     delay in the scroll loop, use the following X resource:\n",
"\n",
"         Ups.ScrollDelay: 40\n",
"\n",
"     values between 25 and 50 usually produce good results.\n",
0
};
static const char *help_level_11g[]=	/* resizing */
{
"                     RESIZING WINDOWS\n",
"\n",
"     The display window, source window and bottom  output  window\n",
"     can  be  resized  by using the left mouse button to move the\n",
"     double arrow gadget at the right  of  the  button  bars.  In\n",
"     addition, X resources can be used:\n",
"\n",
"     SrcwinPercent\n",
"          The percentage of the window height used for the source\n",
"          window  (after  space used by the fixed size regions is\n",
"          subtracted).  The default is 50 (i.e. half).\n",
"\n",
"     DisplayAreaPercent\n",
"          The percentage  of  the  window  height  used  for  the\n",
"          display  area  (after  space  used  by  the  fixed size\n",
"          regions is subtracted).  The default is 50 (i.e. half).\n",
"          If  DisplayAreaPercent  and  SrcwinPercent are both set\n",
"          they need not add up  to  100  -  the  values  actually\n",
"          specify  a  proportion of the total.  Thus setting both\n",
"          to 20 (or any pair of identical values)  results  in  a\n",
"          50-50 split.\n",
"\n",
"     OutwinPercent\n",
"          The percentage of the window height used for the output\n",
"          window  if  and  when  it  is added.  The default is 10\n",
"          (which actually means a fifteenth of the total  -   see\n",
"          note about SrcwinPercent above.\n",
0,
};
static const char *help_level_11h[]=	/* No Mousehole */
{
"                 REMOVING THE MOUSEHOLE\n",
"\n",
"     The \"mousehole\" is the region in the upper right  corner  of\n",
"     the  main  UPS  window that updates to show what actions are\n",
"     invoked  by  the  three  mouse  buttons.   To   remove   the\n",
"     mousehole,  obtaining  more  space  for  the typing line and\n",
"     error message text, use the following X resource:\n",
"\n",
"         Ups.WantMousehole: no\n",
"\n",
"     or start UPS with -nomousehole on the command line.\n",
"\n",
"     This option can be especially useful with large fonts, which\n",
"     make the typing line rather short.\n",
0
};

static const char *help_level_12[]=	/* signals */
{
"                     SIGNALS\n",
"\n",
"    o When the target gets  a  signal  control  returns  to  ups.\n",
"     Depending  on  the  signal and the way you have specified it\n",
"     should be handled, the target is either stopped or restarted\n",
"     (possibly with a display refresh), and the signal can either\n",
"     be passed on to the target or ignored.\n",
"\n",
"     Near the top of the main display area is a  Signals  object.\n",
"     Selecting  this  produces a menu with Expand and Collapse as\n",
"     options.  Expanding the signals object produces  a  list  of\n",
"     all  signals,  with  the  current  way the signal is handled\n",
"     displayed for each signal.  Selecting a  signal  produces  a\n",
"     menu which lets you change the way it is handled.\n",
"\n",
"     You can control whether a given signal causes  ups  to  stop\n",
"     the  target,  refresh the display and continue the target or\n",
"     just continue the target  without  refreshing  the  display.\n",
"     You  can also control whether the signal should be passed on\n",
"     to the target.\n",
"\n",
"    o  UPS  was  coded  to   kill   its   target   on   receiving\n",
"     SIGKILL,SIGSEGV  or  SIGBUS.  Some  programs  have exception\n",
"     handlers that allow the program to continue to  run  despite\n",
"     such  violations.  UPS  was  modified  so that if SIGSEGV or\n",
"     SIGBUS are changed from the default of `Stop - ignore signal\n",
"     on continue' to `Stop - accept signal on continue', UPS will\n",
"     stop on the exception, but allow the target to continue run-\n",
"     ning upon pressing 'cont', `next' or `step'.\n",
"\n",
"    o SIGSEGV or SIGBUS signals that intercepted by  third  party\n",
"     software  such  as ObjectStore can be handled by setting the\n",
"     signal to `accept and continue'.  Do  not  use  `ignore'  as\n",
"     then  the target never gets the signal and it will appear to\n",
"     hang. However, when set to `accept and continue', the target\n",
"     can  crash  on bad code, but UPS will not catch it. For such\n",
"     cases, set a breakpoint in the a target's signal handler (if\n",
"     it has one), or change `accept' to `stop'.\n",
0,
};

static const char *help_level_13A[]=	/* ups version */
{
"                     UPS VERSION\n",
"\n",
"     Press control-V  to show the version  number and build date.\n",
"     The help information was last updated for version 3.37.\n",
0,
};

static const char *help_level_13B[]=	/* ups verison */
{
"                     `STEP' ACTION\n",
"\n",
"    o If the line to be executed calls a function, Step takes you\n",
"     to  the first line of the called function, and stepping con-\n",
"     tinues in the function.  The step action may take some  time\n",
"     occasionally;  however the Stop command can be used to break\n",
"     out such a situation.  If you don't want to step through the\n",
"     code  of called functions in this way, use the Next command.\n",
"     This behaves like Step, except  that  it  never  steps  into\n",
"     called functions.\n",
"\n",
"    o Both Next and  Step work  with  respect  to  the  currently\n",
"     displayed  source.   If you click on a function in the stack\n",
"     trace and select Source to display its source, a  subsequent\n",
"     Next or Step moves to the next line of the displayed source.\n",
"     This makes it easy to get out of a function  that  you  have\n",
"     stepped  into by accident and don't wish to step all the way\n",
"     through.  Use the Source command to display  the  source  of\n",
"     the calling function, then use Next or Step.\n",
"\n",
0,
};

static const char *help_level_13C[]=	/* scrollbars */
{
"                     SCROLLBARS\n",
"\n",
"    o Once a few objects have been added  to  the  display  area,\n",
"     there  is  usually not enough room to display all of them at\n",
"     once.  There is a scroll bar to the left of the display area\n",
"     which  lets  you  scroll  the  display area up and down.  To\n",
"     scroll, press and hold down the  left  mouse  button  whilst\n",
"     within  the  scroll bar, and move the mouse in the direction\n",
"     you wish the display to move.   The  further  you  move  the\n",
"     mouse, the faster the scrolling.\n",
"\n",
"     The black blob in the scroll bar represents  the  proportion\n",
"     of  the  entire  display  that is currently visible, and the\n",
"     position of this visible part within the whole display.  For\n",
"     example,  if  the  black blob is one third the height of the\n",
"     scroll bar, and in the  middle,  it  means  that  the  total\n",
"     height of the objects is about three times the height of the\n",
"     display area,  and  the  middle  third  is  currently  being\n",
"     displayed.\n",
"\n",
"     You can also use the scroll bar to go directly  to  a  given\n",
"     point  in  the  display.  Press and release the middle mouse\n",
"     button at a point in the scroll  bar.   The  black  blob  is\n",
"     moved  so  that it centres around the point, and the display\n",
"     is moved correspondingly.\n",
"\n",
"     You can also use the left and right mouse buttons to page up\n",
"     and  down  through  the  display in the same way as with the\n",
"     xterm(1) scroll bar.  Clicking the left mouse button in  the\n",
"     scroll  bar pages the display down.  Similarly, clicking the\n",
"     right button pages  the  display  up.   The  distance  paged\n",
"     depends  on how far the cursor is from the top of the scroll\n",
"     bar.\n",
"\n",
"     There is a X resource which controls the scrollbar behavior.\n",
"     If `ScrollbarType' is set to anything other that `UPS', such\n",
"     as `MOTIF', then scrolling will move  the  viewport  in  the\n",
"     opposite  direction  to  the  mouse movement, this being the\n",
"     convention used by most X toolkits (for better or worse).,\n",
0,
};

static const char *help_level_13D[]=	/* assembler */
{
"                     ASSEMBLER\n",
"\n",
"     UPS   allows  functions  to   be   disassembled   with   the\n",
"     `$debug:asmsrc  function_name'  command  in the typing line.\n",
"     This dumps an assembler listing of the specified function to\n",
"     \"asm.s\"  in  the  current  directory, with interlaced source\n",
"     lines If no function name is given, the whole binary will be\n",
"     dumped.  The `$debug:asm' command works similarly, but omits\n",
"     the source.\n",
"\n",
"     This is sometimes a valuable tool: it can be used to  deter-\n",
"     mine  what  a  #define value really was when making a binary\n",
"     for instance.\n",
0,
};

static const char *help_level_13E[]=	/* AUTHORS */
{
"                     AUTHORS\n",
"\n",
"     UPS was written by Mark Russell, while at the University  of\n",
"     Kent, UK.  Original version for the ICL Perq and many of the\n",
"     important ideas by John Bovey, University of Kent.\n",
"\n",
"     Rod Armstrong, Schlumberger Tech,  (rod@San-Jose.tt.slb.com)\n",
"     provided:  support  for  C++; symbol table reading fixes for\n",
"     compilers from Sun (SC4.2), Centerline (clcc and cfront/CC),\n",
"     and  GNU  (g++  and  gcc);  port for Linux ELF; various user\n",
"     interface extensions and bug fixes.\n",
"\n",
"     Ian  Edwards  (ian@concerto.demon.co.uk)   contributed   the\n",
"     FreeBSD  port;  support for DWARF symbol tables;  bug fixes.\n",
"     He maintains the UPS web site at http://ups.sourceforge.net/\n",
"\n",
"     Russ Browne, Applied MicroSystems (russ@amc.com) added  han-\n",
"     dling  of  symbol table information concerning base classes,\n",
"     vector tables, and static  class  members  in  SC4  and  g++\n",
"     object  files.  He also contributed the UPS_FORMATS environ-\n",
"     ment variable, control characters in UPS_F*STRs, bumping  of\n",
"     array indicies on duplication, and elastic formating of file\n",
"     names in the  stack  and  breakpoint  list.   The  formating\n",
"     feature allows the file names to be visible for large fonts,\n",
"     and when the debugger window is made quite narrow.  He  also\n",
"     did  the  extended  double  click  accelerators; typing line\n",
"     shortcuts; shading of  inactive  and  disabled  breakpoints;\n",
"     added  X  resources  for  multiclick time, and source window\n",
"     menu control; added a command line option to  force  ups  to\n",
"     pass  the  full name of the target executable; and a fix for\n",
"     the size of \"bool\" data types;\n",
"\n",
"     Tom Hughes (thh@cyberscience.com) contributed: double click-\n",
"     ing  on  a  variable in the source window; calling functions\n",
"     from expressions; improvements for long long types; fix  for\n",
"     parameter widening in the stack; saving and restoring termi-\n",
"     nal state; adding clipping routines to eliminate flicker  on\n",
"     redraw;  for x86 machines, stepping across shared libraries,\n",
"     and handling of frameless stacks; main() can be in a  shared\n",
"     library; support for DT_RUNPATH; better handling for segv on\n",
"     Linux machines; pre-processor macro support; bug fixes.\n",
"\n",
"     C  Daniel  M.  Quinlan  (danq@lemond.colorado.edu)  supplied\n",
"     fixes  for  displaying  Structures  and unions, some fortran\n",
"     variables and a fix for \" unknown type\" in scanning SC4 sym-\n",
"     bol tables.\n",
"\n",
"     Callum Gibson (callum@bain.oz.au) contributed the save state\n",
"     code for signals.\n",
0,
};

/*  Display the popup menu described by po at x,y in window wn,
 *  do the loop to allow the user to select from it, the remove
 *  the menu.
 *
 *  Return the index of the item selected, or -1 if no item
 *  was selected.
 */
int
select_from_popup(wn, name, button, po, x, y)
int wn, button;
const char *name;
popup_t *po;
int x, y;
{
	font_t *menufont;
	int md, command, mask, w, maxw;
	const char **p_str;
	int style = MM_DRAG;

	menufont = Mstdfont();
	md = po->po_mdesc;
	if (md == -1) {
		maxw = 0;
		for(p_str = po->po_caps; *p_str != NULL; p_str++)
			if ((w = wn_strwidth(*p_str, menufont)) > maxw)
				maxw = w; 
		if ( po->po_left)
		    style |= MM_LEFT;
		md = Mmake(name, po->po_caps, (int *)NULL,
			   (int *)NULL, style, maxw);
		if (md == -1)
			panic("mmake failed in select_from_popup");
		Msize(md, maxw+10, (p_str - po->po_caps) *
						     (menufont->ft_height + 4));
		Mfmodes(md, MH_GREY, MH_BLACK|MH_BOLD, MH_GREY);
	}
	mask = ~(1 << md);

	Mplace(md, x - 10, y - po->po_last * (menufont->ft_height + 4) -
					         (menufont->ft_height + 2) / 2);

	/* Force the popup to be inside the main window */
        Mposition_popup(md, wn, &x, &y, TRUE);
	Mdisplay(md, wn, TRUE); 
	Mselect(x, y, wn, MS_PRESSED, mask);
	while (wn_getpuck(wn, &x, &y) & button)
		(void) Mselect(x, y, wn, MS_CONTINUE, mask);
	command = MS_rv(Mselect(x, y, wn, MS_RELEASED, mask)) - 1;
	Mremove(md);
	if (command != -1 && po->po_save_last)
		po->po_last = command;
	po->po_mdesc = md;
	return command;
}

/*  Display open menu md in all of window wn.
 */
void
show_menu(md, wn)
int md, wn;
{
	int w, h;

	wn_get_window_size(wn, &w, &h);
	Mplace(md, 0, 0);
	Msize(md, w, h);
	Mdisplay(md, wn, FALSE);
}

#define INITIAL_OBJTYPE	OT_NO_TYPE

static int Cur_objtype = INITIAL_OBJTYPE;

static dmu_state_t Dynamic_menu_updating_state = DMU_ON;

void
updating_callback_func(oldstate, newstate)
int oldstate, newstate;
{
	if (oldstate != newstate) {
		dmu_state_t dmu_state;

		dmu_state = (newstate == OBJ_UPDATING_ON) ? DMU_ON : DMU_OFF;
		set_dynamic_menu_updating_state(dmu_state);
	}
							
}

static void
sync_dynamic_menu_with_cur_objtype()
{
	static int old_cur_objtype = INITIAL_OBJTYPE;

	if (Cur_objtype != old_cur_objtype) {
		if (Cur_objtype == OT_NO_TYPE)
			set_dynamic_menu(-1, (const char *)NULL);
		else
			set_dynamic_menu(Objtab[Cur_objtype].ot_md,
					 Objtab[Cur_objtype].ot_menuname);
		old_cur_objtype = Cur_objtype;
	}
}

void
set_bphead_menu_state(enable)
     bool enable;
{
  set_dynamic_bphead_menu_state(Objtab[Cur_objtype].ot_md, enable);
}

void
set_wphead_menu_state(enable)
     bool enable;
{
  set_dynamic_wphead_menu_state(Objtab[Cur_objtype].ot_md, enable);
}

void
set_dynamic_menu_updating_state(new_state)
dmu_state_t new_state;
{
	Dynamic_menu_updating_state = new_state;
	if (new_state == DMU_ON)
		sync_dynamic_menu_with_cur_objtype();
}

static void
set_cur_objtype(type)
int type;
{
	Cur_objtype = type;
	if (Dynamic_menu_updating_state == DMU_ON)
		sync_dynamic_menu_with_cur_objtype();
}

int
get_cur_objtype()
{
	return Cur_objtype;
}

/*  Return TRUE if object obj can be selected.
 */
int
can_select(obj)
objid_t obj;
{
	int objtype;

	if (Cur_objtype == OT_NO_TYPE)
		return TRUE;
	
	objtype = ups_get_object_type(obj);
	if (objtype == Cur_objtype)
		return TRUE;
	
	return (objtype == OT_VAR && Cur_objtype == OT_EXPR) ||
	       (objtype == OT_EXPR && Cur_objtype == OT_VAR);
}

/*  Generic selection handling function for all the ups object
 *  types.  Highlighting is simply by inverting foreground and
 *  background colors for the whole object.
 */
void
gen_select(wn, obj, x, y, width, height, flags)
int wn;
objid_t obj;
int x, y, width, height, flags;
{
	int type, nselected;

	if (flags & SEL_VISIBLE) {
	  if (wn_use_alloc_color_for_highlight(0))
	    draw_obj_line(obj, x, y);
	  else
	    wn_invert_area(wn, x, y, width, height);
	}
	if ((flags & SEL_CHANGING) && (nselected = get_num_selected()) <= 1) {
		if (nselected == 0)
			set_cur_objtype(OT_NO_TYPE);
		else if (nselected == 1) {
			type = ups_get_object_type(obj);
			if (type < 0 || type > OT_MAXTYPE)
				panic("bad objtype in gen_select");
			set_cur_objtype(type);
		}
		else
			panic("bad nsel in gs");
	}

	td_record_select(obj, flags);
}

/*  Mousehole caption function for the dynamic menu.  Return 0 (i.e.
 *  no mousehole captions) if there are no objects selected, and hence
 *  no dynamic menu displayed.
 */
int
mfn_dmenu(caps, unused_arg)
int caps;
char *unused_arg;
{
	return (Cur_objtype == OT_NO_TYPE) ? 0 : caps;
}

/*  Input handling function for the dynamic menu.
 */
void
dynamic_menu_func(unused_data, md, command)
char *unused_data;
int md, command;
{
	ot_t *ot;
	int res;
	char *arg = NULL;

	if (Cur_objtype == OT_NO_TYPE || get_num_selected() == 0)
		panic("dmf called with no obj selected");
	if (Cur_objtype < 0 || Cur_objtype > OT_MAXTYPE)
		panic("cur_objtype bad in rd");

	ot = &Objtab[Cur_objtype];

	if (md != -1)
		Mclear(md);
	td_set_obj_updating(OBJ_UPDATING_OFF);

	if (ot->ot_pre_mfunc == NULL) {
		arg = unused_data; /* RGA added for load_library_input */
		res = 0;
	}
	else {
		res = (*ot->ot_pre_mfunc)(command, &arg);
	}

	if (res == 0) {
		sel_t *sel;

		for (sel = get_selection(); sel != NULL;)
		{
		  (*ot->ot_mfunc)(sel->se_code, command, arg);
		  /* RGA need this check to handle selecting a bpt */
		  /* and its duplicate in a tracking file - first */
		  /* call will remove both bpts, and then sel->se_next */
		  /* becomes invalid */
		  if (ot->ot_mfunc == do_bpt && command == MR_BPTMEN_REMOVE)
		    sel = get_selection();
		  else
		    sel = sel->se_next;
		}

		if (ot->ot_post_mfunc != NULL)
			(*ot->ot_post_mfunc)(command, arg);
	}

	if (New_selection != NULL) {
		newsel_t *ns, *next;

		clear_selection();
		for (ns = New_selection; ns != NULL; ns = next) {
			select_object(ns->ns_obj, TRUE, OBJ_SELF);
			next = ns->ns_next;
			free((char *)ns);
		}
		New_selection = NULL;
	}

	td_set_obj_updating(OBJ_UPDATING_ON);
}

void
add_to_new_selection(obj)
objid_t obj;
{
	newsel_t *ns;

	ns = (newsel_t *)e_malloc(sizeof(newsel_t));
	ns->ns_obj = obj;
	ns->ns_next = New_selection;
	New_selection = ns;
}

/*  Input handling function for the target control menu.
 */
/* ARGSUSED */
void
target_menu_func(unused_data, md, command)
char *unused_data;
int md, command;
{
	target_menu_info_t *tm;
	cursor_t old_cursor;
	target_t *xp;

	/* Unselect anything selected in the display area */
	display_area_overlay_unselect_global_selection();

	tm = get_target_menu_info();
	xp = get_current_target();

	tm->tm_current_md = md;
	update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));

	old_cursor = wn_get_window_cursor(WN_STDWIN);
	set_bm_cursor(WN_STDWIN, CU_WAIT);


	target_menu_search_disabled(1, 0); /* set */
	do_menu_target_command(command);
	target_menu_search_disabled(0, 1); /* reset */

	wn_define_cursor(WN_STDWIN, old_cursor);

	tm->tm_current_md = -1;
	Mclear(md);
	xp = get_current_target(); /* RGA: attach may have changed xp */
	update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
	wn_define_cursor(WN_STDWIN, old_cursor); /* RCB: was this missing? */
}

bool
stop_pressed(set, reset)
     int set;
     int reset;
{
  static bool pressed = 0;

  if (set)
    pressed = 1;
  if (reset)
    pressed = 0;
  return(pressed);
}

bool
user_wants_stop(peek_at_event)
bool peek_at_event;
{
	int wn, mask, command, md, exposed, resized;
	int p_x, p_y, e_x, e_y, e_w, e_h;
	bool press;
	target_menu_info_t *tm;
	event_t event;

	tm = get_target_menu_info();
	md = tm->tm_mdtab[(int)TM_STOP].md;
	wn = tm->tm_mdtab[(int)TM_STOP].wn;

	press = wn_expose_or_press_queued
	  (wn, md, &p_x, &p_y, &exposed, &resized, &e_x, &e_y, &e_w, &e_h) == 1;
	if (peek_at_event)
	{
/*	  press = wn_expose_or_press_queued*/
/*	    (wn, md, &x, &y, &exposed, &resized) == 1;*/
	  if (md && (exposed || resized))
	  {
	    int oldstate;
	    oldstate = td_set_obj_updating(OBJ_UPDATING_ON);
	    re_redraw_root_clip(resized ? EV_WINDOW_RESIZED : EV_WINDOW_EXPOSED,
				e_x, e_y, e_w, e_h, TRUE);
	    td_set_obj_updating(oldstate);
	  }
	  if (stop_pressed(0, 0))
	    return TRUE;
	  if (press)
	  {
	    errf("Symbol reading beyond current level aborted ...");
	    Mclear(md);
	    mask = ~(1 << md);
	    Mselect(p_x, p_y, wn, MS_PRESSED, mask);
	  }
	  return (press);
	}
/*	else*/
/*	  wn_next_event(wn, EVENT_MASK, &event);*/
	  if (md && (exposed || resized)) {
/*	if ((event.ev_buttons & B_LEFT) == 0) {*/
/*		if (event.ev_type == EV_WINDOW_EXPOSED ||*/
/*						event.ev_type == EV_WINDOW_RESIZED) {*/
			int i;

/*			re_redraw_root(event.ev_type, FALSE);*/
			re_redraw_root_clip(resized ? EV_WINDOW_RESIZED :
					    EV_WINDOW_EXPOSED, 
					    e_x, e_y, e_w, e_h, TRUE);

			for (i = 0; i < (int)TM_NTAGS; ++i)
				if (tm->tm_mdtab[i].md == tm->tm_current_md)
					break;
			if (i < (int)TM_NTAGS) {
				int msmask;

				if (wn_intersects_rect(tm->tm_mdtab[i].wn, e_x, e_y, e_w, e_h))
				{
				msmask = !(1 << tm->tm_current_md);
				Mselect(1, 1, tm->tm_mdtab[i].wn,
							MS_PRESSED, msmask);
				Mselect(1, 1, tm->tm_mdtab[i].wn,
							MS_RELEASED, msmask);
			      }
			}
/*		}*/
		return FALSE;
	}
	  if (!press)
	    return FALSE;
	
	Mclear(md);
	update_target_menu_state
	  (TS_RUNNING, xp_is_attached(get_current_target()));

	mask = ~(1 << md);
/*	if (!Mselect(event.ev_x, event.ev_y, wn, MS_PRESSED, mask))*/
	if (!Mselect(p_x, p_y, wn, MS_PRESSED, mask))
		return FALSE;

	for (;;) {
		wn_next_event(wn, EVENT_MASK, &event);
		if ((event.ev_buttons & B_LEFT) == 0)
			break;
		Mselect(event.ev_x, event.ev_y, wn, MS_CONTINUE, mask);
		wn_show_updates(wn);
	}
	command = MS_rv(Mselect(event.ev_x, event.ev_y, wn, MS_RELEASED, mask));

	Mclear(md);

	return (command == 'S');
}

/* Get time in micro-seconds after going into run until
 * run_alarm_time_expired() should be called
 */
int
get_run_alarm_time()
{
    switch(raise_on_break)
    {
    case LOWER_ON_RUN:
    case ICONIFY_ON_RUN:
        return lower_time;
    default:
	return 0;
    }
}

/* Called after above alarm has expired.
 * lower or iconify the source window
 */
void
run_alarm_time_expired()
{
    Outwin* ow = (Outwin*)get_current_srcwin();
    switch(raise_on_break)
    {
    case LOWER_ON_RUN:
	outwin_lower(ow);
        lower_time = 0;
	break;
    case ICONIFY_ON_RUN:
	outwin_iconify(ow);
        lower_time = 0;
	break;
    default:
	break;
    }
}

void
run_done(suggest_raise)
int suggest_raise;
{
    if (raise_on_break >= RAISE_ON_BREAK && suggest_raise) 
    {
	Outwin* sw = (Outwin*)get_current_srcwin();
	outwin_raise(sw);
    }
    /* Reset the timer */
    lower_time = lower_time0;
}

void
update_target_menu_state(tstate, attached)
tstate_t tstate;
int attached;
{
	enum {
		START = 1 << TM_START,
		NEXT = 1 << TM_NEXT,
		STEP = 1 << TM_STEP,
		CONT = 1 << TM_CONT,
		STOP = 1 << TM_STOP,
		ATTACH = 1 << TM_ATTACH,
		DETACH = 1 << TM_DETACH,
		KILL = 1 << TM_KILL
	};
	static char nrel[TM_NTAGS];
	static unsigned last_enabled;
	static bool last_have_stack;
	static char ns_str[6];
	target_menu_info_t *tm;
	unsigned enabled, changed;
	int start_menu;
	bool have_stack;
	int i;

	tm = get_target_menu_info();
	if (!tm->tm_current_md ||
	    (target_menu_search_disabled(0, 0) == TRUE &&
	     tstate == TS_SEARCHING))
	  return;

	if (stop_pressed(0, 0))
	{
	  stop_pressed(0, 1);	/* reset */
	  Mclear(tm->tm_mdtab[(int)TM_STOP].md);
	}

	/* RGA if debuger is invoked without a PID, make start */
	/* and attach buttons active. Then user can run a new instance */
	/* or attach to another. Once attached, detach becomes active */
	/* (and start inactive). If invoked with a PID, attach and */
	/* detach are both active. Also allow a detach after a halt */

	switch(tstate) {
	case TS_RUNNING:
	case TS_SEARCHING:	/* allow stop button to stop search */
		enabled = STOP;
		start_menu = get_restart_target_menu();
		nrel[TM_START] = MR_TGT_RESTART;
		break;
	case TS_NOTR:
	case TS_CORE:
		if (attached)
		  enabled = START | ATTACH | DETACH;
		else
		  enabled = START | ATTACH;
		start_menu = get_start_target_menu();
		nrel[TM_START] = MR_TGT_START;
		break;
	case TS_HALTED:
		enabled = START | KILL | DETACH;
		start_menu = get_restart_target_menu();
		nrel[TM_START] = MR_TGT_RESTART;
		break;
	case TS_STOPPED:
		if (attached)
		  enabled = START | NEXT | STEP | CONT | KILL | ATTACH | DETACH;
		else
		  enabled = START | NEXT | STEP | CONT | KILL | ATTACH;
		start_menu = get_restart_target_menu();
		nrel[TM_START] = MR_TGT_RESTART;
		break;
	default:
		panic("unknown target state");
		enabled = 0;	/* to satisfy gcc */
		start_menu = 0;	/* to satisfy gcc */
	}
	
	have_stack = tstate != TS_NOTR;

	wn_updating_off(WN_STDWIN);

	if (nrel[1] == '\0') {
		nrel[TM_START] = MR_TGT_START;
		nrel[TM_NEXT] = MR_TGT_NEXT;
		nrel[TM_STEP] = MR_TGT_STEP;
		nrel[TM_CONT] = MR_TGT_CONT;
		nrel[TM_STOP] = MR_TGT_STOP;
		nrel[TM_ATTACH] = MR_TGT_ATTACH;
		nrel[TM_DETACH] = MR_TGT_DETACH;
		nrel[TM_KILL] = MR_TGT_KILL;
		
		last_enabled = ~enabled;
		last_have_stack = !have_stack;
	}

	changed = last_enabled ^ enabled;
	
	if (tm->tm_mdtab[TM_START].md != start_menu) {
		tm->tm_mdtab[TM_START].md = start_menu;

		set_start_menu(start_menu);

		changed |= START;
	}

	if (tm->tm_current_md == start_menu) {
		Mclear(tm->tm_current_md);
		tm->tm_current_md = -1;
	}
	
	for (i = 0; i < TM_NTAGS; ++i) {
		unsigned mask;
		
		mask = 1 << i;
		
		if (tm->tm_mdtab[i].md != tm->tm_current_md &&
		    (changed & mask) != 0) {
			char ns_str2[2];

			if (enabled & mask) {
				ns_str2[0] = '\0';
				last_enabled |= mask;
			}
			else {
				ns_str2[0] = nrel[i];
				ns_str2[1] = '\0';
				last_enabled &= ~mask;
			}
			
			Mnonsel(tm->tm_mdtab[i].md, ns_str2, 1);
		}
	}

	if (tstate == TS_RUNNING || tstate == TS_SEARCHING)
	{
	  ns_str[0] = MR_SRCWIN_UP_STACK;
	  ns_str[1] = MR_SRCWIN_DOWN_STACK;
	  ns_str[2] = MR_SRCWIN_BACK;
	  ns_str[3] = MR_SRCWIN_SEARCH_FORWARDS;
	  ns_str[4] = MR_SRCWIN_SEARCH_BACKWARDS;
	  Mnonsel(get_current_srcwin_menu(), ns_str, 1);
	}
	else
	{
	  ns_str[0] = '\0';
	  if (have_stack != last_have_stack)
	  {
	    if (!have_stack)
	    {
	      ns_str[0] = MR_SRCWIN_UP_STACK;
	      ns_str[1] = MR_SRCWIN_DOWN_STACK;
	      ns_str[2] = '\0';
	    }
	  }
	  Mnonsel(get_current_srcwin_menu(), ns_str, 1);
	}

	wn_updating_on(WN_STDWIN);

	last_have_stack = have_stack;
}

/*  Menu function for the permanent menu (the quit button).
 */
static int clear_window = 0;
void
permanent_menu_func(unused_data, md, command)
char *unused_data;
int md, command;
{
  const char **str_ptr = NULL;
  char *pattern;
  Outwin *dw;

  switch(command) {
  case MR_QUIT_UPS:
    re_set_exit_event_loop_flag();
    break;
  case MR_DONT_QUIT:
    break;
  case MR_SNAPSHOT_SELECTED:
    dump_selected_objects();
    break;
  case MR_SNAPSHOT_ALL:
    dump_all_objects();
    break;
  case MR_TOGGLE_LOGGING:
    log_messages = !log_messages; /* do the toggle */
    if ( log_messages)
    {
        errf_set_ofunc1(outwin_insert_string);
    } else
    {
        errf_set_ofunc1(null_ofunc);
    }
    break;
  case MR_LOGGING_ON:
    log_messages = TRUE;
    errf_set_ofunc1(outwin_insert_string);
    break;
  case MR_LOGGING_OFF:
    log_messages = FALSE;
    errf_set_ofunc1(null_ofunc);
    break;
  case MR_RAISE_DISPLAY_WIN:
    dw = get_display_area_overlay();
    outwin_raise(dw);
    break;
  case MR_RAISE_SRC_WIN:
    dw = (Outwin*)get_current_srcwin();
    outwin_raise(dw);
    break;
  case MR_RAISE_OUT_WIN:
    dw = get_or_create_outwin();
    outwin_raise(dw);
    break;
  /***
  case MR_CLOSE_OUT_WIN:
    dw = get_current_outwin();
    outwin_unmap(dw);
    break;
  ****/
  case MR_NO_RAISE_ON_BREAK:
    raise_on_break = NO_RAISE;;
    break;
  case MR_RAISE_ON_BREAK:
    raise_on_break = RAISE_ON_BREAK;;
    break;
  case MR_LOWER_ON_RUN:
    raise_on_break = LOWER_ON_RUN;;
    break;
  case MR_ICONIFY_ON_RUN:
    raise_on_break = ICONIFY_ON_RUN;
    break;
  case MR_DISWIN_SEARCH_FORWARDS:
  case MR_DISWIN_SEARCH_BACKWARDS:
    {
      /*  For split screens, search something on the current screen
	*/
      Region* typing_region =  get_typing_line_region();
      int    typing_wn = re_get_wn(typing_region);
      int    ourRoot = wn_get_root_window(typing_wn);
      Region* dspl_region = get_display_area_region();
      int	   dspl_wn = re_get_wn(dspl_region);
      int    dsplRoot = wn_get_root_window(dspl_wn);
      int forwards = command == MR_DISWIN_SEARCH_FORWARDS;
      const char* text = pattern = get_typing_line_string();

	/* Skip initial direction indicator if any */
      if ( *pattern == '/' || *pattern == '?') 
	text++;

      if ( dsplRoot == ourRoot)
      {
	display_area_overlay_unselect_global_selection();
	dw = (Outwin *)re_get_data(dspl_region);
	display_area_overlay_search (dw, text, forwards);
      } else
      {
	Region* src_region = get_current_srcwin_region();
	int	   src_wn = re_get_wn(src_region);
	int    srcRoot = wn_get_root_window(src_wn);
	if ( srcRoot == ourRoot)
	{
	  srcwin_search(get_current_srcwin(), text, forwards);
	} else
	{
	  Outwin* ow = get_current_outwin();
	  if  (ow)
	    outwin_search(ow, text, forwards);
	}
      }
      free(pattern);
      break;
   }
  case MR_HELP_00:
    str_ptr = help_level_00;
    break;
  case MR_HELP_0A:
    str_ptr = help_level_0A;
    break;
  case MR_HELP_0B:
    str_ptr = help_level_0B;
    break;
  case MR_HELP_1A:
    str_ptr = help_level_1A;
    break;
  case MR_HELP_1B:
    str_ptr = help_level_1B;
    break;
  case MR_HELP_1C:
    str_ptr = help_level_1C;
    break;
  case MR_HELP_1D:
    str_ptr = help_level_1D;
    break;
  case MR_HELP_1E:
    str_ptr = help_level_1E;
    break;
  case MR_HELP_1F:
    str_ptr = help_level_1F;
    break;
  case MR_HELP_1G:
    str_ptr = help_level_1G;
    break;
  case MR_HELP_1H:
    str_ptr = help_level_1H;
    break;
  case MR_HELP_1I:
    str_ptr = help_level_1I;
    break;
  case MR_HELP_1J:
    str_ptr = help_level_1J;
    break;
  case MR_HELP_2A:
    str_ptr = help_level_2A;
    break;
  case MR_HELP_2B:
    str_ptr = help_level_2B;
    break;
  case MR_HELP_3A:
    str_ptr = help_level_3A;
    break;
  case MR_HELP_3B:
    str_ptr = help_level_3B;
    break;
  case MR_HELP_3C:
    str_ptr = help_level_3C;
    break;
  case MR_HELP_3D:
    str_ptr = help_level_3D;
    break;
  case MR_HELP_3E:
    str_ptr = help_level_3E;
    break;
  case MR_HELP_3G:
    str_ptr = help_level_3G;
    break;
  case MR_HELP_4:
    str_ptr = help_level_4;
    break;
  case MR_HELP_5A:
    str_ptr = help_level_5A;
    break;
  case MR_HELP_5B:
    str_ptr = help_level_5B;
    break;
  case MR_HELP_5C:
    str_ptr = help_level_5C;
    break;
  case MR_HELP_5D:
    str_ptr = help_level_5D;
    break;
  case MR_HELP_5E:
    str_ptr = help_level_5E;
    break;
  case MR_HELP_5F:
    str_ptr = help_level_5F;
    break;
  case MR_HELP_5G:
    str_ptr = help_level_5G;
    break;
  case MR_HELP_5H:
    str_ptr = help_level_5H;
    break;
  case MR_HELP_5I:
    str_ptr = help_level_5I;
    break;
  case MR_HELP_6A:
    str_ptr = help_level_6A;
    break;
  case MR_HELP_6B:
    str_ptr = help_level_6B;
    break;
  case MR_HELP_6C:
    str_ptr = help_level_6C;
    break;
  case MR_HELP_7:
    str_ptr = help_level_7;
    break;
  case MR_HELP_8:
    str_ptr = help_level_8;
    break;
  case MR_HELP_9:
    str_ptr = help_level_9;
    break;
  case MR_HELP_10:
    str_ptr = help_level_10;
    break;
  case MR_HELP_11A:
    str_ptr = help_level_11a;
    break;
  case MR_HELP_11B:
    str_ptr = help_level_11b;
    break;
  case MR_HELP_11C:
    str_ptr = help_level_11c;
    break;
  case MR_HELP_11D:
    str_ptr = help_level_11d;
    break;
  case MR_HELP_11E:
    str_ptr = help_level_11e;
    break;
  case MR_HELP_11F:
    str_ptr = help_level_11f;
    break;
  case MR_HELP_11G:
    str_ptr = help_level_11g;
    break;
  case MR_HELP_11H:
    str_ptr = help_level_11h;
    break;
  case MR_HELP_12:
    str_ptr = help_level_12;
    break;
  case MR_HELP_13A:
    str_ptr = help_level_13A;
    break;
  case MR_HELP_13B:
    str_ptr = help_level_13B;
    break;
  case MR_HELP_13C:
    str_ptr = help_level_13C;
    break;
  case MR_HELP_13D:
    str_ptr = help_level_13D;
    break;
  case MR_HELP_13E:
    str_ptr = help_level_13E;
    break;
  case MR_HELP_14A:
    clear_window = 0;
    break;
  case MR_HELP_14B:
    clear_window = 1;
    break;
  default:
    panic("bad cmd in pmf");
  }
  
  if (str_ptr)
  {
    size_t buflen = 0;
    Outwin *ow;

    ow = get_or_create_outwin();
    if (clear_window)
      outwin_delete_all_text(ow);
    else
      buflen = outwin_get_buffer_length(ow);

    outwin_insert_string("\n");
    for (; *str_ptr; str_ptr++)
      outwin_insert_string(*str_ptr);
    outwin_insert_string("\n");

    if (clear_window)
      outwin_goto_point_and_update(ow, 0);
    else
      outwin_goto_point_and_update(ow, buflen);
  }
  
  if (md != -1)
    Mclear(md);
}


void
init_menu_toggle_items(md)
int md;
{
	static int called_once = FALSE;
	if (!called_once)
	{
	    const char* res;

	    called_once = TRUE;
	    raise_on_break=NO_RAISE;

	    res = wn_get_default("WantRaiseOnBreak");
	    if ( res && !strcmp(res,"yes"))
		raise_on_break = RAISE_ON_BREAK;

	    res = wn_get_default("WantLowerOnRun");
	    if ( res && !strcmp(res,"yes"))
		raise_on_break = LOWER_ON_RUN;

	    res = wn_get_default("WantIconifyOnRun");
	    if ( res && !strcmp(res,"yes"))
		raise_on_break = ICONIFY_ON_RUN;

	    res = wn_get_default("LowerOnRunTime");
	    if ( res && atoi(res))
	    {
		/* res = time to wait before lower in milliseconds */
		lower_time = lower_time0 = atoi(res)*1000;
	    }

	    res= wn_get_default("WantMessageLogging");
	    log_messages = !(res && !strcmp(res,"yes"));
	    permanent_menu_func(NULL,-1,MR_TOGGLE_LOGGING);
	}

	/* Set message logging as an on/off option */
	Mmaketoggle(md, MR_TOGGLE_LOGGING, &log_messages);

	/* Set up the raise on break options as an exclusive group */
	Mmakegroup(md, (int*)&raise_on_break,
	    MR_NO_RAISE_ON_BREAK, NO_RAISE,
	    MR_RAISE_ON_BREAK,RAISE_ON_BREAK,
	    MR_LOWER_ON_RUN,LOWER_ON_RUN,
	    MR_ICONIFY_ON_RUN,ICONIFY_ON_RUN,
	    0);

	/* Set up the help clear/append options as an exclusive group */
	Mmakegroup(md, &clear_window,
	    MR_HELP_14A,0,
	    MR_HELP_14B,1,
	    0);
}

static void
dump_all_objects()
{
	dump_object((objid_t)NULL, show_in_outwin,
		    (char *)get_or_create_outwin(), OBJ_DESCENDENTS);
}
	
static void
dump_selected_objects()
{
	sel_t *sel;
	Outwin *ow;
	
	sel = get_selection();

	if (sel == NULL) {
		errf("No objects selected");
		return;
	}
	
	ow = get_or_create_outwin();

	for (; sel != NULL; sel = sel->se_next) {
		dump_object(sel->se_code, show_in_outwin, (char *)ow, OBJ_SELF);
		dump_object(sel->se_code, show_in_outwin, (char *)ow,
			    OBJ_DESCENDENTS);
	}
}

static int
show_in_outwin(arg, level, line)
char *arg;
int level;
char *line;
{
	int i;
	Outwin *ow;

	ow = (Outwin *)arg;
	
	for (i = 0; i < level; ++i) {
		static const char spaces[] = "    ";
		
		outwin_insert(ow, spaces, sizeof(spaces) - 1);
	}

	outwin_insert(ow, line, strlen(line));
	outwin_insert(ow, "\n", 1);

	free(line);
        
	return 0;
}

void
get_custom_menu_str(func, ev)
     void (*func)PROTO((int c, bool meta));
     event_t *ev;
{
#define ESC         ('[' & 037)
  static const char *bptcaps_strs[] = {
    "UPS_F1_STR",
    "UPS_F2_STR",
    "UPS_F3_STR",
    "UPS_F4_STR",
    "UPS_F5_STR",
    "UPS_F6_STR",
    "UPS_F7_STR",
    "UPS_F8_STR",
    "UPS_F9_STR",
    "UPS_F10_STR",
    "UPS_F11_STR",
    "UPS_F12_STR",
    NULL
  };
  static const char *bptcaps[13];
  static int i = -1;
  char *env, *c;
  static popup_t bptpop = { -1, FALSE, 0, bptcaps, 0 };
  int res, j;

  if (i == -1)
  {
    for (i = 0, j = 0;j < 12 ; j++)
    {
      env = getenv(bptcaps_strs[j]);
      if (env)
      {
	char *copy;
	int controls = 0;

	/*
	 **   Count the number of control characters in env
	 */
	for ( copy = env; *copy; copy++ )
	  if ( *copy < 0x20) controls++;

	/*
	 **   Now allocate space for the string
	 **   and copy, mapping Control-X to ^X.
	 */
	bptcaps[i] = copy = malloc(strlen(env)+1 + controls);
	for ( ; *env; env++ )
	{
	  if ( *env < 0x20 )
	  {
	    *copy++ = '^';
	    *copy++ = 0x40 | *env;
	  }
	  else
	    *copy++ = *env;
	}
	*copy='\0';
	i++;
      }
    }
    bptcaps[i] = NULL;
  }
  if (i > 0)
  {
    /*  
     **   Allow control chars in paste string.
     */
    int escape = FALSE;
    int control = FALSE;
    int meta = FALSE;
   res = select_from_popup(ev->ev_wn, "CustomMenu", B_RIGHT, &bptpop,
			    ev->ev_x, ev->ev_y);
    switch(res) {
    case 0:
    case 1: 
    case 2: 
    case 3: 
    case 4: 
    case 5: 
    case 6: 
    case 7: 
    case 8: 
    case 9: 
    case 10:
    case 11:
      for (c = (char *)*(bptcaps+res); c && *c; c++) {
	ev->ev_char = *c;
	if ( !escape )
	{
	  if ( control )
	  {
	    ev->ev_char &= 0x1F;
	    control = FALSE;
	  }
	  else if (ev->ev_char == '\\' )
	  {
	    escape = TRUE;
	    continue;
	  }
	  else if (ev->ev_char == '^' )
	  {
	    control = TRUE;
	    continue;
	  }
	  else if (ev->ev_char == '@' )
	  {
	    meta = TRUE;
	    continue;
	  }
	} else
	{
	  switch ( ev->ev_char )
	  {
	  case '@':
	  case '^':
	  case '\\':
	    /*
	     **   Pass \@, \^ or \\ as @, ^ or \ respectively
	     */
	    break;

	  case 'n':
	    /*
	     ** We paste in ESC rather than RETURN
	     ** here because of the check for RETURN
	     ** in the default case in edit_field
	     */
	  case 'e':
	    /*
	     **   If at the end of the buffer, map to ESC to terminate the edit
	     */
	    if ( c[1] == '\0' )
	    {
	      ev->ev_char = ESC;
	      break;
	    }
	    /* else fall into default case */
	  default:
	    /*
	     **    Back up and pass the '\' through
	     */
	    ev->ev_char = '\\';
	    c--;
	  }
	  escape = FALSE;	/* New */
	}
	(*func)(ev->ev_char, meta);
	meta = FALSE;
      }
      break;
    default:
      break;
    }
  }
}

bool 
target_menu_search_disabled(set, reset)
     int set;
     int reset;
{
  static int disabled;

  if (set)
    disabled = set;
  if (reset)
    disabled = 0;

  return(disabled);
    
}
void
indicate_target_menu_search(set)
     int set;
{
  target_t *xp;
  
  xp = get_current_target();

  if (set)
    update_target_menu_state(TS_SEARCHING, xp_is_attached(xp));
  else
    update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
}


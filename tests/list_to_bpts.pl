#!/usr/bin/perl -w

# list_to_bpts.pl
# ---------------
# Convert a list of function names (one per line) into breakpoints to trace
# a program's execution.
#
#  nm myprog | grep -v _start | grep ' T ' | cut -c 12- | list_to_bpts.pl > bpts

while (<>)
{
    chomp;
    printf STDOUT 'breakpoint "%s" {
	                printf ("at %s()\\n");
}
', $_, $_;

}


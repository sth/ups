# $Id$

# -------------------------------------------------------------------------
#  RPM spec file for 'ups'
# -------------------------------------------------------------------------

Summary: A native X Windows based debugger for C, C++ and Fortran 
Name: ups 
Version: 3.36
Release: 1
Copyright: BSD 
Group: Development/Debuggers

# Source: should be - ftp://...
Source: http://www.concerto.demon.co.uk/UPS/src/ups-3.36.tar.gz
Source1: ups.wmconfig
Source2: ups.sh
URL: http://www.concerto.demon.co.uk/UPS/
# Distribution: N/A
# Vendor: N/A
Packager: Ian Edwards <ian@concerto.demon.co.uk>
BuildRoot: /var/tmp/%{name}-%{version}

# From the README
%description 
Ups is a source level C and C++ debugger that runs under X11.
Fortran is also supported on some systems.
 
It runs in a window with two major regions: one showing the
current state of the target program data and the other showing
the currently executing source code.  A key feature of ups is
that the variables display is persistent: when you add a variable
to the display it stays there as you step through the code.  The
current stack trace (which function called which) is always visible.
 
Ups includes a C interpreter which allows you to add fragments
of code simply by editing them into the source window (the source
file itself is not modified).  This lets you add debugging printf
calls without recompiling, relinking (or even restarting) the
target program.  You can also add conditional breakpoints in a
natural way - you just add a statement like "if (i == 73) #stop"
at the appropriate place in the source window.

%prep
# Use standard 'setup'
%setup -n ups-3.36

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
install -m 755 ups/ups $RPM_BUILD_ROOT/%{_bindir}
cp ups/doc/ups.man ups.1
gzip ups.1
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/man/man1
install -m 644 ups.1.gz $RPM_BUILD_ROOT/%{_datadir}/man/man1
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/X11/wmconfig
install -m 644 %{SOURCE1} $RPM_BUILD_ROOT/%{_sysconfdir}/X11/wmconfig
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
install -m 755 %{SOURCE2} $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/usr/lib/X11/app-defaults
install -m 644 Ups $RPM_BUILD_ROOT/usr/lib/X11/app-defaults

%clean
rm -rf $RPM_BUILD_ROOT

%files
# '%doc' puts files in /usr/doc/<package>/...
%doc README CHANGES BUGS FAQ
%{_bindir}/ups
%{_bindir}/ups.sh
%{_datadir}/man/man1/ups.1.gz
%{_sysconfdir}/X11/wmconfig/ups.wmconfig
/usr/lib/X11/app-defaults/Ups

# -------------------------------------------------------------------------


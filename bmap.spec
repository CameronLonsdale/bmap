Summary:	filesystem blockmapper tools
Name:		bmap
Version:	1.0.17
Release:	1
Source:		bmap-1.0.17.tar.gz
Copyright:	GPL
Group:		Brazil
Buildroot:	/var/tmp/bmap-root
Requires:

%description
bmap (and freinds) allow you to perform a variety of interesting operations
on filesystems.

%prep

%setup

%build
make 

%install
export BINDIR=$RPM_BUILD_ROOT/usr/bin
export MANDIR=$RPM_BUILD_ROOT/usr/man
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT $RPM_BUILD_ROOT/usr/bin $RPM_BUILD_ROOT/usr/man
make -e install
rm -rf doc/man

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(-,root,root) %doc README LICENSE COPYING doc/
%attr(755,root,root) /usr/bin/bmap
%attr(755,root,root) /usr/bin/slacker

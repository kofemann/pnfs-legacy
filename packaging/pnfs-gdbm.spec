Summary: Meta package for pnfs server with GDBM DB backend
Name: pnfs-gdbm
Version: 3.1.10
Release: 3
Requires: pnfs
Conflicts: pnfs-postgresql
Provides: pnfs-server
BuildRoot: %{_topdir}/BUILDROOT/%{name}-%{version}
License: Open Source
Group: Applications/System
AutoReqProv: no
%description
The namespace server pnfs for the dCache mass storage software
and some HSMs.

%prep

%build

%install

%clean

%pre

%post

%preun

%files
%defattr(-,root,root)

%changelog

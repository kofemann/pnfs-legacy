Summary: pnfs server with postgresql DB backend
Name: pnfs-postgresql
Version: 3.1.10
Release: 3
Conflicts: pnfs
Provides: pnfs-server
Source0: %{name}.tar
BuildRoot: %{_topdir}/BUILDROOT/%{name}-%{version}
License: Open Source
Group: Applications/System
AutoReqProv: no
%description
The namespace server pnfs for the dCache mass storage software
and some HSMs.

%prep
%setup -c

%build
cd %{name}/tools
for BINARY in linux/*
do
  ln -s $BINARY .
done

%install
mkdir -p $RPM_BUILD_ROOT/opt/pnfs/
cp -a $RPM_PACKAGE_NAME/* $RPM_BUILD_ROOT/opt/pnfs/

mkdir -p $RPM_BUILD_ROOT/opt/pnfs.3.1.10/pnfs/bin/
cat <<HEREDOC > $RPM_BUILD_ROOT/opt/pnfs.3.1.10/pnfs/bin/pnfs
#!/bin/sh
echo "[ERROR] The PostgreSQL version of the PNFS server is now installed."
echo "        Its start-up script is located at /opt/pnfs/bin/pnfs. Exiting."
exit 1
HEREDOC

%clean
rm -rf $RPM_BUILD_ROOT/opt/pnfs
rm -rf $RPM_PACKAGE_NAME

%pre

if grep 'pnfs=/opt/pnfs.3.1.' /usr/etc/pnfsSetup ; then
   echo "[ERROR] Automatic switching between PNFS DB backends not possible."
   echo "        If you are certain there is no GDBM version running,"
   exit "        please remove the file '/usr/etc/pnfsSetup'. Exiting."
   exit 1
fi

exit 0

%post

%preun

if [ "$1" = 0 -a -r /usr/etc/pnfsSetup ] ; then
   . /usr/etc/pnfsSetup
   ${pnfs}/bin/pnfs stop
fi

echo "[WARN]  Make sure you do not try to use the package 'pnfs'" 
echo "        after this (currently being uninstalled) package 'pnfs-postgres' has been used."

exit 0

%files
%defattr(-,root,root)
/opt/pnfs/
/opt/pnfs.3.1.10/pnfs/bin/pnfs

%changelog
* Fri Sep  23 2005 Mathias de Riese <mathias.de.riese@desy.de>
- Initial build.

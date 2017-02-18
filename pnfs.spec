Summary:  pnfs server with postgresql DB backend
Name: pnfs-postgresql
Version: 3.1.10
Vendor: DESY
Release: 7
License: DCache
Group: LCG
Source: %{name}.src.tgz
BuildArch: i386
#BuildArch: x86_64
Prefix:	/opt/pnfs
BuildRoot: %{_tmppath}/%{name}-%{version}
Packager: owen.synge@desy.de


Requires: postgresql

Provides: pnfs-server
Conflicts: pnfs

Obsoletes: pnfs-gdbm

%description
Pnfs name server for for the dCache mass storage software and some HSMs.

%prep

%setup -c

%build
make
make install
echo make deposit prefix=%{buildroot}%{prefix}
make deposit prefix=%{buildroot}%{prefix}

%files
%defattr(-,root,root)

%{prefix}/docs/html/admin.html
%{prefix}/docs/html/backup.html
%{prefix}/docs/html/basics.html
%{prefix}/docs/html/checkBackup.html
%{prefix}/docs/html/download.html
%{prefix}/docs/html/export.html
%{prefix}/docs/html/gettingStarted.html
%{prefix}/docs/html/index.html
%{prefix}/docs/html/info.html
%{prefix}/docs/html/LICENSE.html
%{prefix}/docs/html/logformat.html
%{prefix}/docs/html/moresecurity.html
%{prefix}/docs/html/movedb.html
%{prefix}/docs/html/noio.html
%{prefix}/docs/html/pnfsDirAttr.html
%{prefix}/docs/html/pnfslogo3.gif
%{prefix}/docs/html/remove.html
%{prefix}/docs/html/shrink.html
%{prefix}/docs/html/tags.html
%{prefix}/docs/html/v3.1.10.html
%{prefix}/docs/html/v3.1.3.a.html
%{prefix}/docs/html/v3.1.3.html
%{prefix}/docs/html/v3.1.4.html
%{prefix}/docs/html/v3.1.5.html
%{prefix}/docs/html/v3.1.8.html
%{prefix}/docs/html/v3.1.9.html
%{prefix}/docs/html/worms.html
%{prefix}/docs/notes.problems
%{prefix}/etc/pnfs_config.template

%defattr(0755,root,root)
%{prefix}/bin/pnfs
%{prefix}/install/pnfs-install.sh
%{prefix}/tools/autoinstall-s.sh
%{prefix}/tools/backupConfig
%{prefix}/tools/checkDots
%{prefix}/tools/checkTags
%{prefix}/tools/dbserver
%{prefix}/tools/dbserverWatchdog
%{prefix}/tools/heartbeat
%{prefix}/tools/heartbeatBackup
%{prefix}/tools/install-s.sh
%{prefix}/tools/linux/dbserver
%{prefix}/tools/linux/md2tool
%{prefix}/tools/linux/md3tool
%{prefix}/tools/linux/pmountd
%{prefix}/tools/linux/pnfsd
%{prefix}/tools/linux/sclient
%{prefix}/tools/linux/shmcom
%{prefix}/tools/md
%{prefix}/tools/md2tool
%{prefix}/tools/md3tool
%{prefix}/tools/mdb
%{prefix}/tools/mdconfig
%{prefix}/tools/mdcreate
%{prefix}/tools/observer.sh
%{prefix}/tools/packup
%{prefix}/tools/packup.sof
%{prefix}/tools/pathfinder
%{prefix}/tools/pcpattr
%{prefix}/tools/pexports
%{prefix}/tools/pflags
%{prefix}/tools/platform
%{prefix}/tools/playout
%{prefix}/tools/plog
%{prefix}/tools/pls
%{prefix}/tools/pmount
%{prefix}/tools/pmountd
%{prefix}/tools/pnewpool
%{prefix}/tools/pnfs
%{prefix}/tools/pnfsd
%{prefix}/tools/pnfsFastBackup
%{prefix}/tools/pnfsHeartbeat
%{prefix}/tools/pnfsHour
%{prefix}/tools/pnfsMinute
%{prefix}/tools/pnfsObserver
%{prefix}/tools/pnfsperf.sh
%{prefix}/tools/pnfs.server
%{prefix}/tools/pnfstools
%{prefix}/tools/pshowmounts
%{prefix}/tools/ps.linux.sed
%{prefix}/tools/ps.sed
%{prefix}/tools/ptools
%{prefix}/tools/scandir.sh
%{prefix}/tools/sclient
%{prefix}/tools/setversion
%{prefix}/tools/shmcom
%{prefix}/tools/showlog
%{prefix}/tools/smd
%{prefix}/tools/special

%clean
rm -rf $RPM_BUILD_ROOT


Summary: A utility for scanning a PNFS instance
Name: pnfs-dump
Version: 1.0.11
Release: 1
Requires: pnfs-server
#Provides: pnfs-dump
Provides: dcache-namespace-dump
Source0: pnfs-postgresql.src.tgz
BuildRoot: /var/tmp/%{name}-%{version}
License: Restricted
Group: Applications/System
AutoReqProv: yes
%description
The pnfsDump tool provides the ability to scan a PNFS instance and
provide the result in various output formats.  It works independantly
from the NFS daemons and gives much higher performance.

One of the output formats is a series of SQL statements that recreate
the same filesystem within Chimera.  This allows migration of a PNFS 
namespace to Chimera.


%prep
%setup -c

%build
rm version
ln -s version.linux version
make -C dump
make -C dump install

%install
mkdir -p $RPM_BUILD_ROOT/opt/pnfs/tools
mkdir -p $RPM_BUILD_ROOT/opt/pnfs/docs
mkdir -p $RPM_BUILD_ROOT/opt/pnfs/share/sql

cp -a tools/pnfsDump       $RPM_BUILD_ROOT/opt/pnfs/tools
cp -a docs/README.pnfsDump $RPM_BUILD_ROOT/opt/pnfs/docs
cp -a sql/prep-chimera-for-migration.sql $RPM_BUILD_ROOT/opt/pnfs/share/sql/prep-chimera-for-migration.sql


%clean
rm -rf $RPM_BUILD_ROOT
# I think the following two lines should be some macro
cd $RPM_BUILD_DIR
rm -rf $RPM_PACKAGE_NAME-$RPM_PACKAGE_VERSION


%files
%attr(0755, root, root)       /opt/pnfs/tools/pnfsDump
%attr(0644, root, root) %doc  /opt/pnfs/docs/README.pnfsDump
%attr(0644, root, root)       /opt/pnfs/share/sql/prep-chimera-for-migration.sql

%changelog
* Mon Mar 16 2009 Paul Millar <paul.millar@desy.de>
- pnfsDump will retry three times if a database operation times-out
- statistics are kept and reported about the number of database problems.

* Wed Feb 18 2009 Paul Millar <paul.millar@desy.de>
- Remove unreliable ".(tags)()"-based test in verify.

* Wed Feb 18 2009 Paul Millar <paul.millar@desy.de>
- Allow reporting of AL and RP in XML output.
- Improve warning when unknown level-2 token is encountered.
- Allocate read-ahead buffer off heap, preventing stack overflow.
- Fix numerical values of AL and RP in Chimera output.

* Tue Feb 10 2009 Paul Millar <paul.millar@desy.de>
- Read dCache system-wide default AL & RP from dCache config
- Add support for emitting AL & RP in syncat output
- Add support for emitting AL & RP in files output.
- Add support for emitting whether file is stored in HSM in files output.
- Improve error messages
- Add a new check in verify output: number of tags in a directory.
- Add support for Chimera v1 and v2 schemata.

* Fri Jan 14 2009 Paul Millar <paul.millar@desy.de>
- Add support for migrating stored access-latency/retention-policy information
- Misc. speed-ups.

* Wed Dec 10 2008 Paul Millar <paul.millar@desy.de>
- Fix Chimera output for pseudo-primary tags.
- Fix verify to always check inherited tags.
- Fix some bugs when checking for orphaned tags.
- Clarify outstanding issue with Chimera SQL and inherited tags.

* Mon Dec  8 2008 Paul Millar <paul.millar@desy.de>
- Switch to using a stored procedure to insert/update primary tags.

* Wed Dec  3 2008 Paul Millar <paul.millar@desy.de>
- Add support for LFS (> 2GiB) output via -o option
- Add support for parsing uc attributes in level-2 metadata.
- Add consistency checking for multiple checksums of the same type.

* Mon Nov 24 2008 Paul Millar <paul.millar@desy.de>
- Enable chimera-migration and MD5 outputs.

* Fri Sep 26 2008 Paul Millar <paul.millar@desy.de>
- Fix wrongly reported filesize bug,
- Fix bug where duration of the dump is wrongly reported in the formatted version,
- Improve consistency checking,
- Add "paranoid mode" for increased sanity checking at the expense of longer run-time.

* Wed Sep 17 2008 Paul Millar <paul.millar@desy.de>
- Report correct filesize for files greater than 2GiB,
- Add ability to report file checksum(s),
- Improve dump duration report,
- Improve documentation.

* Thu Sep  4 2008 Paul Millar <paul.millar@desy.de>
- Initial build.

PACKAGE_NAME=pnfs-postgresql
TOOLDIRS=dbfs dbserver nfs dump
OBJDIRS=fs shmcom
ALLDIRS=$(OBJDIRS) $(TOOLDIRS)
prefix=/usr/local/pnfs

all:
	./platform; for d in $(TOOLDIRS) ; do cd $$d; make ; cd ..;done

clean:
	./platform; for d in $(ALLDIRS) ; do cd $$d; make clean;cd ..; done
install:
	./platform install ;\
	for d in $(ALLDIRS) ; do cd $$d; make install;cd ..; done

deposit: all
	./platform install ;\
	for d in $(ALLDIRS) ; do cd $$d; make deposit prefix=$(prefix) ; cd ..; done
	@mkdir -p $(prefix)/bin/
	@mkdir -p $(prefix)/docs/html/
	@mkdir -p $(prefix)/etc/
	@mkdir -p $(prefix)/install/
	@mkdir -p $(prefix)/tools/
	@mkdir -p $(prefix)/share/sql/
	@install -m 0644 etc/pnfs_config.template   $(prefix)/etc
	@install -m 0644 tools/autoinstall-s.sh     $(prefix)/tools
	@install -m 0644 install/pnfs-install.sh    $(prefix)/install
	@install -m 0644 bin/pnfs                   $(prefix)/bin
	@install -m 0644 docs/notes.problems        $(prefix)/docs/
	@install -m 0644 docs/README.pnfsDump       $(prefix)/docs/
	@install -m 0644 docs/html/*.gif            $(prefix)/docs/html/
	@install -m 0644 docs/html/*.html           $(prefix)/docs/html/
	@install -m 0644 sql/prep-chimera-for-migration.sql $(prefix)/share/sql
	@install -m 0744 tools/dbserver             $(prefix)/tools
	@install -m 0744 tools/backupConfig         $(prefix)/tools
	@install -m 0744 tools/checkDots            $(prefix)/tools
	@install -m 0744 tools/checkTags            $(prefix)/tools
	@install -m 0744 tools/dbserverWatchdog     $(prefix)/tools
	@install -m 0744 tools/heartbeat            $(prefix)/tools
	@install -m 0744 tools/heartbeatBackup      $(prefix)/tools
	@install -m 0744 tools/install-s.sh         $(prefix)/tools
	@install -m 0744 tools/md                   $(prefix)/tools
	@install -m 0744 tools/mdb                  $(prefix)/tools
	@install -m 0744 tools/mdconfig             $(prefix)/tools
	@install -m 0744 tools/mdcreate             $(prefix)/tools
	@install -m 0744 tools/observer.sh          $(prefix)/tools
	@install -m 0744 tools/packup               $(prefix)/tools
	@install -m 0744 tools/packup.sof           $(prefix)/tools
	@install -m 0744 tools/pathfinder           $(prefix)/tools
	@install -m 0744 tools/pcpattr              $(prefix)/tools
	@install -m 0744 tools/pexports             $(prefix)/tools
	@install -m 0744 tools/pflags               $(prefix)/tools
	@install -m 0744 tools/platform             $(prefix)/tools
	@install -m 0744 tools/playout              $(prefix)/tools
	@install -m 0744 tools/plog                 $(prefix)/tools
	@install -m 0744 tools/pls                  $(prefix)/tools
	@install -m 0744 tools/pmount               $(prefix)/tools
	@install -m 0744 tools/pnewpool             $(prefix)/tools
	@install -m 0744 tools/pnfs                 $(prefix)/tools
	@install -m 0744 tools/pnfs.server          $(prefix)/tools
	@install -m 0744 tools/pnfsDump             $(prefix)/tools
	@install -m 0744 tools/pnfsFastBackup       $(prefix)/tools
	@install -m 0744 tools/pnfsHeartbeat        $(prefix)/tools
	@install -m 0744 tools/pnfsHour             $(prefix)/tools
	@install -m 0744 tools/pnfsMinute           $(prefix)/tools
	@install -m 0744 tools/pnfsObserver         $(prefix)/tools
	@install -m 0744 tools/pnfsperf.sh          $(prefix)/tools
	@install -m 0744 tools/pnfstools            $(prefix)/tools
	@install -m 0744 tools/ps.linux.sed         $(prefix)/tools
	@install -m 0744 tools/ps.sed               $(prefix)/tools
	@install -m 0744 tools/pshowmounts          $(prefix)/tools
	@install -m 0744 tools/ptools               $(prefix)/tools
	@install -m 0744 tools/scandir.sh           $(prefix)/tools
	@install -m 0744 tools/setversion           $(prefix)/tools
	@install -m 0744 tools/showlog              $(prefix)/tools
	@install -m 0744 tools/smd                  $(prefix)/tools
	@install -m 0744 tools/special              $(prefix)/tools	



tags:
	ctags dbfs/*.[ch] dbserver/*.[ch] nfs/*.[ch] shmcom/*.[ch]


dist:
	make clean
	@mkdir -p build
	tar --gzip --exclude='*.svn*' -cf build/$(PACKAGE_NAME).src.tgz *


rpm: dist
	@rpmbuild -ta build/$(PACKAGE_NAME).src.tgz 

.PHONY: install

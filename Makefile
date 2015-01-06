
#
# USAGE:
#
# make <option>
#
# to see a list of all options, enter 'make' without any options
#
# Note! on some systems do 'gmake' instead of 'make' (most BSD)
#
# for 'make install' you have to login as 'root' else do 'make install_home'
#
# to make the distribution archive:  'make dist'
#
# to clean up (delete .o *~ core etc.):  'make clean'
#
#
# Compile flags used:
#
# NANOSLEEP enables capability to return time in milli secs in 'time'
# READLINE enables commandline editing and history, requires readline lib
# NOIMPPORT disables the 'import' shared library import primitive
#
# Regular expressions now on all platforms Perl Compatible Regular Expresssions PCRE
# see http://www.pcre.org. PCRE can be localized to other languages than English
# by generating different character tables, see documentation at www.pcre.org
# and file LOCALIZATION for details
#

VERSION = 9.1.0
INT_VERSION = 9100

default:
	./build

help:
	@echo "Do one of the following:"
	@echo
	@echo "  make linux           # newlisp for LINUX (tested Debian & Fedora)"
	@echo "  make linux_utf8      # newlisp for LINUX UTF-8"
	@echo "  make linux_readline  # newlisp for LINUX with readline support"
	@echo "  make linux_debian    # newlisp for LINUX with readline support for debian"
	@echo "  make linux_utf8_readline  # newlisp for LINUX UTF-8 with readline support"
	@echo "  make linux_lib       # newlisp.so as shared library for LINUX"
	@echo "  make linux_lib_utf8  # newlisp.so as shared library for LINUX with UTF-8"
	@echo "  make linux64ILP32    # newlisp for LINUX 64 with 32-bit pointers / AMD64"
	@echo "  make linux64LP64     # newlisp for LINUX 64 with 64-bit pointers / AMD64"
	@echo "  make tru64           # newlisp for HP tru64 with 32 bit pointers - read doc/TRU64BUILD"
	@echo "  make bsd             # newlisp for FreeBSD and OpenBSD"
	@echo "  make netbsd          # newlisp for NetBSD (same as previous w/o readline)"
	@echo "  make bsd_lib         # newlisp.so as shared library for FreeBSD, OpenBSD, NetBSD"
	@echo "  make osx             # newlisp for Mac OSX v.10.2" 
	@echo "  make osx_fink        # newlisp for Mac OSX v.10.3 or later, readline support via fink"
	@echo "  make osx_utf8        # newlisp for Mac OSX v.10.2/3 UTF-8 support"
	@echo "  make osx_lib         # newlisp for Mac OSX v.10.2 or later as shared library"
	@echo "  make osxLP64         # newlisp for Mac OSX v.10.3 or later no readline support LP64" 
	@echo "  make darwin          # newlisp for Mac OSX v.10.4 or later, readline support"
	@echo "  make darwin_utf8     # newlisp for Mac OSX v.10.4 or later, readline and UTF-8 support"
	@echo "  make solaris         # newLISP for Sun SOLARIS (tested on Sparc)"
	@echo "  make solarisLP64     # newLISP for Sun SOLARIS 64-bit LP64 (tested on Sparc)"
	@echo "  make solaris_utf8    # newLISP for Sun SOLARIS UTF-8 (tested on Sparc)"
	@echo "  make true64          # newLISP for tru64 UNIX LP64 tested on Alpha CPU"
	@echo "  make mingw           # newlisp.exe for Win32 (MinGW compiler)"
	@echo "  make mingw_utf8      # newlisp.exe for Win32 UTF-8 (MinGW icompiler)"
	@echo "  make mingwdll        # newlisp.dll for Win32 (MinGW compiler)"
	@echo "  make mingwdll_utf8   # newlisp.dll for Win32 UTF-8 (MinGW compiler)"
	@echo "  make os2             # newlisp for OS/2 GCC 3.3.5 with libc061.dll"
	@echo 
	@echo "  make install         # install on LINUX/UNIX in /usr/bin and /usr/share (need to be root)"
	@echo "  make uninstall       # uninstall on LINUX/UNIX from /usr/bin and /usr/share (need to be root)"
	@echo "  make install_home    # install on LINUX/UNIX in users home directory "
	@echo "  make uninstall_home  # uninstall on LINUX/UNIX from users home directory "
	@echo
	@echo "  make clean           # remove all *.o and .tar files etc. USE BETWEEN FLAVORS!"
	@echo "  make test            # run qa-dot, qa-net and qa-xml test scripts
	@echo
	@echo "Note! on some systems use gmake instead of make"
	@echo "If there is no UTF-8 option for your OS, consult makefile_xxx"

linux:
	make -f makefile_linux

linux_utf8:
	make -f makefile_linux_utf8

linux_readline:
	make -f makefile_linux_readline

linux_debian:
	make -f makefile_debian

debian:
	make -f makefile_debian

linux_utf8_readline:
	make -f makefile_linux_utf8_readline

debian_utf8:
	make -f makefile_linux_utf8_readline

linux_lib:
	make -f makefile_linux_lib

linux_lib_utf8:
	make -f makefile_linux_lib_utf8

linux64ILP32:
	make -f makefile_linux64ILP32
	
linux64LP64:
	make -f makefile_linux64LP64

tru64:
	make -f makefile_tru64

bsd:
	make -f makefile_bsd

netbsd:
	make -f makefile_netbsd

bsd_lib:
	make -f makefile_bsd_lib

osx:
	make -f makefile_osx_10.3

osx_fink:
	make -f makefile_osx_10.3_fink

osx_utf8:
	make -f makefile_osx_utf8

osx_lib:
	make -f makefile_osx_lib

osxLP64:
	make -f makefile_osx_LP64

darwin:
	make -f makefile_darwin
	
darwin_utf8:
	make -f makefile_darwin_utf8

solaris:
	make -f makefile_solaris

solarisLP64:
	make -f makefile_solarisLP64

solaris_utf8:
	make -f makefile_solaris_utf8

mingw:
	make -f makefile_mingw

mingw_utf8:
	make -f makefile_mingw_utf8

mingwdll:
	make -f makefile_mingwdll

mingwdll_utf8:
	make -f makefile_mingwdll_utf8

os2: 
	make -f makefile_os2 
	
win32:
	/c/Borland/BCC55/Bin/make -f makefile_win32

win32_utf8:
	/c/Borland/BCC55/Bin/make -f makefile_win32_utf8

win32dll:
	/c/Borland/BCC55/Bin/make -f makefile_win32dll

win32dll_utf8:
	/c/Borland/BCC55/Bin/make -f makefile_win32dll_utf8

winall:
	make clean
	make -f makefile_mingw
	make clean
	make -f makefile_mingwdll
	make clean
	./newlisp qa-dot

# this cleans up the distribution directory for a clean build
clean:
	-rm *~ *.bak *.o *.obj *.map core *.tgz TEST
	-rm newlisp-tk/*~ doc/*~ util/*~ examples/*~ modules/*~
	-rm newlisp-tk/*.bak doc/*.bak util/*.bak examples/*.bak modules/*.bak
	-chmod 644 *.h *.c *.lsp Makefile makefile*
	-chmod 755 build configure examples/*
	-chmod 644 doc/* modules/*.lsp examples/*.lsp examples/*.html
	-chmod 644 newlisp-tk/*
	-chmod 755 newlisp-tk/*.bat newlisp-tk/*.tcl 
	-chmod 755 newlisp-tk/images
	-chmod 644 newlisp-tk/images/*

# run test scripts
test:
	./newlisp qa-dot
	./newlisp qa-xml
	./newlisp qa-setsig
	./newlisp qa-net

# directory definitions
datadir=$(DESTDIR)/usr/share
bindir=$(DESTDIR)/usr/bin
mandir=$(DESTDIR)/usr/share/man

# this is the standard install in /usr/bin and usr/share
# which as to be done as 'root' with supersuser permissions
# for an install in your home directory use make install_home
#
# One-line description for distribution packages: 
# newLISP is a LISP like, general purpose scripting language. 
#
# Longer description for distribution packages: 
# newLISP is a scripting language for developing web applications and programs 
# in general and in the domains of artificial intelligence (AI) and statistics.

install:
	-install -d $(datadir)/doc/newlisp
	-install -d $(datadir)/newlisp/newlisp-tk/images
	-install -m 755 newlisp $(bindir)/newlisp
	-install -m 755 newlisp-tk/newlisp-tk.tcl $(bindir)/newlisp-tk
	-install -m 755 examples/newlispdoc $(bindir)/newlispdoc
	-install -m 644 init.lsp.example $(datadir)/newlisp/init.lsp.example
	-install -m 644 examples/httpd-conf.lsp $(datadir)/newlisp/httpd-conf.lsp
	-install -m 644 examples/link.lsp $(datadir)/newlisp/link.lsp
	-install -m 644 examples/tcltk.lsp $(datadir)/newlisp/tcltk.lsp
	-install -m 755 examples/syntax.cgi $(datadir)/newlisp/syntax.cgi
	-install -m 644 modules/mysql.lsp $(datadir)/newlisp/mysql.lsp
	-install -m 644 modules/mysql5.lsp $(datadir)/newlisp/mysql5.lsp
	-install -m 644 modules/pop3.lsp $(datadir)/newlisp/pop3.lsp
	-install -m 644 modules/ftp.lsp $(datadir)/newlisp/ftp.lsp
	-install -m 644 modules/infix.lsp $(datadir)/newlisp/infix.lsp
	-install -m 644 modules/smtp.lsp $(datadir)/newlisp/smtp.lsp
	-install -m 644 modules/odbc.lsp $(datadir)/newlisp/odbc.lsp
	-install -m 644 modules/cgi.lsp $(datadir)/newlisp/cgi.lsp
	-install -m 644 modules/sqlite3.lsp $(datadir)/newlisp/sqlite3.lsp
	-install -m 644 modules/stat.lsp $(datadir)/newlisp/stat.lsp
	-install -m 644 modules/gmp.lsp $(datadir)/newlisp/gmp.lsp
	-install -m 644 modules/postscript.lsp $(datadir)/newlisp/postscript.lsp
	-install -m 644 modules/unix.lsp $(datadir)/newlisp/unix.lsp
	-install -m 644 modules/zlib.lsp $(datadir)/newlisp/zlib.lsp
	-install -m 644 doc/COPYING $(datadir)/doc/newlisp/COPYING
	-install -m 644 doc/CREDITS $(datadir)/doc/newlisp/CREDITS
	-install -m 644 doc/newlisp_manual.html $(datadir)/doc/newlisp/newlisp_manual.html
	-install -m 644 doc/newlisp_index.html $(datadir)/doc/newlisp/newlisp_index.html
	-install -m 644 doc/manual_frame.html $(datadir)/doc/newlisp/manual_frame.html
	-install -m 644 doc/CodePatterns.html $(datadir)/doc/newlisp/CodePatterns.html
	-install -m 644 doc/newLISPdoc.html $(datadir)/doc/newlisp/newLISPdoc.html
	-install -m 644 doc/newLISP-9.1-Release.html $(datadir)/doc/newlisp/newLISP-9.1-Release.html
	-install -m 644 newlisp-tk/newlisp-tk.html $(datadir)/doc/newlisp/newlisp-tk.html
	-install -m 644 newlisp-tk/*.lsp $(datadir)/newlisp/newlisp-tk/
	-install -m 644 newlisp-tk/images/* $(datadir)/newlisp/newlisp-tk/images/
	-install -m 644 doc/newlisp.1 $(mandir)/man1/newlisp.1
	-install -m 644 doc/newlisp-tk.1 $(mandir)/man1/newlisp-tk.1
	-install -m 644 doc/newlispdoc.1 $(mandir)/man1/newlispdoc.1

uninstall:
	-rm  $(bindir)/newlisp
	-rm  $(bindir)/newlisp-tk
	-rm  -rf $(datadir)/newlisp
	-rm  -rf $(datadir)/doc/newlisp
	-rm  $(mandir)/man1/newlisp.1
	-rm  $(mandir)/man1/newlisp-tk.1

# for install to systems where the usual install dirs are read-only e.g. Knoppix live 
# CD etc. This will install in the home directory of the user, root permissions are 
# not required for directory installation in the home directory


install_home:
	-install -d $(HOME)/bin
	-install -d $(HOME)/share/man/man1
	-install -d $(HOME)/share/newlisp/doc
	-install -d $(HOME)/share/newlisp/newlisp-tk/images
	-install -m 755 newlisp $(HOME)/bin/newlisp
	-install -m 755 newlisp-tk/newlisp-tk.tcl $(HOME)/bin/newlisp-tk
	-install -m 755 examples/newlispdoc $(HOME)/bin/newlispdoc
	-install -m 644 init.lsp.example $(HOME)/share/newlisp/init.lsp.example
	-install -m 644 examples/httpd-conf.lsp $(HOME)/share/newlisp/httpd-conf.lsp
	-install -m 644 examples/link.lsp $(HOME)/share/newlisp/link.lsp
	-install -m 644 examples/tcltk.lsp $(HOME)/share/newlisp/tcltk.lsp
	-install -m 755 examples/syntax.cgi $(HOME)/share/newlisp/syntax.cgi
	-install -m 644 modules/mysql.lsp $(HOME)/share/newlisp/mysql.lsp
	-install -m 644 modules/mysql5.lsp $(HOME)/share/newlisp/mysql5.lsp
	-install -m 644 modules/pop3.lsp $(HOME)/share/newlisp/pop3.lsp
	-install -m 644 modules/ftp.lsp $(HOME)/share/newlisp/ftp.lsp
	-install -m 644 modules/infix.lsp $(HOME)/share/newlisp/infix.lsp
	-install -m 644 modules/smtp.lsp $(HOME)/share/newlisp/smtp.lsp
	-install -m 644 modules/odbc.lsp $(HOME)/share/newlisp/odbc.lsp
	-install -m 644 modules/sqlite3.lsp $(HOME)/share/newlisp/sqlite3.lsp
	-install -m 644 modules/cgi.lsp $(HOME)/share/newlisp/cgi.lsp
	-install -m 644 modules/stat.lsp $(HOME)/share/newlisp/stat.lsp
	-install -m 644 modules/gmp.lsp $(HOME)/share/newlisp/gmp.lsp
	-install -m 644 modules/postscript.lsp $(HOME)/share/newlisp/postscript.lsp
	-install -m 644 modules/zlib.lsp $(HOME)/share/newlisp/zlib.lsp
	-install -m 644 doc/COPYING $(HOME)/share/doc/newlisp/COPYING
	-install -m 644 doc/CREDITS $(HOME)/share/doc/newlisp/CREDITS
	-install -m 644 doc/newlisp_manual.html $(HOME)/share/doc/newlisp/newlisp_manual.html
	-install -m 644 doc/newlisp_index.html $(HOME)/share/doc/newlisp/newlisp_index.html
	-install -m 644 doc/manual_frame.html $(HOME)/share/doc/newlisp/manual_frame.html
	-install -m 644 doc/CodePatterns.html $(HOME)/share/doc/newlisp/CodePatterns.html
	-install -m 644 doc/newLISPdoc.html $(HOME)/share/doc/newlisp/newLISPdoc.html
	-install -m 644 doc/newLISP-9.1-Release.html $(HOME)/share/doc/newlisp/newLISP-9.1-Release.html
	-install -m 644 newlisp-tk/newlisp-tk.html $(HOME)/share/doc/newlisp/newlisp-tk.html
	-install -m 644 newlisp-tk/*.lsp $(HOME)/share/newlisp/newlisp-tk/
	-install -m 644 newlisp-tk/images/* $(HOME)/share/newlisp/newlisp-tk/images/
	-install -m 644 doc/newlisp.1 $(HOME)/share/man/man1/newlisp.1
	-install -m 644 doc/newlisp-tk.1 $(HOME)/share/man/man1/newlisp-tk.1
	-install -m 644 newlisp-tk/newlisp-tk.config.home $(HOME)/newlisp-tk.config


# undo above install

uninstall_home:
	-rm $(HOME)/bin/newlisp
	-rm $(HOME)/bin/newlisp-tk
	-rm -rf $(HOME)/share/newlisp
	-rm -rf $(HOME)/share/doc/newlisp
	-rm $(HOME)/share/man/man1/newlisp.1
	-rm $(HOME)/share/man/man1/newlisp-tk.1 
	-rm $(HOME)/newlisp-tk.config


# this makes the distribution newlisp-x.x.x.tgz from inside newlisp-x.x.x directory
# you shouldn't use this, but send me the changed files with your contribution/fixes 
# to lutz@nuevatec.com put the word: newlisp in the subject line
#
dist:
	-mkdir newlisp-$(VERSION)
	-mkdir newlisp-$(VERSION)/newlisp-tk
	-mkdir newlisp-$(VERSION)/newlisp-tk/images
	-mkdir newlisp-$(VERSION)/modules
	-mkdir newlisp-$(VERSION)/examples
	-mkdir newlisp-$(VERSION)/doc
	-mkdir newlisp-$(VERSION)/util
	cp init.lsp.example nl*.c newlisp.c *.h osx*.c pcre*.c newlisp-$(VERSION)
	cp win3*.* unix*.c newlisp-$(VERSION)
	cp Makefile build configure makefile* qa* newlisp-$(VERSION)
	cp modules/* newlisp-$(VERSION)/modules
	cp examples/* newlisp-$(VERSION)/examples
	cp doc/* newlisp-$(VERSION)/doc
	cp util/* newlisp-$(VERSION)/util
	cp README newlisp-$(VERSION)
	cp -R newlisp-tk/* newlisp-$(VERSION)/newlisp-tk

	tar czvf newlisp-$(VERSION).tgz newlisp-$(VERSION)/*
	tar czvf newlisp-$(VERSION)-modex.tgz newlisp-$(VERSION)/examples/* newlisp-$(VERSION)/modules/*
	rm -rf newlisp-$(VERSION)
	mv newlisp-$(VERSION).tgz ..
	mv newlisp-$(VERSION)-modex.tgz ..
	

# package Win32 newLISP-tk release
# expects /c/freewrap and /c/bwidget

win-tk:
	-mkdir /c/newlisp
	cp newlisp.exe /c/newlisp
	cp newlisp.dll /c/newlisp
	cp newlisp-tk/newlisp-tk.config.win32 /c/newlisp/newlisp-tk.config
	cp newlisp.exe /usr/bin
	cp doc/newlisp_manual.html /c/newlisp
	cp doc/newlisp_index.html /c/newlisp
	cp doc/manual_frame.html /c/newlisp
	cp doc/CodePatterns.html /c/newlisp
	cp doc/newLISPdoc.html /c/newlisp
	cp doc/newLISP-9.1-Release.html /c/newlisp
	cp doc/keywords.txt /c/newlisp
	cp examples/newlispdoc /c/newlisp
	cp examples/httpd-conf.lsp /c/newlisp
	cp util/newlisp.vim /c/newlisp
	cp doc/COPYING /c/newlisp
	cp newlisp-tk/newlisp-tk.html /c/newlisp
	cp newlisp-tk/README.txt /c/newlisp
	cp newlisp-tk/*.lsp /c/newlisp
	cp newlisp-tk/images/newlisp.ico /c/newlisp
	cp modules/stat.lsp /c/newlisp
	cp newlisp-tk/newlisp-tk.nsi /NSIS
	cp newlisp-tk/newlisp-tk.tcl /c/freewrap
	cp newlisp-tk/bwidget.txt /c/freewrap
	cp newlisp-tk/images.txt /c/freewrap
	cp newlisp-tk/make-wrapped-exe.bat /c/freewrap
	
	echo 'cd /c/freewrap' > pack-tk
	echo './make-wrapped-exe.bat' >> pack-tk
	echo 'cp /c/freewrap/newlisp-tk.exe /c/newlisp' >> pack-tk
	echo 'md5 /c/newlisp/*.exe /c/newlisp/*.dll > /c/newlisp/md5-checksums.txt' >> pack-tk
	echo 'cd /NSIS/' >> pack-tk
	echo './makensis newlisp-tk.nsi' >> pack-tk
	chmod 755 pack-tk
	
	./pack-tk
	rm ./pack-tk
	rm -rf /c/newlisp

osx_package:	
	make -f makefile_osx_package

# this changes to the current version number in several files
#
# before doing a 'make version' the VERSION variable at the beginning
# of this file has to be changed to the new number
#
version:
	sed -i.bak -E 's/int version = .+;/int version = $(INT_VERSION);/' newlisp.c
	sed -i.bak -E 's/newLISP v.[[:digit:]]+.[[:digit:]]+.[[:digit:]]+ /newLISP v.$(VERSION) /' newlisp.c
	sed -i.bak -E 's/newLISP v.+ Manual/newLISP v.$(VERSION) Manual/' doc/newlisp_manual.html
	sed -i.bak -E 's/Reference v.+<\/h2>/Reference v.$(VERSION)<\/h2>/' doc/newlisp_manual.html
	sed -i.bak -E 's/newlisp-....-win/newlisp-$(INT_VERSION)-win/' newlisp-tk/newlisp-tk.nsi
	sed -i.bak -E 's/and newLISP .+ on /and newLISP $(VERSION) on /' newlisp-tk/newlisp-tk.nsi

# Prepare the manual file for PDF conversion, byt replaceing all <span class="function"></span>
# with <font color="#DD0000"></font> in the syntax statements and replacing &rarr; (one line
# arrow with &rArr; (double line arrow). This is necessary when using OpenOffcice PDF conversion 
#
preparepdf:
	util/preparepdf doc/newlisp_manual.html doc/newlisp_manual_preparepdf.html
	


# end of file

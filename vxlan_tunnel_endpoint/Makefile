#
# Copryright (C) 2012-2013 NEC Corporation
#

SRCDIR = src
MANDIR = doc

DEBUILD = debuild
DEBCLEAN = debclean

all:
	cd $(SRCDIR) && $(MAKE)
	cd $(MANDIR) && $(MAKE)

install:
	cd $(SRCDIR) && $(MAKE) install
	cd $(MANDIR) && $(MAKE) install

deb:
	$(DEBUILD) -i -us -uc -b

clean:
	cd $(SRCDIR) && $(MAKE) clean
	cd $(MANDIR) && $(MAKE) clean
	$(DEBCLEAN)


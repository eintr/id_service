################ Edit lines below #################

PREFIX=/usr/local/dungeon
SBINDIR=$(PREFIX)/sbin
CONFDIR=$(PREFIX)/conf
LOGDIR=$(PREFIX)/log
MODDIR=$(PREFIX)/rooms

################ DON'T Edit lines below #################

APPNAME=titans_id_service
APPVERSION_MAJ=1
APPVERSION_MIN=0

INSTALL=install

CFLAGS+=-DAPPNAME=\"$(APPNAME)\" -DAPPVERSION_MAJ=\"$(APPVERSION_MAJ)\" -DAPPVERSION_MIN=\"$(APPVERSION_MIN)\" -DCONFDIR=\"$(CONFDIR)\" -DINSTALL_PREFIX=\"$(PREFIX)\" -DMODDIR=\"$(MODDIR)\"


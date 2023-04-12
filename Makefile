UINITPATH=/etc/init
UINITSOCK=/dev/initctl
override CFLAGS+=-std=c89 -Wall -O2 -D_UINIT_BASEPATH=\"$(UINITPATH)\" -D_UINIT_SOCKNAME=\"$(UINITSOCK)\"

all: init shutdown respawn
init: init.c
shutdown: shutdown.c
respawn: respawn.c

clean:
	rm -f init shutdown respawn

install:
	install -m 0755 init $(DESTDIR)/sbin/
	install -m 0755 shutdown $(DESTDIR)/sbin/
	install -m 0755 respawn $(DESTDIR)/sbin/
	rm -f $(DESTDIR)/sbin/reboot
	ln -s shutdown $(DESTDIR)/sbin/reboot
	rm -f $(DESTDIR)/sbin/halt
	ln -s shutdown $(DESTDIR)/sbin/halt
	rm -f $(DESTDIR)/sbin/poweroff
	ln -s shutdown $(DESTDIR)/sbin/poweroff
	rm -f $(DESTDIR)/sbin/telinit
	ln -s init $(DESTDIR)/sbin/telinit
	rm -f $(DESTDIR)/sbin/mkinitctl
	ln -s init $(DESTDIR)/sbin/mkinitctl
	rm -f $(DESTDIR)/bin/respawn
	ln -s ../sbin/respawn $(DESTDIR)/bin/respawn

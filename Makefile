override CFLAGS+=-std=c89 -Wall -O2

all: init shutdown spawn
init: init.c
shutdown: shutdown.c
spawn: spawn.c

clean:
	rm -f init shutdown spawn

install:
	install -m 755 init $(DESTDIR)/sbin
	install -m 755 shutdown $(DESTDIR)/sbin
	install -m 755 spawn $(DESTDIR)/sbin
	rm -f $(DESTDIR)/sbin/reboot
	ln -s shutdown $(DESTDIR)/sbin/reboot
	rm -f $(DESTDIR)/sbin/halt
	ln -s shutdown $(DESTDIR)/sbin/halt
	rm -f $(DESTDIR)/sbin/poweroff
	ln -s shutdown $(DESTDIR)/sbin/poweroff

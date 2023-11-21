/*
 * This code is in public domain
 */

#include "sdefs.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "xstrlcpy.c"
#ifndef _UINIT_SOCKNAME
#define _UINIT_SOCKNAME "/dev/initctl"
#endif

#ifndef _UINIT_BASEPATH
#define _UINIT_BASEPATH "/etc/init"
#endif

typedef void (*sighandler_t)(int);

static void strtoi(char *s, unsigned x)
{
	size_t y = 0, z;
	char t[12];

	memset(s, 0, sizeof(t));
	if (x == 0) {
		s[0] = '0';
		return;
	}
	while (x) {
		t[y] = (x % 10) + '0'; y++;
		x /= 10;
	}
	z = 0;
	while (y) {
		s[z] = t[y-1];
		z++; y--;
	}
}

static unsigned atoui(const char *s)
{
	const char *p = s;
	unsigned x;
	size_t z;

	x = 0; z = 1;
	while (*s) s++;
	s--;
	while (s-p >= 0) {
		if (*s >= '0' && *s <= '9') {
			x += (*s - '0') * z;
			s--; z *= 10;
		}
		else return 0;
	}

	return x;
}

static int setfd_cloexec(int fd)
{
	int x;

	x = fcntl(fd, F_GETFL, 0);
	if (x == -1) return -1;
	return fcntl(fd, F_SETFD, x | FD_CLOEXEC);
}

static int create_ctrl_socket(const char *path)
{
	int rfd, x;
	struct sockaddr_un sun;
	socklen_t sunl;

	memset(&sun, 0, sizeof(struct sockaddr_un));
	sun.sun_family = AF_UNIX;
	if (path[0] != '@') unlink(path);
	xstrlcpy(sun.sun_path, path, sizeof(sun.sun_path));
	if (path[0] == '@') {
		sun.sun_path[0] = '\0';
		sunl = sizeof(sun.sun_family) + strlen(path);
	}
	else sunl = sizeof(struct sockaddr_un);
	rfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (rfd != -1) {
		x = fcntl(rfd, F_GETFL, 0);
		if (x == -1) goto _mfdf;
		if (fcntl(rfd, F_SETFL, x | O_ASYNC) == -1) goto _mfdf;
		if (fcntl(rfd, F_SETOWN, 1) == -1) goto _mfdf;
		if (fcntl(rfd, F_SETSIG, SIGIO) == -1) goto _mfdf;
		if (fchown(rfd, 0, 0) == -1) goto _mfdf;
		if (fchmod(rfd, 0600) == -1) goto _mfdf;
		umask(0077);
		if (bind(rfd, (struct sockaddr *)&sun, sunl) == -1) goto _mfdf;
		if (listen(rfd, 256) == -1) goto _mfdf;
		umask(0022);
		if (fchown(rfd, 0, 0) == -1) goto _mfdf;
		if (fchmod(rfd, 0600) == -1) goto _mfdf;
	}
	else {
_mfdf:		if (rfd != -1) close(rfd);
		rfd = -1;
	}

	return rfd;
}

static int goingdown;
static int ctlfd = -1;

static void signal_handler(int sig)
{
	int clfd, cmd;
	size_t sz;
	sigset_t set;

	cmd = UINIT_CMD_INVALID;

	if (sig == SIGIO) {
		struct ucred cr;
		socklen_t crl;

		if (ctlfd == -1) return;

		clfd = accept(ctlfd, NULL, 0);
		if (clfd != -1) {
			cr.uid = NOUID;
			cr.gid = NOGID;
			cr.pid = 1;
			crl = sizeof(struct ucred);
			if (getsockopt(clfd, SOL_SOCKET, SO_PEERCRED, &cr, &crl) == -1) {
				cmd = UINIT_CMD_INVALID;
			}
			else {
				if (cr.uid == 0 && cr.gid == 0 && cr.pid > 1) {
					sz = (size_t)read(clfd, &cmd, sizeof(int));
					if (sz == NOSIZE) cmd = UINIT_CMD_INVALID;
					if (sz < sizeof(int)) cmd = UINIT_CMD_INVALID;
				}
			}
			close(clfd);
		}
	}
	else if (sig == SIGALRM) {
		if (ctlfd != -1) return;
		ctlfd = create_ctrl_socket(_UINIT_SOCKNAME);
		return;
	}
	else if (sig == SIGINT) {
		cmd = UINIT_CMD_CAD;
	}
	else if (sig == SIGHUP) {
		cmd = UINIT_CMD_SINGLEUSER;
		goto _su;
	}

	if (cmd == UINIT_CMD_INVALID) return;

	if (goingdown) return;

	if (cmd == UINIT_CMD_SINGLEUSER) {
_su:		if (ctlfd != -1) {
			char t[12];
			strtoi(t, ctlfd);
			setenv("UINIT_SOCKFD", t, 1);
		}
		sigfillset(&set);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		execl(_UINIT_BASEPATH "/single", "single", (char *)NULL);
		return;
	}

	if (fork()) {
		if (cmd != UINIT_CMD_POWERFAIL) {
			sigfillset(&set);
			sigprocmask(SIG_BLOCK, &set, NULL);
			goingdown = 1;
			if (ctlfd != -1) {
				shutdown(ctlfd, SHUT_RDWR);
				close(ctlfd);
			}
		}
		return;
	}

	if (ctlfd != -1) setfd_cloexec(ctlfd);

	switch (cmd) {
		case UINIT_CMD_CAD:
		execl(_UINIT_BASEPATH "/cad", "cad", (char *)NULL);
		break;

		case UINIT_CMD_REBOOT:
		execl(_UINIT_BASEPATH "/reboot", "reboot", (char *)NULL);
		break;

		case UINIT_CMD_POWEROFF:
		execl(_UINIT_BASEPATH "/poweroff", "poweroff", (char *)NULL);
		break;

		case UINIT_CMD_SHUTDOWN:
		execl(_UINIT_BASEPATH "/shutdown", "shutdown", (char *)NULL);
		break;

		case UINIT_CMD_POWERFAIL:
		execl(_UINIT_BASEPATH "/powerfail", "powerfail", (char *)NULL);
		break;
	}
}

static char *progname;

static int reopen_console(const char *path)
{
	int fd;

	if ((fd = open(path ? path : "/dev/console", O_RDWR | O_NONBLOCK | O_NOCTTY)) != -1) {
		close(0);
		if (dup2(fd, 0) == -1) return fd;
		close(1);
		if (dup2(fd, 1) == -1) return fd;
		close(2);
		if (dup2(fd, 2) == -1) return fd;
		if (fd > 2) close(fd);
		return 5;
	}
	return -1;
}

int main(int argc, char **argv)
{
	sigset_t set;
	int x;
	char *s;

	umask(0022);
	progname = basename(argv[0]);

	if (!strcmp(progname, "mkinitctl")) {
		kill(1, SIGALRM);
		return 0;
	}

	if (!strcmp(progname, "telinit")) {
		return 0;
	}

	if (getpid() != 1) return 1;

	if (!access(_UINIT_BASEPATH "/altinit", X_OK) && !getenv("_INIT"))
		execl(_UINIT_BASEPATH "/altinit", "init", (char *)NULL);

	s = getenv("CONSOLE");
	x = reopen_console(s ? s : "/dev/console");
	if (x != 5) {
		if (x != -1) write(x, "Unable to open initial console", (sizeof("Unable to open initial console")-1));
		pause();
	}

	s = getenv("UINIT_SOCKFD");
	if (s) {
		ctlfd = atoui(s);
		unsetenv("UINIT_SOCKFD");
		/* we don't need alien fds from the past, recreate our own */
		close(ctlfd);
		ctlfd = -1;
	}

	reboot(RB_DISABLE_CAD);

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	if (fork()) {
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		sigdelset(&set, SIGALRM);
		sigdelset(&set, SIGINT);
		sigdelset(&set, SIGIO);
		sigdelset(&set, SIGCHLD);
		sigprocmask(SIG_BLOCK, &set, NULL);
		signal(SIGALRM, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGIO, signal_handler);
		signal(SIGCHLD, SIG_DFL);
		if (_UINIT_SOCKNAME[0] != '@') alarm(10);

		while (1) {
			if (wait(&x) == -1 && errno == ECHILD) {
				pause();
			}
		}
	}

	if (ctlfd != -1) setfd_cloexec(ctlfd);

	sigprocmask(SIG_UNBLOCK, &set, NULL);

	setsid();
	setpgid(0, 0);
	return execl(_UINIT_BASEPATH "/boot", "boot", (char *)NULL);
}

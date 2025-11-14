/*
 * This code is in public domain
 */

#include "sdefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/reboot.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "xstrlcpy.c"
#include "xstrlcat.c"
#ifndef _UINIT_SOCKPATH
#define _UINIT_SOCKPATH "/dev/initctl"
#endif

static int usage(void)
{
	puts("usage: reboot [-Sw]");
	puts("usage: halt [-Sw]");
	puts("usage: poweroff [-Sw]");
	puts("usage: powerfail [-Sw]");
	puts("usage: singleuser [-Sw]");
	puts("usage: shutdown [-RHPSw]");
	puts("  -R: reboot system immediately");
	puts("  -H: halt system immediately");
	puts("  -P: halt and power off system immediately");
	puts("  -S: do not sync, useless without command");
	return 1;
}

static char *progname;

static void xerror(const char *s)
{
	char t[64];
	xstrlcpy(t, progname, sizeof(t));
	xstrlcat(t, ": ", sizeof(t));
	xstrlcat(t, s, sizeof(t));
	puts(t);
	exit(2);
}

static int rb_xlate(int ucmd)
{
	switch (ucmd) {
		case UINIT_CMD_SHUTDOWN: return RB_HALT_SYSTEM;
		case UINIT_CMD_POWEROFF: return RB_POWER_OFF;
		case UINIT_CMD_REBOOT:   return RB_AUTOBOOT;
		default:		 return -1;
	}
}

static void do_reboot(int cmd)
{
	int r;

	cmd = rb_xlate(cmd);
	r = reboot(cmd);
	if (r == -1 && errno == EPERM) xerror("must be superuser");
}

static int checkids(void)
{
	return (geteuid() == 0 && getegid() == 0);
}

static int validate_response(int rsp)
{
	if (rsp >= UINIT_RSP_OK && rsp <= UINIT_RSP_DENIED) return 1;
	return 0;
}

static int send_init_cmd(const char *path, int cmd)
{
	struct sockaddr_un sun;
	socklen_t sunl;
	int clfd, rsp, scmd = cmd;
	size_t sz;

	memset(&sun, 0, sizeof(struct sockaddr_un));
	sun.sun_family = AF_UNIX;
	xstrlcpy(sun.sun_path, path, sizeof(sun.sun_path));
	if (path[0] == '@') {
		sun.sun_path[0] = '\0';
		sunl = sizeof(sun.sun_family) + strlen(path);
	}
	else sunl = sizeof(struct sockaddr_un);
	clfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (clfd == -1) return -1;
	if (connect(clfd, (struct sockaddr *)&sun, sunl) == -1) {
		if (errno == EACCES || errno == EPERM) {
			rsp = UINIT_RSP_DENIED;
		}
		else if (errno == ENOENT || errno == ECONNREFUSED) {
			rsp = UINIT_NO_SOCKET;
		}
		else {
			rsp = -1;
		}
		goto _err;
	}

	rsp = UINIT_RSP_OK;
	sz = (size_t)write(clfd, &scmd, sizeof(int));
	if (sz == NOSIZE) rsp = -1;
	if (sz < sizeof(int)) rsp = UINIT_RSP_INVALID;
	if (rsp != UINIT_RSP_OK) goto _err;

	rsp = UINIT_RSP_INVALID;
	sz = (size_t)read(clfd, &rsp, sizeof(int));
	if (sz == NOSIZE) rsp = UINIT_RSP_INVALID;
	if (sz < sizeof(int)) rsp = UINIT_RSP_INVALID;
	if (!validate_response(rsp)) rsp = UINIT_RSP_INVALID;

_err:	close(clfd);
	return rsp;
}

enum { ACT_NONE = 0, ACT_REBOOT = 1, ACT_HALT = 2, ACT_POWEROFF = 4, ACT_NOSYNC = 128 };

static int try_init_comm(const char *prognam, int actf)
{
	int r, cmd;
	char *sockpath;

	if (!strcmp(prognam, "reboot")) cmd = UINIT_CMD_REBOOT;
	else if (!strcmp(prognam, "halt")) cmd = UINIT_CMD_SHUTDOWN;
	else if (!strcmp(prognam, "poweroff")) cmd = UINIT_CMD_POWEROFF;
	else if (!strcmp(prognam, "powerfail")) cmd = UINIT_CMD_POWERFAIL;
	else if (!strcmp(prognam, "singleuser")) cmd = UINIT_CMD_SINGLEUSER;
	else return 0;

	sockpath = getenv("UINIT_SOCKPATH");
	if (!sockpath) sockpath = _UINIT_SOCKPATH;
_again:	r = send_init_cmd(sockpath, cmd);
	if (r == -1) goto _again;
	else if (r == UINIT_RSP_OK) return 1;
	else if (r == UINIT_RSP_INVALID) xerror("bad communication with init");
	else if (r == UINIT_RSP_DENIED) xerror("permission denied by init");
	else if (r == UINIT_NO_SOCKET) {
		if (!checkids()) xerror("must be superuser");
		if (!(actf & ACT_NOSYNC)) sync();
		do_reboot(cmd);
		return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int c, actf;

	progname = basename(argv[0]);
	actf = ACT_NONE;

	opterr = 0;
	while ((c = getopt(argc, argv, "Sw")) != -1) {
		switch (c) {
			case 'S': actf |= ACT_NOSYNC; break;
			case 'w': return 0;
		}
	}

	if (try_init_comm(progname, actf)) return 0;

	opterr = 0;
	optind = 1;
	while ((c = getopt(argc, argv, "RHPSw")) != -1) {
		switch (c) {
			case 'R': actf |= ACT_REBOOT; break;
			case 'H': actf |= ACT_HALT; break;
			case 'P': actf |= ACT_POWEROFF; break;
			case 'S': actf |= ACT_NOSYNC; break;
			case 'w': return 0;
			default: return usage();
		}
	}

	if (!(actf & ACT_NOSYNC)) sync();

	if (actf & ACT_REBOOT) do_reboot(UINIT_CMD_REBOOT);
	else if (actf & ACT_HALT) do_reboot(UINIT_CMD_SHUTDOWN);
	else if (actf & ACT_POWEROFF) do_reboot(UINIT_CMD_POWEROFF);
	else return usage();

	return 0;
}

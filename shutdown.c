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

static void do_reboot(int cmd)
{
	int r = reboot(cmd);
	if (r == -1 && errno == EPERM) xerror("must be superuser");
}

static int checkids(void)
{
	return (geteuid() == 0 && getegid() == 0);
}

static int send_init_cmd(const char *path, int cmd)
{
	struct sockaddr_un sun;
	socklen_t sunl;
	int clfd, scmd = cmd;

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
		if (errno == EACCES || errno == EPERM) return -2;
		return 0;
	}
	write(clfd, &scmd, sizeof(int));
	close(clfd);
	return 1;
}

enum { ACT_NONE = 0, ACT_REBOOT = 1, ACT_HALT = 2, ACT_POWEROFF = 4, ACT_NOSYNC = 128 };

int main(int argc, char **argv)
{
	char *sockpath = _UINIT_SOCKPATH;
	int c, actf;
	int t;

	progname = basename(argv[0]);
	actf = ACT_NONE;

	opterr = 0;
	while ((c = getopt(argc, argv, "Sw")) != -1) {
		switch (c) {
			case 'S': actf |= ACT_NOSYNC; break;
			case 'w': return 0;
		}
	}

	sockpath = getenv("UINIT_SOCKPATH");
	if (!sockpath) sockpath = _UINIT_SOCKPATH;

	if (!strcmp(progname, "reboot")) {
_ckr:		t = send_init_cmd(sockpath, UINIT_CMD_REBOOT);
		if (t == -1) goto _ckr;
		else if (t == -2) xerror("must be superuser");
		else if (t == 0) {
			if (!checkids()) xerror("must be superuser");
			if (!(actf & ACT_NOSYNC)) sync();
			do_reboot(RB_AUTOBOOT);
		}
		return 0;
	}
	else if (!strcmp(progname, "halt")) {
_ckh:		t = send_init_cmd(sockpath, UINIT_CMD_SHUTDOWN);
		if (t == -1) goto _ckh;
		else if (t == -2) xerror("must be superuser");
		else if (t == 0) {
			if (!checkids()) xerror("must be superuser");
			if (!(actf & ACT_NOSYNC)) sync();
			do_reboot(RB_HALT_SYSTEM);
		}
		return 0;
	}
	else if (!strcmp(progname, "poweroff")) {
_ckp:		t = send_init_cmd(sockpath, UINIT_CMD_POWEROFF);
		if (t == -1) goto _ckp;
		else if (t == -2) xerror("must be superuser");
		else if (t == 0) {
			if (!checkids()) xerror("must be superuser");
			if (!(actf & ACT_NOSYNC)) sync();
			do_reboot(RB_POWER_OFF);
		}
		return 0;
	}
	else if (!strcmp(progname, "powerfail")) {
_ckf:		t = send_init_cmd(sockpath, UINIT_CMD_POWERFAIL);
		if (t == -1) goto _ckf;
		else if (t == -2) xerror("must be superuser");
		else if (t == 0) {
			if (!checkids()) xerror("must be superuser");
			if (!(actf & ACT_NOSYNC)) sync();
		}
		return 0;
	}
	else if (!strcmp(progname, "singleuser")) {
_cks:		t = send_init_cmd(sockpath, UINIT_CMD_SINGLEUSER);
		if (t == -1) goto _cks;
		else if (t == -2) xerror("must be superuser");
		else if (t == 0) {
			if (!checkids()) xerror("must be superuser");
			if (!(actf & ACT_NOSYNC)) sync();
		}
		return 0;
	}

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

	if (actf & ACT_REBOOT) do_reboot(RB_AUTOBOOT);
	else if (actf & ACT_HALT) do_reboot(RB_HALT_SYSTEM);
	else if (actf & ACT_POWEROFF) do_reboot(RB_POWER_OFF);
	else return usage();

	return 0;
}

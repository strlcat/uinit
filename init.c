/*
 * This code is in public domain
 */

#define _XOPEN_SOURCE 700
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reboot.h>

#ifndef _UINIT_PATH
#define _UINIT_PATH "/etc/init"
#endif

typedef void (*sighandler_t)(int);

static void signal_handler(int sig)
{
	if (fork()) return;

	switch (sig) {
		case SIGINT:
		execl(_UINIT_PATH "/cad", "cad", (char *)NULL);
		break;

		case SIGALRM:
		execl(_UINIT_PATH "/reboot", "reboot", (char *)NULL);
		break;

		case SIGQUIT:
		execl(_UINIT_PATH "/poweroff", "poweroff", (char *)NULL);
		break;

		case SIGABRT:
		execl(_UINIT_PATH "/shutdown", "shutdown", (char *)NULL);
		break;

#ifdef SIGPWR
		case SIGPWR:
		execl(_UINIT_PATH "/pwrfail", "pwrfail", (char *)NULL);
		break;
#endif
	}
}

int main(void)
{
	sigset_t set;
	int status;

	if (getpid() != 1) return 1;

	if (!access(_UINIT_PATH "/altinit", X_OK) && !getenv("_INIT"))
		execl(_UINIT_PATH "/altinit", "init", (char *)NULL);

	reboot(RB_DISABLE_CAD);

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	if (fork()) {
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		sigdelset(&set, SIGINT);
		sigdelset(&set, SIGALRM);
		sigdelset(&set, SIGQUIT);
		sigdelset(&set, SIGABRT);
#ifdef SIGPWR
		sigdelset(&set, SIGPWR);
#endif
		sigprocmask(SIG_BLOCK, &set, NULL);
		signal(SIGINT, signal_handler);
		signal(SIGALRM, signal_handler);
		signal(SIGQUIT, signal_handler);
		signal(SIGABRT, signal_handler);
#ifdef SIGPWR
		signal(SIGPWR, signal_handler);
#endif

		for (;;) wait(&status);
	}

	sigprocmask(SIG_UNBLOCK, &set, NULL);

	setsid();
	setpgid(0, 0);
	return execl(_UINIT_PATH "/boot", "boot", (char *)NULL);
}

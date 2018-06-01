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

typedef void (*sighandler_t)(int);

static void sighandl(int sig)
{
	if (fork()) return;

	switch (sig) {
		case SIGINT:
		execl("/etc/init/cad", "cad", (char *)NULL);
		break;

		case SIGALRM:
		execl("/etc/init/reboot", "reboot", (char *)NULL);
		break;

		case SIGQUIT:
		execl("/etc/init/poweroff", "poweroff", (char *)NULL);
		break;

		case SIGABRT:
		execl("/etc/init/shutdown", "shutdown", (char *)NULL);
		break;

#ifdef SIGPWR
		case SIGPWR:
		execl("/etc/init/pwrfail", "pwrfail", (char *)NULL);
		break;
#endif
	}
}

int main(void)
{
	sigset_t set;
	int status;

	if (getpid() != 1) return 1;

	if (!access("/etc/init/altinit", X_OK) && !getenv("_INIT"))
		execl("/etc/init/altinit", "init", (char *)NULL);

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
		signal(SIGINT, sighandl);
		signal(SIGALRM, sighandl);
		signal(SIGQUIT, sighandl);
		signal(SIGABRT, sighandl);
#ifdef SIGPWR
		signal(SIGPWR, sighandl);
#endif

		for (;;) wait(&status);
	}

	sigprocmask(SIG_UNBLOCK, &set, NULL);

	setsid();
	setpgid(0, 0);
	return execl("/etc/init/boot", "boot", (char *)NULL);
}

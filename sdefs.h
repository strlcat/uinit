#ifndef _UINIT_SDEFS_H
#define _UINIT_SDEFS_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NOSIZE ((size_t)-1)
#define NOUID  ((uid_t)-1)
#define NOGID  ((gid_t)-1)

enum {
	UINIT_CMD_INVALID, UINIT_CMD_SHUTDOWN, UINIT_CMD_POWEROFF,
	UINIT_CMD_REBOOT, UINIT_CMD_CAD, UINIT_CMD_POWERFAIL, UINIT_CMD_SINGLEUSER,
};

#endif

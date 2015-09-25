uinit - smallest init possible.
===============================

## HOW IT WORKS

uinit is a simple init skeleton which is called by kernel directly
(executing /sbin/init as first process). It then disables
Ctrl-Alt-Delete sequence and executes /etc/init/boot script as it's child which is
responsible for further system rollup. Beyond that, init only waits for
ended orphaned processes.

Writing /etc/init/boot is a duty of a system designer. It's not
included (nor any of additional scripts), because I think everyone can
adapt their own or write from scratch. Additionally, the following
scripts are needed (but they can be a hard/symlink to a more general script):

/etc/init/reboot - gets executed when system is going to be rebooted
/etc/init/poweroff - gets executed when system is shutting down
/etc/init/shutdown - gets executed when system will be halted
/etc/init/cad - gets executed when user typed Ctrl-Alt-Delete on system
console
/etc/init/pwrfail - gets executed when init receives SIGPWR. System
administrator can install a UPS monitoring service which then, on power
failure, will send SIGPWR to any process in system.

/etc/init/altinit - this is preinit script. It is usually not needed (and can be safely omitted),
but can be used as a fallback when you have no chance to specify init=
kernel parameter. init process replaces itself with this script if it
exists, and script gets pid 1. It can continue execution out by
executing /sbin/init again with _INIT environment variable set to any
value:

	#!/bin/sh
	# ... Do your preinit stuff there ...
	_INIT=1 exec /sbin/init

Internally uinit is controlled by sending signals to pid 1. See source for details. The user with sufficient privileges
can send signals to init and it will respect them too.

## Why?

Because I wanted to learn how Linux system boots. And following recent saga about init madness, I wanted something
very simple and controllable. It's mostly an init system for embedded devices, but can serve well even on desktops.

## How to use it

Installing the programs is recommended when you're building your own system. Replacing existing init with uinit can be
risky. You should have prepared your own boot scripts already and have them in place.

/sbin/init is called by kernel at system startup. Normally you should never call it when system is running.
But if you do, it will just exit if it's not a pid 1, it's safe to do so. It does not accept any command line arguments.

/sbin/shutdown is called when system needs to be turned off. When called as 'shutdown', the program will, according
to command line, bring the system down IMMEDIATELY. Without arguments it will just show a help.

Calling `/sbin/shutdown -h` will halt your system. It will not show the help screen :-)

'-h' will halt, '-p' will power down if possible, '-r' will reboot the system.
'-c' will tell Linux kernel to send SIGINT to init when user types C-a-d in system console. '-C' cancels that, and
returns Linux kernel to default: quick reboot on C-a-d.

When called as 'reboot', 'poweroff', or 'halt' (process name), the program will instead send needed signals to init.
The system will not be halted immediately, but init will run the scripts to do so.

Unprivileged user usually can't send signals to pid 1, or directly control system shutdown. The errors are ignored then,
/sbin/shutdown does not report them.

/sbin/spawn is a little supervising program. The program to be spawned is given in it's command line (including arguments).
Then spawn runs it as it's child, watches it's state and respawns it if it exited. Spawn must be killed to stop respawning
the program, or countdown must be specified. It supports specifying respawn counts, delay between respawns and controlling
tty the child will have, which must already exist. This tty is opened then, asigned as standard three Unix IO streams.

'-c' specifies path to tty, '-t' specifies time between respawns in milliseconds, '-T' specifies maximum count of respawns.

## Limitations

Of course if you want full-blown runtime features other than fundamental system control, then this is not your init.

Things that are not going to be supported:
- Runlevels
- Single user mode (as part of runlevels)
- Suck in any parts from userspace
- Sending signals, watching processes other than reaping orphans
- Configuration file(s)
- Listening to commands via pipe
- Exporting it's own control api via any mechanism
- Other features which you can see in various init systems I'm unaware of.

In fact, this is already finished init program which can be used everywhere. I don't plan any further changes.

## See also

Building Linux from ground was (and partially is) my hobby. From the experience in this area I can tell that small and efficient init is not sufficient, especially when you need to alter certain program properties such as privileges (with free setXid/groups control), arguments or environment.

Below is a list of programs which I use often when writing my own init scripts:

- fork - process daemonizer (force programs to background)
- setugid or super - universal privilege separator tool (force programs to be under certain
- uid/gid/groups, free group control, set environment, set current directory, chroot, set resource limits and priority)
- execvp - set program display name, alter argv, fork-call execvp() from shell
- swd - simple but useful when you want to `pwd` relative directory

These tools can be found either in my experimental [R2](https://github.com/siblynx/r2) Linux, or in [lynxware](https://github.com/siblynx/lynxware) repository. They also simple enough to be in one source file. The only super itself is a big experimental program which aims to be a more powerful sudo replacement. It is located [here](http://lynxlynx.tk/prg/super/), and has it's own [repository](https://github.com/siblynx/super).

Lynx, Sep2015.

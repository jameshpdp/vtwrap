#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <paths.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

#ifdef linux
#include <pty.h>
#else
#include <util.h>
#endif

#include "vtwrap.h"


#ifndef __dead
#define __dead	__attribute__((noreturn))
#endif

#ifndef INFTIM
#define INFTIM	-1
#endif


static struct termios tp;
static sig_atomic_t quit;
static pid_t pid;
static int fd;

static int strip;


static void
sighandler(int signo)
{
	int save_errno = errno;
	struct winsize winsz;
	pid_t pgrp;

	switch (signo) {
	case SIGCHLD:
		if (waitpid(pid, NULL, WNOHANG) == pid) {
			pid = 0;
			quit++;
		}
		break;
	case SIGWINCH:
		if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsz) >= 0) {
			if (ioctl(fd, TIOCSWINSZ, &winsz) >= 0 &&
			    ioctl(fd, TIOCGPGRP, &pgrp) >= 0)
				(void)killpg(pgrp, SIGWINCH);
		}
		break;
	}

	errno = save_errno;
}

static void
cleanup(void)
{
	int save_errno = errno;

	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp);
	if (pid > 0)
		(void)kill(SIGTERM, pid);

	errno = save_errno;
}

__dead static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-s] [-t terminal]\n", __progname);
	exit(EX_USAGE);
}

int
main(int argc, char *const argv[])
{
	enum ioerr (*vtwrap)(int, struct pollfd *) = vtwrap_none;
	struct termios raw_tp;
	struct pollfd pfds[2];
	struct winsize winsz;
	struct sigaction sa;
	enum ioerr ioerr = E_NONE;
	int ch;

	while ((ch = getopt(argc, argv, "st:")) != -1)
		switch (ch) {
		case 's':
			strip++;
			break;
		case 't':
			if (strcasecmp(optarg, "vt52") == 0) {
				vtwrap = vtwrap_vt52;
				break;
			}

			if (strcasecmp(optarg, "cm7209") == 0) {
				vtwrap = vtwrap_cm7209;
				break;
			}

			if (strcasecmp(optarg, "none") == 0) {
				vtwrap = vtwrap_none;
				break;
			}

			errx(EX_CONFIG, "%s terminal not supported", optarg);
			/* NOTREACHED */
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (tcgetattr(STDIN_FILENO, &tp) < 0)
		err(EX_UNAVAILABLE, "tcgetattr");
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsz) < 0)
		err(EX_UNAVAILABLE, "TIOCGWINSZ");

	raw_tp = tp;
	cfmakeraw(&raw_tp);
	raw_tp.c_lflag &= ~ECHO;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_tp) < 0)
		err(EX_UNAVAILABLE, "tcsetattr");

	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sighandler;
	(void)sigaction(SIGCHLD, &sa, NULL);
	(void)sigaction(SIGWINCH, &sa, NULL);

	if ((pid = forkpty(&fd, NULL, &tp, &winsz)) == 0) {
		char *shell;

		if (vtwrap == vtwrap_vt52 || vtwrap == vtwrap_cm7209)
			(void)setenv("TERM", "vt52", 1);

		if ((shell = getenv("SHELL")) == NULL)
			shell = _PATH_BSHELL;
		(void)execl(shell, shell, "-i", (char *)NULL);
		cleanup();
		err(EX_UNAVAILABLE, "%s", shell);
	}

	if (pid < 0) {
		cleanup();
		err(EX_OSERR, "forkpty");
	}

	pfds[0].fd = STDIN_FILENO;
	pfds[1].fd = fd;
	pfds[0].events = pfds[1].events = POLLIN;

	while (!quit) {
		if (poll(pfds, 2, INFTIM) < 0) {
			if (errno == EINTR)
				continue;
			cleanup();
			err(EX_UNAVAILABLE, "poll");
		}

		if ((ioerr = vtwrap(strip, pfds)) != E_NONE)
			break;
	}

	cleanup();

	switch (ioerr) {
	case E_RSTDIN:
		err(EX_IOERR, "read: stdin");
		/* NOTREACHED */
	case E_WSTDOUT:
		err(EX_IOERR, "write: stdout");
		/* NOTREACHED */
	case E_RPTY:
		err(EX_IOERR, "read: pty");
		/* NOTREACHED */
	case E_WPTY:
		err(EX_IOERR, "write: pty");
		/* NOTREACHED */
	case E_EOF:
	case E_NONE:
		break;
	}

	return (EX_OK);
}

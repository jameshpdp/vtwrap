#include <poll.h>
#include <unistd.h>

#include "vtwrap.h"


enum ioerr
vtwrap_none(int strip, struct pollfd *pfds)
{
	unsigned char buf[512];
	ssize_t i, n;

	if (pfds[0].revents & POLLIN) {
		if ((n = read(pfds[0].fd, buf, sizeof(buf))) < 0)
			return (E_RSTDIN);

		if (n == 0)
			return (E_EOF);

		if (strip)
			for (i = 0; i < n; i++)
				buf[i] &= '\177';

		if (write(pfds[1].fd, buf, (size_t)n) < 0)
			return (E_WPTY);
	}

	if (pfds[1].revents & POLLIN) {
		if ((n = read(pfds[1].fd, buf, sizeof(buf))) < 0)
			return (E_RPTY);

		if (n == 0)
			return (E_EOF);

		if (strip)
			for (i = 0; i < n; i++)
				buf[i] &= '\177';

		if (write(pfds[0].fd, buf, (size_t)n) < 0)
			return (E_WSTDOUT);
	}

	return (E_NONE);
}

#include <poll.h>
#include <unistd.h>

#include "vtwrap.h"

#include <stdio.h>


/* ARGSUSED */
enum ioerr
vtwrap_cm7209(int strip __attribute__((unused)), struct pollfd *pfds)
{
	unsigned char buf[512], obuf[512 * 3];
	static int esc, x, y;
	ssize_t i, n, o;

	if (pfds[0].revents & POLLIN) {
		if ((n = read(pfds[0].fd, buf, sizeof(buf))) < 0)
			return (E_RSTDIN);

		if (n == 0)
			return (E_EOF);

		for (i = 0; i < n; i++)
			buf[i] &= '\177';

		for (i = o = 0; i < n; i++) {
			if (buf[i] == '\033' && i + 2 < n) {
				if (buf[i + 1] == '[') {
					switch (buf[i + 2]) {
					case 'A':	/* Up */
					case 'B':	/* Down */
					case 'C':	/* Right */
					case 'D':	/* Left */
					case 'H':	/* Home */
						obuf[o++] = buf[i++];
						obuf[o++] = buf[++i];
						continue;
					default:
						i += 2;
						while (i < n &&
						    buf[i] >= '0' &&
						    buf[i] <= '9')
							i++;
						continue;
					}
				} else if (buf[i + 1] == 'O') {
					switch (buf[i + 2]) {
					case 'P':	/* Gold */
					case 'Q':	/* Help */
					case 'R':	/* DelLine */
					case 'M':	/* Ret */
					case 'n':	/* . */
					case 'p':	/* 0 */
					case 'q':	/* 1 */
					case 'r':	/* 2 */
					case 's':	/* 3 */
					case 't':	/* 4 */
					case 'u':	/* 5 */
					case 'v':	/* 6 */
					case 'w':	/* 7 */
					case 'x':	/* 8 */
					case 'y':	/* 9 */
						obuf[o++] = buf[i++];
						obuf[o++] = buf[++i];
						continue;
					default:
						i += 2;
						continue;
					}
				}
			}

			obuf[o++] = buf[i];
		}

		if (write(pfds[1].fd, obuf, (size_t)o) < 0)
			return (E_WPTY);
	}

	if (pfds[1].revents & POLLIN) {
		if ((n = read(pfds[1].fd, buf, sizeof(buf))) < 0)
			return (E_RPTY);

		if (n == 0)
			return (E_EOF);

		for (i = 0; i < n; i++)
			buf[i] &= '\177';

		for (i = o = 0; i < n; i++) {
			switch (esc) {
			case 0:
				if (buf[i] == '\033') {
					esc++;
					continue;
				}

				if (buf[i] >= '`' && buf[i] <= '~')
					buf[i] |= '\200';

				obuf[o++] = buf[i];
				break;
			case 1:
				switch (buf[i]) {
				case 'I':
					buf[i] = 'M';
					/* FALLTHROUGH */
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'H':
				case 'J':
				case 'K':
					obuf[o++] = '\033';
					obuf[o++] = '[';
					obuf[o++] = buf[i];
					--esc;
					break;
				case 'Y':
					esc++;
					break;
				case 'Z':
					if (write(pfds[1].fd, "\033/Z", 3) < 0)
						return (E_WPTY);
					/* FALLTHROUGH */
				default:
					--esc;
					break;
				}

				continue;
			case 2:
				if ((y = (int)(buf[i] - '\037')) < 0)
					y = 0;
				esc++;
				break;
			case 3:
				if ((x = (int)(buf[i] - '\037')) < 0)
					x = 0;
				obuf[o++] = '\033';
				obuf[o++] = '[';
				obuf[o++] = (y / 10) + '0';
				obuf[o++] = (y % 10) + '0';
				obuf[o++] = ';';
				obuf[o++] = (x / 10) + '0';
				obuf[o++] = (x % 10) + '0';
				obuf[o++] = 'H';
				esc = 0;
				break;
			}
		}

		if (write(pfds[0].fd, obuf, (size_t)o) < 0)
			return (E_WSTDOUT);
	}

	return (E_NONE);
}

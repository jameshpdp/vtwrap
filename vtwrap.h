#ifndef __VTWRAP_H__
#define __VTWRAP_H__

enum ioerr {
	E_NONE,
	E_EOF,
	E_RSTDIN,
	E_RPTY,
	E_WSTDOUT,
	E_WPTY
};

__BEGIN_DECLS
enum ioerr vtwrap_cm7209(int, struct pollfd *);
enum ioerr vtwrap_none(int, struct pollfd *);
enum ioerr vtwrap_vt52(int, struct pollfd *);
__END_DECLS

#endif	/* __VTWRAP_H__ */

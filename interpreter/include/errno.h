#ifndef __UPS_ERRNO_H
#define __UPS_ERRNO_H

#undef errno
extern int *_errno_(void);
#define errno (*_errno_())

/* PORTABILITY */
#define EDOM 33
#define ERANGE 34

#endif


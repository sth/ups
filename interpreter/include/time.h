#ifndef __UPS_TIME_H
#define __UPS_TIME_H

#include <stddef.h>		/* for NULL, size_t */

#ifndef __UPS_TIME_T_DEFINED
	typedef long int time_t;
#	define __UPS_TIME_T_DEFINED
#endif

#ifndef __UPS_CLOCK_T_DEFINED
	typedef long int clock_t;
#	define __UPS_CLOCK_T_DEFINED
#endif

#ifndef __UPS_TM_DEFINED
	struct tm
	{
	  int tm_sec;			 
	  int tm_min;			 
	  int tm_hour;			 
	  int tm_mday;			 
	  int tm_mon;			 
	  int tm_year;			 
	  int tm_wday;			 
	  int tm_yday;			 
	  int tm_isdst;			 
	  long int tm_gmtoff;		 
	  const char *tm_zone;	 
	};
#	define __UPS_TM_DEFINED
#endif

extern clock_t    clock  (void);
extern double     difftime  (time_t, time_t);
extern time_t     mktime  (struct tm *);
extern time_t     time  (time_t *);
extern char *     asctime  (const struct tm *);
extern char *     ctime  (const time_t *);
extern struct tm *gmtime  (const time_t *);
extern struct tm *localtime  (const time_t *);
extern size_t     strftime  (char *, size_t, const char *, const struct tm *);
/* wcsftime() not supported */

#endif

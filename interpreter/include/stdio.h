#ifndef __UPS_STDIO_H
#define __UPS_STDIO_H

#include <stddef.h>			/* for NULL and size_t */
#include <stdarg.h>			/* for va_list */

#undef  _IOFBF
#undef  _IOLBF
#undef  _IONBF

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#undef  BUFSIZ
#define BUFSIZ 8192

#undef  EOF
#define EOF (-1)

#ifndef __UPS_FILE_DEFINED
	typedef struct __ups_file_t FILE;
#	define __UPS_FILE_DEFINED
#endif

#undef  FILENAME_MAX
#define FILENAME_MAX 4095

#undef  FOPEN_MAX
#define FOPEN_MAX 256

#ifndef __UPS_FPOS_T_DEFINED
	typedef long int fpos_t;		 
#	define __UPS_FPOS_T_DEFINED
#endif

#undef  L_tmpnam
#define L_tmpnam 19

#undef  SEEK_SET
#undef  SEEK_CUR
#undef  SEEK_END

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#undef  stdin
#undef  stdout
#undef  stderr

extern FILE *_stdin_(void), *_stdout_(void), *_stderr_(void);  
#define stdin  _stdin_()
#define stdout _stdout_()
#define stderr _stderr_()

#undef  TMP_MAX
#define TMP_MAX 238328

extern int      remove  (const char*)  ;
extern int      rename  (const char*, const char*)  ;
#ifdef NOTYET
extern FILE*    tmpfile  (void)  ;
#endif
extern char*    tmpnam  (char*)  ;
extern int      fclose  (FILE*)  ;
extern int      fflush  (FILE*)  ;
extern FILE*    fopen  (const char*, const char*)  ;
extern FILE*    freopen  (const char*, const char*, FILE*)  ;
extern void     setbuf  (FILE*, char*)  ;
extern int      setvbuf  (FILE*, char*, int, size_t)  ;
extern int      fprintf  (FILE*, const char*, ...)  ;
extern int      fscanf  (FILE *, const char*, ...)  ;
extern int      printf  (const char*, ...)  ;
extern int      scanf  (const char*, ...)  ;
extern int      sprintf  (char*, const char*, ...)  ;
extern int      sscanf  (const char*, const char*, ...)  ;
extern int      vfprintf  (FILE *, char const *, va_list)  ;
extern int      vprintf  (char const *, va_list)  ;
extern int      vsprintf  (char*, const char*, va_list)  ;
extern int      fgetc  (FILE *)  ;
extern char*    fgets  (char*, int, FILE*)  ;
extern int      fputc  (int, FILE*)  ;
extern int      fputs  (const char *, FILE *)  ;
extern int      getc  (FILE *)  ;
extern int      getchar  (void)  ;
#ifdef NOTYET
extern char*    gets  (char*)  ;
#endif
extern int      putc  (int, FILE *)  ;
extern int      putchar  (int)  ;
extern int      puts  (const char *)  ;
extern int      ungetc  (int, FILE*)  ;
extern size_t   fread  (void*, size_t, size_t, FILE*)  ;
extern size_t   fwrite  (const void*, size_t, size_t, FILE*)  ;
extern int      fgetpos  (FILE*, fpos_t *)  ;
extern int      fseek  (FILE*, long int, int)  ;
extern int      fsetpos  (FILE*, const fpos_t *)  ;
extern long int ftell  (FILE*)  ;
extern void     rewind  (FILE*)  ;
extern void     clearerr  (FILE*)  ;
extern int      feof  (FILE*)  ;
extern int      ferror  (FILE*)  ;
extern void     perror  (const char *)  ;

#undef  getchar
#define getchar() fgetc(stdin)

#undef  putchar
#define putchar(ch) fputc((ch),stdout)

#undef  getc
#define getc(fp) fgetc((fp))

#undef  putc
#define putc(ch,fp) fputc((ch),(fp))

#endif

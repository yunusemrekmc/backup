#ifndef STATUS_H
#define STATUS_H

#include <errno.h>

#define C_RESET  "\x1b[0m"
#define C_RED    "\x1b[31m"
#define C_YELLOW "\x1b[33m"
#define C_BLUE   "\x1b[34m"
#define C_GRAY   "\x1b[90m"
#define C_BOLD   "\x1b[1m"

#ifdef NO_COLOR
#define C_RESET  ""
#define C_RED    ""
#define C_YELLOW ""
#define C_BLUE   ""
#define C_GRAY   ""
#define C_BOLD   ""
#endif

#define STATUS(code, sysc, msg, file) \
((status_t){ (code), (sysc), (msg), (file), __FILE__, __LINE__ })

#define STATUS_E(code, msg, file) \
STATUS((code), errno, (msg), (file))

#define ST_ISERROR(code) ((ST_WARN_END < (code)) && ((code) < ST_ERR_END))
#define ST_ISWARN(code) (((code) > ST_INT_END) && ((code) < ST_WARN_END))
#define ST_ISOK(code) (((code) == ST_OK))
#define ST_ISINT(code)  (((code) > ST_OK) && ((code) < ST_INT_END))

typedef enum {
	ST_OK = 0,

	/* Internal */
	ST_INT_ISNULL,		/* Got NULL instead of a pointer */
	ST_INT_END,             /* END of internal declaration: easier to use in macros */

	/* Warnings */
	ST_WARN_NO_CRYPT,	/* No encryption */
	ST_WARN_FILERD_MD, 	/* Couldn't read file metadata */
	ST_WARN_END,		/* END of warning declaration: easier to use in macros */

	/* Errors */
	ST_ERR_PUSH_DIR,
	ST_ERR_MALLOC,
	ST_ERR_HASH_CRE,	/* Couldn't create a hash table */
	ST_ERR_HASH_UPS,	/* Can't upsize hash table */
	ST_ERR_FILERD,		/* Couldn't read file */
	ST_ERR_FILERD_MD, 	/* Couldn't read file metadata */
	ST_ERR_OPEN,
	ST_ERR_END		/* END of error declaration: easier to use in macros */
} stcode_t;

typedef struct {
	stcode_t c;
	int sysc; 		/* errno code */
	const char* text;
	char* file_target; /* the file that caused the problem */
	const char* file;	 /* C source file */
	int line;
} status_t;

const char* stmsg(stcode_t c);
void sterr(status_t st);
void status_free(status_t st);

#endif

#ifndef STATUS_H
#define STATUS_H

typedef enum {
	ST_OK = 0,

	/* Internal */
	ST_INT_ISNULL,		/* Got NULL instead of a pointer */

	/* Warnings */
	ST_WARN_NO_CRYPT,	/* No encryption */

	/* Errors */
	ST_ERR_PUSH_DIR,
	ST_ERR_NOMEM,
	ST_ERR_FILERD,
	ST_ERR_OPEN
} stcode_t;

typedef struct {
	stcode_t c;
	int sysc; 		/* errno code */
	const char* text;
} status_t;

const char* stmsg(stcode_t c);
void sterr(status_t st);

#endif

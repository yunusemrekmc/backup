#include <stdio.h>
#include <string.h>

#include "status.h"

const char* stmsg(stcode_t c)
{
	switch(c) {
	case ST_OK: return "OK";
	case ST_WARN_NO_CRYPT: return "No encryption at target";
	case ST_ERR_OPEN: return "Failed to open file or directory";
	case ST_ERR_NOMEM: return "Failed to allocate memory";
	case ST_ERR_FILERD: return "Failed to read file or directory";
	default: return "Unknown status";
	}
}

void sterr(status_t st)
{
	fprintf(stderr, "%s: %s: %s\n",
		st.text, stmsg(st.c), strerror(st.sysc));
	return;
}

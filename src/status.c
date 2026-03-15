#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "status.h"

const char* stmsg(stcode_t c)
{
	switch(c) {
	case ST_OK: return "OK";
	case ST_INT_ISNULL: return "Got NULL as argument";
	case ST_WARN_NO_CRYPT: return "No encryption at target";
	case ST_ERR_OPEN: return "Failed to open file or directory";
	case ST_ERR_NOMEM: return "Failed to allocate memory";
	case ST_ERR_HASH_CRE: return "Couldn't create a hash table";
	case ST_ERR_HASH_UPS: return "Couldn't resize the hash table";
	case ST_ERR_FILERD: return "Failed to read file or directory";
	case ST_ERR_FILERD_MD: return "Couldn't read file metadata";
	default: return "Unknown status";
	}
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
void sterr(status_t st)
{
	if (st.file_target) {
		fprintf(stderr, "[%s:%d] %s (code=%d, errno=%d, file=%s: %s)\n",
			st.file, st.line,
			st.text, st.c, st.sysc, st.file_target, strerror(st.sysc));
		free((void*)st.file_target);
		return;
	}
		fprintf(stderr, "[%s:%d] %s (code=%d, errno=%d: %s)\n",
			st.file, st.line,
			st.text, st.c, st.sysc, strerror(st.sysc));
		
	return;
}

#pragma GCC diagnostic pop

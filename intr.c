#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "intr.h"

inline static void term_wanted(int sig);

void install_handlers(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = term_wanted;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

static void term_wanted(int sig)
{
	(void)sig;
	terminate_wanted = 1;
}

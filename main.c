/**
	\file main.c
	main()
*/

/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/resource.h>
#include <syslog.h>
/** \endcond */

#include "conf.h"

static int terminate=0;

static char *conf_path;

/** Daemon exit. Handles most of signals. */
static void daemon_exit(int s)
{
	if (s>0) {
		syslog(LOG_INFO, "Signal %d caught, exit now.", s); 
	} else {
		syslog(LOG_INFO, "Synchronized exit."); 
	}
	//TODO: do exit
	conf_delete();
	closelog();
	exit(0);
}

/** Initiate signal actions */
static void signal_init(void)
{
	struct sigaction sa; 

	sa.sa_handler = daemon_exit;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGQUIT);
	sigaddset(&sa.sa_mask, SIGINT);
	sa.sa_flags = 0;

	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGPIPE, &sa, NULL);
}

static void usage(const char *a0)
{   
	fprintf(stderr, "Usage: \n\t"				\
			"%1$s -h\t\t"                       	\
			"Print usage and exit.\n\t"           \
			"%1$s -v\t\t"                      \
			"Print version info and exit.\n\t"    \
			"%1$s -c CONFIGFILE\t"                \
			"Start program with CONFIGFILE.\n\t"  \
			"%1$s\t\t"                       \
			"Start program with default configure file(%2$s)\n", a0, DEFAULT_CONFPATH);

}

static void log_init(void)
{
	openlog("id_service", LOG_PERROR|LOG_PID|LOG_NDELAY, LOG_DAEMON);
}

static void version(void)
{   
	fprintf(stdout, "%s\n", APPVERSION);
}

static int dungeon_get_options(int argc, char **argv)
{
	int c;

	while (-1 != (c = getopt(argc, argv,
					"c:" /* configure file */
					"v" /* version */
					"h" /* help */
					))) {
		switch (c) {
			case 'c': /* configure file path */
				conf_path = optarg;
				break;
			case 'v': /* show version */
				version();
				break;
			case 'h': /* usage */
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			default:
				fprintf(stderr, "Invalid arguments");
				return -1;
		}
	}

	return 0;
}

static rlim_t rlimit_try(rlim_t left, rlim_t right)
{
	struct rlimit r;

	if (left>right) {
		syslog(LOG_ERR, "call: rlimit_try(%d, %d) is illegal!", left, right);
		abort();
	}
	if (left==right || left==right-1) {
		return left;
	}
	r.rlim_cur = right;
	r.rlim_max = r.rlim_cur;
	if (setrlimit(RLIMIT_NOFILE, &r)==0) {
		return right;
	}
	r.rlim_cur = (right-left)/2+left;
	r.rlim_max = r.rlim_cur;
	if (setrlimit(RLIMIT_NOFILE, &r)==0) {
		return rlimit_try(r.rlim_cur, right);
	} else {
		return rlimit_try(left, r.rlim_cur);
	}
}

static void rlimit_init()
{
	syslog(LOG_INFO, "ulimit -n => %d", rlimit_try(1024, conf_get_concurrent_max() * 5 + 1024));
}

int main(int argc, char **argv)
{
	/*
	 * Parse arguments
	 */
	if (dungeon_get_options(argc, argv) == -1) {
		return -1;
	}

	/*
	 * Parse config file
	 */
	if (conf_path == NULL) {
		conf_path = DEFAULT_CONFPATH;
	}

	if (conf_new(conf_path) == -1) {
		fprintf(stderr, "cannot find configure file: [%s], exit.\n", conf_path);
		return -1;
	}

	/*
	 * Init
	 */
	log_init();

	/* set open files number */
	rlimit_init();

	if (conf_get_daemon()) {
		daemon(1, 0);
	}

	chdir(conf_get_working_dir());

	signal_init();

	if (id_service_start()!=0) {
		syslog(LOG_ERR, "Can't create dungeon heart!");
		abort();
	}

	while (!terminate) {
		// Process signals.
		pause();
	}

	daemon_exit(0);
	return 0;
}


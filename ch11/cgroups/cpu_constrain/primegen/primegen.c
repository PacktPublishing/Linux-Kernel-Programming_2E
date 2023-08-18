/*
 * primegen.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Kernel Programming" 2E
 *  Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Linux-Kernel-Programming_2E
 ****************************************************************
 * Brief Description:
 *
 * This program is part of the Cgroups v2 demo: trying out setting constraints
 * on CPU usage on this app, a very simple prime number generator. It generates
 * primes from 2 to the number requested. It operates on a given timeout;
 * after the specified # of seconds have elapsed it's simply killed.
 *
 * The CPU constraint is imposed in 2 ways:
 * 1) by leveraging the power of systemd service units
 * 2) by manually creating and managing a cgroup from where this app's executed
 *
 * NOTE-
 * This prime number generation code is copied from open source.
 * We claim no credit, nor take on any liability whatsoever.
 *
 * For details, please refer the book, Ch 11.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <signal.h>

#define MAX	    10000000		// 10 million
#define MAX_TIME          60		// seconds

static void simple_primegen(int limit)
{
	int i, j, num = 2, isprime;

	printf("  2,  3, ");
	for (i = 4; i <= limit; i++) {
		isprime = 1;
		for (j = 2; j < limit / 2; j++) {
			if ((i != j) && (i % j == 0)) {
				isprime = 0;
				break;
			}
		}
		if (isprime) {
			num++;
			printf("%6d, ", i);
			/* Wrap after WRAP primes are printed on a line;
			 * this is crude; in production code, one must query
			 * the terminal window's width and calculate the column
			 * to wrap at.
			 */
#define WRAP    16
			if (num % WRAP == 0) {
				fflush(stdout);
				printf("\n");
			}
		}
	}
	printf("\n");
	fflush(stdout);
}

static void buzz(int signo)
{
	printf("%s:%s()\n", __FILE__, __func__);
	fflush(stdout);
	raise(SIGTERM);
}

int main(int argc, char **argv)
{
	int limit, time_allowed = 1;

	if (argc < 3) {
		fprintf(stderr,
			"Usage: %s limit-to-generate-primes-upto time-allowed-to-run(in seconds)\n"
			" max allowed value for:\n"
			"  first parameter is %d #s\n"
			"  second parameter is %ds\n"
			, argv[0], MAX, MAX_TIME);
		exit(EXIT_FAILURE);
	}

	limit = atoi(argv[1]);
	if (limit < 4 || limit > MAX) {
		fprintf(stderr,
			"%s: param 1, invalid value (%d); pl pass a value within "
			"the range [4 - %d].\n", argv[0], limit, MAX);
		exit(EXIT_FAILURE);
	}
	time_allowed = atoi(argv[2]);
	if (time_allowed < 1 || time_allowed > MAX_TIME) {
		fprintf(stderr,
			"%s: param 2, invalid value (%d); pl pass a value within "
			"the range [1 - %d].\n", argv[0], time_allowed, MAX_TIME);
		exit(EXIT_FAILURE);
	}

	if (signal(SIGALRM, buzz) < 0) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	alarm(time_allowed);
	simple_primegen(limit);
	exit(EXIT_SUCCESS);
}

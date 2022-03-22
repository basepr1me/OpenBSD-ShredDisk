/*
 * Copyright (c) 2018, 2022 Tracey Emery <tracey@traceyemery.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#define BYTES 32768

int main(void);
void sh_sig(int);

char string0[] = "#########################################";
char string1[] = "#  Welcome to shredderc. BE CAREFUL!!!  #";
char string2[] = "Which device would you like to shred? [ex: sd0]: ";
char string3[] = "How many passes would you like to make? ";
char string4[] = "ARE YOU SURE YOU WISH TO CONTINUE?\nOnce you start, the"
		     " damage is done! [y|n]: ";
char string5[] = "What do you want to write? [1:null, 2:0, 3:rand]: ";

void
sh_sig(int sig)
{
	/* re-enable the cursor and exit */
	printf("\e[?25h");
	printf("\n");
	exit(1);
}

int
main(void)
{
	FILE *dd;
	char *realdev = NULL, *ppasses = NULL, *cont = NULL;
	char *doshred = NULL, *todo = NULL;
	char dev[512];
	int passes, i_pass, dev_fd, si, tdo;
	double percent, total_bytes, time_remaining = 0;
	uint8_t rnum[BYTES];
	u_int64_t sectors, total;
	uint64_t start, seconds, now, elapsed = 0;
	struct  disklabel lab;
	size_t written;
	time_t sh_time, now_time;

	signal(SIGINT, sh_sig);
	signal(SIGTERM, sh_sig);
	signal(SIGHUP, sh_sig);
	signal(SIGPIPE, sh_sig);
	signal(SIGUSR1, sh_sig);

	if ((realdev = malloc(sizeof(char))) == NULL)
		err(1, "malloc");
	if ((ppasses = malloc(sizeof(char))) == NULL)
		err(1, "malloc");
	if ((cont = malloc(sizeof(char))) == NULL)
		err(1, "malloc");
	if ((doshred = malloc(sizeof(char))) == NULL)
		err(1, "malloc");
	if ((todo = malloc(sizeof(char))) == NULL)
		err(1, "malloc");

	printf("\n\n%s\n%s\n%s\n\n", string0, string1, string0);

	do {
		printf("%s", string2);
		fgets(dev, sizeof(dev), stdin);
		dev[strlen(dev) - 1] = '\0';
	} while (strlen(dev) < 1);

	printf("\n");

	do {
		printf("%s", string3);
		fgets(ppasses, sizeof(ppasses), stdin);
		ppasses[strlen(ppasses) - 1] = '\0';
		passes = atoi(ppasses);
	} while (passes < 1);

	printf("\n");

	do {
		printf("%s", string5);
		fgets(todo, sizeof(todo), stdin);
		todo[strlen(todo) - 1] = '\0';
		tdo = atoi(todo);
	} while (tdo < 1 || tdo > 3);

	printf("\n");

	do {
		printf("Do you really want to shred %s in %d passes? [y|n]: ",
		    dev, passes);
		fgets(cont, sizeof(cont), stdin);
		cont[strlen(cont) - 1] = '\0';
		for (si = 0; si < strlen(cont); si++) {
			cont[si] = tolower(cont[si]);
		}
		if (strcmp(cont, "n") == 0 || strcmp(cont, "no") == 0) {
			printf("Exiting!\n");
			exit(0);
		} else if (strcmp(cont, "y") == 0 || strcmp(cont, "yes") == 0)
			break;
	} while (1);

	printf("\n");

	do {
		printf("%s", string4);
		fgets(doshred, sizeof(doshred), stdin);
		doshred[strlen(doshred) - 1] = '\0';
		for (si = 0; si< strlen(doshred); si++) {
			doshred[si] = tolower(doshred[si]);
		}
		if (strcmp(doshred, "n") == 0 || strcmp(doshred, "no") == 0) {
			printf("Exiting!\n");
			exit(0);
		} else if (strcmp(doshred, "y") == 0 || strcmp(doshred, "yes")
		    == 0)
			break;
	} while (1);

	dev_fd = opendev(dev, O_RDWR, OPENDEV_PART, &realdev);

	if (dev_fd < 0)
		err(1, "open: %s", dev);
	else {
		if (ioctl(dev_fd, DIOCGPDINFO, &lab) < 0)
			err(4, "ioctl DIOCGPDINFO");

		sectors = DL_GETDSIZE(&lab);
		total_bytes = sectors * lab.d_secsize;
		close(dev_fd);

		dd = fopen(realdev, "w");
	};

	if (pledge("stdio unveil", NULL) == -1)
		err(1, "pledge");
	if (unveil(realdev, "rw") != 0)
		err(1, "unveil");

	/* disable the cursor */
	printf("\e[?25l");

	printf("\nShredding %s: %0.0f bytes", dev, total_bytes);

	start = time(NULL);

	for (i_pass = 0; i_pass < passes; i_pass++) {
		total = 0;
		fseek(dd, 0, SEEK_END);
		sh_time = time(NULL);

		printf("\n\nPass %d of %d started %s\n", i_pass, passes,
		    ctime(&sh_time));

		do {
			seconds = time(NULL) - start;
			switch (tdo) {
			case 1:
				memset(&rnum, '\0', BYTES);
				break;
			case 2:
				memset(&rnum, '0', BYTES);
				break;
			case 3:
				arc4random_buf(&rnum, BYTES);
				break;
			}
			written = fwrite(&rnum, BYTES, 1, dd);
			total += (written * BYTES);
			now = seconds;
			if (now - elapsed >= 1) {
				now_time = time(NULL);
				uint64_t passed_time = now_time - sh_time;
				char *suffix;
				if (passed_time > 0)
					time_remaining = (total_bytes - total) /
					    (total / passed_time);
				if (time_remaining > (60 * 60)) {
					time_remaining /= (60 * 60);
					asprintf(&suffix, "%s", "hours       ");
				} else if (time_remaining < (60 * 60) &&
				    time_remaining > 60) {
					time_remaining /= 60;
					asprintf(&suffix, "%s", "minutes     ");
				} else
					asprintf(&suffix, "%s", "seconds     ");

				percent = total / total_bytes *
				    100;
				printf("\rPercent completed: %0.2f%% / " \
				    "Approximate time remaining: %0.2f %s     ",
				    percent, time_remaining * (passes - i_pass),
				    suffix);
				fflush(stdout);
				free(suffix);
				elapsed = now;
			}
		} while ((written * BYTES) == sizeof(rnum));
	}

	sh_time = time(NULL);
	printf("\n\nFinished shredding %s\n", ctime(&sh_time));

	fclose(dd);

	/* re-enable cursor on finish */
	printf("\e[?25h");

	return 0;
}

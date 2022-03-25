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
#define BUFF 15

int main(void);
void sh_sig(int);

char string0[] = "#########################################";
char string1[] = "#  Welcome to shreddisk. BE CAREFUL!!!  #";
char string2[] = "Which device would you like to shred? [ex: sd0]: ";
char string3[] = "How many passes would you like to make? ";
char string4[] = "ARE YOU SURE YOU WISH TO CONTINUE?\nOnce you start, the"
		     " damage is done! [y|n]: ";
char string5[] = "What do you want to write? [1:null, 2:0, 3:rand]: ";

unsigned int shredding = 0;

void
sh_sig(int sig)
{
	/* re-enable the cursor and exit */
	if (shredding && sig == SIGINT)
		do {
			char quit[BUFF];
			unsigned int si;
			printf("\r                                             "
			    "                                                  "
			    "\rDo you really want to quit? [y|n]: ");
			fflush(stdout);
			fgets(quit, sizeof(quit), stdin);
			quit[strlen(quit) - 1] = '\0';
			for (si = 0; si < strlen(quit); si++)
			quit[si] = tolower(quit[si]);
			if (strcmp(quit, "n") == 0 || strcmp(quit, "no") == 0) {
				printf("\n");
				return;
			} else if (strcmp(quit, "y") == 0 ||
			    strcmp(quit, "yes") == 0) {
				printf("\n");
				break;
			}
		} while (1);
	printf("\e[?25h");
	printf("\n\n");
	exit(1);
}

int
main(void)
{
	FILE *dd;
	char *realdev = NULL;
	char ppasses[BUFF], cont[BUFF], doshred[BUFF], todo[BUFF], dev[BUFF];
	unsigned int passes, i_pass, dev_fd, si, tdo, twrite;
	double percent, total_bytes, time_remaining = 0, byte_diff;
	u_int64_t sectors, total;
	uint64_t start, seconds, now, elapsed = 0, passed_time;
	struct  disklabel lab;
	size_t written;
	time_t sh_time, now_time;
	const char *errstr;

	signal(SIGINT, sh_sig);
	signal(SIGTERM, sh_sig);
	signal(SIGHUP, sh_sig);
	signal(SIGPIPE, sh_sig);
	signal(SIGUSR1, sh_sig);

	if ((realdev = malloc(sizeof(char))) == NULL)
		err(1, "malloc");

	printf("\n\n%s\n%s\n%s\n\n", string0, string1, string0);

	do {
baddev:
		printf("%s", string2);
		fgets(dev, sizeof(dev), stdin);
	} while (strlen(dev) < 4 || strlen(dev) > BUFF);
	dev[strlen(dev) - 1] = '\0';

	dev_fd = opendev(dev, O_RDWR, OPENDEV_PART, &realdev);

	if (dev_fd < 0) {
		printf("\nCould not open device %s\n\n", dev);
		goto baddev;
	} else {
		if (ioctl(dev_fd, DIOCGPDINFO, &lab) < 0) {
			printf("\nDevice %s does not exist\n\n", dev);
			memset(&dev, 0, sizeof(dev));
			goto baddev;
		}
		sectors = DL_GETDSIZE(&lab);
		total_bytes = sectors * lab.d_secsize;
		if (close(dev_fd) == -1) {
			printf("\nCould not close fd %d\n\n", dev_fd);
			memset(&dev, 0, sizeof(dev));
			goto done;
		}
		if ((dd = fopen(realdev, "w")) == NULL) {
			printf("\nCould not open device %s\n\n", realdev);
			memset(&dev, 0, sizeof(dev));
			goto baddev;
		}
	}

	printf("\n");

	do {
		printf("%s", string3);
		fgets(ppasses, sizeof(ppasses), stdin);
		ppasses[strlen(ppasses) - 1] = '\0';
		passes = strtonum(ppasses, 1, INT_MAX, &errstr);
		if (errstr != NULL)
			printf("\nInvalid input: %s\n\n", ppasses);
	} while (errstr != NULL);
	errstr = NULL;

	printf("\n");

	do {
		printf("%s", string5);
		fgets(todo, sizeof(todo), stdin);
		todo[strlen(todo) - 1] = '\0';
		tdo = strtonum(todo, 1, 3, &errstr);
		if (errstr != NULL)
			printf("\nInvalid input: %s\n\n", todo);
	} while (errstr != NULL);
	errstr = NULL;

	printf("\n");

	do {
		printf("Do you really want to shred %s in %d passes? [y|n]: ",
		    dev, passes);
		fgets(cont, sizeof(cont), stdin);
		cont[strlen(cont) - 1] = '\0';
		for (si = 0; si < strlen(cont); si++)
			cont[si] = tolower(cont[si]);
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
		for (si = 0; si< strlen(doshred); si++)
			doshred[si] = tolower(doshred[si]);
		if (strcmp(doshred, "n") == 0 || strcmp(doshred, "no") == 0) {
			printf("Exiting!\n");
			exit(0);
		} else if (strcmp(doshred, "y") == 0 ||
		    strcmp(doshred, "yes") == 0)
			break;
	} while (1);

	if (pledge("stdio unveil", NULL) == -1)
		err(1, "pledge");
	if (unveil(realdev, "w") != 0)
		err(1, "unveil");

	/* disable the cursor */
	printf("\e[?25l");

	printf("\nShredding %s: %0.0f bytes", dev, total_bytes);

	shredding = 1;
	start = time(NULL);

	for (i_pass = 0; i_pass < passes; i_pass++) {
		total = 0;
		if (fseek(dd, 0, SEEK_END) != 0) {
			printf("\nSeek problem on device %s\n\n", realdev);
			goto done;
		}
		sh_time = time(NULL);

		printf("\n\nPass %d of %d started %s\n", i_pass + 1, passes,
		    ctime(&sh_time));

		do {
			byte_diff = total_bytes - total;
			twrite = BUFF;
			if (byte_diff < BYTES)
				twrite = byte_diff;
			uint8_t rnum[twrite];
			seconds = time(NULL) - start;
			switch (tdo) {
			case 1:
				memset(&rnum, '\0', twrite);
				break;
			case 2:
				memset(&rnum, '0', twrite);
				break;
			case 3:
				arc4random_buf(&rnum, twrite);
				break;
			}

			written = fwrite(&rnum, twrite, 1, dd);
			if (sizeof(rnum) != written * twrite)
				break;

			total += (written * twrite);

			now = seconds;
			if (now - elapsed >= 1 || total_bytes == total) {
				char *suffix;

				now_time = time(NULL);
				passed_time = now_time - sh_time;
				if (passed_time > 0)
					time_remaining = (total_bytes - total) /
					    (total / passed_time);
				if (time_remaining > (60 * 60)) {
					time_remaining /= (60 * 60);
					if (asprintf(&suffix, "%s",
					    "hours") == -1)
						err(1, "asprintf");
				} else {
					time_remaining /= 60;
					if (asprintf(&suffix, "%s",
					    "minutes") == -1)
						err(1, "asprintf");
				}

				percent = total / total_bytes * 100;

				printf("\rPercent completed: %0.2f%% | "
				    "Approximate time remaining: %0.2f %s     ",
				    percent, time_remaining * (passes - i_pass),
				    suffix);
				if (fflush(stdout) != 0) {
					printf("\nCould not fflush stdout\n\n");
					goto done;
				}

				free(suffix);

				elapsed = now;
			}
			if (total_bytes == total)
				break;
		} while (1);
	}

	sh_time = time(NULL);

	if (percent < 100.00)
		printf("\n\nDisk removed or a write error occurred.");

	printf("\n\nFinished shredding %s\n", ctime(&sh_time));

	if (fclose(dd) != 0)
		printf("\nCould not close device %s\n\n", realdev);
done:
	/* re-enable cursor on finish */
	printf("\e[?25h");

	return 0;
}

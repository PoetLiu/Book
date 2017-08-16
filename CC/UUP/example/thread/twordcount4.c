/*
 *
 * Authored by Liu Peng on: Thu Nov  6 16:53:39 CST 2014
 *
 * @desc: threaded word counter for two files.
 *	  version 4: condition variable allows counter
 * 	  functions to report results early
 * @func:	
**/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

void	*count_words(void *);

struct arg_set {	/* two values in one arg */
	char	*fname;	/* file to examine */
	int	count;	/* number of words */
};

struct arg_set *mailbox;
pthread_mutex_t lock	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	flag	= PTHREAD_COND_INITIALIZER;

int main(int ac, char *av[])
{
	pthread_t	t1, t2;		/* two threads */
	struct arg_set	args1, args2;	/* two argsets */
	int		reports_in	= 0;
	int		total_words	= 0;
	int		ret	= 0;

	if (ac != 3) {
		printf("usage:%s file1 file2\n", av[0]);
		exit(1);
	}
	pthread_mutex_lock(&lock);	/* lock the report box now */

	args1.fname	= av[1];
	args1.count	= 0;
	pthread_create(&t1, NULL, count_words, (void *)&args1);

	args2.fname	= av[2];
	args2.count	= 0;
	pthread_create(&t2, NULL, count_words, (void *)&args2);

	while (reports_in < 2) {
		printf("MAIN: waiting for flag to go up\n");
		pthread_cond_wait(&flag, &lock);	/* wait for notify */
		printf("MAIN: Wow! flag was raised, I have the lock\n");
		printf("%7d:%s\n", mailbox->count, mailbox->fname);
		total_words	+= mailbox->count;
		if (mailbox == &args1)
			pthread_join(t1, NULL);
		if (mailbox == &args2)
			pthread_join(t2, NULL);
		mailbox	= NULL;
		pthread_cond_signal(&flag);
		reports_in++;
	}

	printf("%7d:total words\n", total_words);
}

void *count_words(void *a)
{
	struct arg_set	*args	= a;	/* cast arg back to correct type */
	FILE	*fp;
	int	c, prevc	= '\0';

	if ((fp = fopen(args->fname, "r")) != NULL) {
		while ((c = getc(fp)) != EOF) {
			if (!isalnum(c) && isalnum(prevc))
				args->count++;
			prevc	= c;
		}
		fclose(fp);
	} else
		perror(args->fname);
	printf("COUNT:waiting to get lock\n");
	pthread_mutex_lock(&lock);	/* get the mailbox */
	printf("COUNT:have lock, storing data\n");

	if (mailbox != NULL) {
		printf("COUNT:waiting to cond\n");
		pthread_cond_wait(&flag, &lock);
	}
	mailbox	= args;
	printf("COUNT:raising flag\n");
	pthread_cond_signal(&flag);	/* raise the flag */
	printf("COUNT:unlocking box\n");
	pthread_mutex_unlock(&lock);	/* release the mailbox */
	return NULL;
}

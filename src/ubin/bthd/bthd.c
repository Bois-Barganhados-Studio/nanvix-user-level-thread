#include <stdio.h>
#include <sys/types.h>
#include <bthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------*
 *                          PRODUCER-CONSUMER EXAMPLE                         *
 *----------------------------------------------------------------------------*/

#define PROD_CONS_DURATION 0x10
#define BUFFER_SIZE 10

int buffer[BUFFER_SIZE] = { 0 };

int buffer_index = 0;

struct bt_mutex *lock;

void *producer()
{
    int i = 0;
    while (i++ < PROD_CONS_DURATION)
    {
        if (buffer_index < BUFFER_SIZE)
        {
            bthread_mutex_lock(lock);
            if (buffer_index < BUFFER_SIZE)
            {
                int r = rand() % 100 + 1;
                buffer[buffer_index++] = r;
                printf("Produced: %d\n", r);
            }
            bthread_mutex_unlock(lock);
        }
        if (rand() % 2)
            bthread_yield();
    }
    return NULL;
}

void *consumer()
{
    int i = 0;
    while (i++ < PROD_CONS_DURATION)
    {
        if (buffer_index > 0)
        {
            bthread_mutex_lock(lock);
            if (buffer_index > 0)
            {
                int r = buffer[buffer_index - 1];
                buffer[--buffer_index] = 0;
                printf("Consumed: %d\n", r);
            }
            bthread_mutex_unlock(lock);
        }
        if (rand() % 2)
            bthread_yield();
    }
    return NULL;
}

void producer_consumer_test(void)
{
    lock = bthread_mutex_init();
    bthread_t t1, t2;
    bthread_create(&t1, producer, NULL);
    bthread_create(&t2, consumer, NULL);
    bthread_detach(t1);
    bthread_join(t2, NULL);
    bthread_mutex_destroy(lock);

    // Print the buffer
    printf("Buffer: ");
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        printf("%d ", buffer[i]);
    }
    printf("\n");
}

/*----------------------------------------------------------------------------*
 *                          SIMPLE SCHEDULER EXAMPLE                          *
 *----------------------------------------------------------------------------*/

#define N 3
int arr[N] = { 0 };

void *thread(int *arg)
{
    // WARNING - A larger number can cause a stack overflow
    #define THREAD_DURATION 5

    for (int i = 0; i < THREAD_DURATION; i++)
    {
        *arg += 1;
        printf("array: ");
        for (int i = 0; i < N; i++)
        {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
    return NULL;
}

void simple_sched(void)
{
    bthread_t t[N];
    for (int i = 0; i < N; i++)
    {
        bthread_create(&t[i], thread, &arr[i]);
    }
    for (int i = 0; i < N; i++)
    {
        bthread_join(t[i], NULL);
    }
}

/*----------------------------------------------------------------------------*
 *                               MAIN & OPTIONS                               *
 *----------------------------------------------------------------------------*/

/*
 * Prints program version and exits.
 */
static void version(void)
{
	printf("bthd (Bthread Example)\n");
	printf("Copyright(C) 2023 Bthread maintainers\n");
	printf("This is free software under the ");
	printf("GNU General Public License Version 3.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n\n");

	exit(EXIT_SUCCESS);
}

/*
 * Prints program usage and exits.
 */
static void usage(void)
{
	printf("Usage: bthd [options]\n\n");
	printf("Brief: Bthread library examples.\n\n");
	printf("Options:\n");
    printf("  sched     Executes simple scheduling example\n");
    printf("  prodcons  Executes producer-consumer example\n");
	printf("  --help    Display this information and exit\n");
	printf("  --version Display program version and exit\n");

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
		usage();

    if (strcmp(argv[1], "sched") == 0)
    {
        simple_sched();
    }
    else if (strcmp(argv[1], "prodcons") == 0)
    {
        producer_consumer_test();
    }
    else if (strcmp(argv[1], "--version") == 0) {
        version();
    } else {
        usage();
    }

    return (EXIT_SUCCESS);
}


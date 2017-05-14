#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

#define RECEBIDO "RECEBIDO"
#define SERVIDO "SERVIDO"
#define REJEITADO "REJEITADO"

unsigned int program_state = 0;

pid_t this_process;
pthread_t this_thread;
time_t start_inst;

char PATH_REGISTRY_FILE[BUFFER_SIZE];
char * PATH_REQUEST_QUEUE = "/tmp/entrada";
char * PATH_REJECTED_QUEUE = "/tmp/rejeitados";

typedef struct timespec timespec_t;

typedef struct message_t
{
    float inst;
    pid_t pid;
    pthread_t tid;
    unsigned long p;
    char g;
    unsigned int dur;
    char * tip;
}
message_t;

unsigned long number_seats;
unsigned long current_seats = 0;
char current_gender = '\0';

int registry_file;
int request_queue;
int rejected_queue;

pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

void cleanup()
{
    switch(program_state)
    {
        case 5:
            close(rejected_queue);
        case 4:
            close(request_queue);
        case 3:
            unlink(PATH_REJECTED_QUEUE);
        case 2:
            unlink(PATH_REQUEST_QUEUE);
        case 1:
            close(registry_file);
            unlink(PATH_REGISTRY_FILE);
    }
}

void signal_cleanup(int signo)
{
    cleanup();
    
    pthread_exit(NULL);
}

void * wait_seat(void * msg)
{
    timespec_t timespec;
    message_t * message = (message_t *) msg;

    clock_gettime(CLOCK_MONOTONIC, &timespec);

    sleep((message->dur - ((timespec.tv_nsec / 1.0e6 - start_inst) - message->inst)) / 1.0e3);

    int write_bytes;
    char message_buffer[BUFFER_SIZE];

    pthread_mutex_lock(&registry_mutex);

    clock_gettime(CLOCK_MONOTONIC, &timespec);
    message->inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;
    message->tid = pthread_self();
    message->tip = SERVIDO;

    write_bytes = snprintf(message_buffer, BUFFER_SIZE, "%.2f-%lu-%lu-%lu:%c-%u-%s\n", message->inst, (unsigned long) message->pid, (unsigned long) message->tid, message->p, message->g, message->dur, message->tip);
    write_bytes = write(registry_file, message_buffer, write_bytes);

    if (--current_seats == 0)
    {
        current_gender = '\0';
    }

    pthread_mutex_unlock(&registry_mutex);

    free(message);

    pthread_exit(NULL);
    return NULL; // Actually meaningless call, only to remove warning/error
}

void message_listener()
{
    fprintf(stdout, "Listening...\n");

    int valid = 1;
    while (valid)
    {
        timespec_t timespec;
        message_t * message = (message_t *) malloc(sizeof(message_t));

        message->pid = this_process;
        message->tid = this_thread;

        size_t buffer_pos = 0;

        int read_bytes;
        int write_bytes;
        char message_buffer[BUFFER_SIZE];

        do
        {
            read_bytes = read(request_queue, &message_buffer[buffer_pos], 1);
            if (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // Call would block (maybe wait a bit?)
                continue;
            }
            else if (read_bytes > 0)
            {
                // Successfull call
            }
            else
            {
                valid = 0; // FIFO write-side closed
                break;
            }
        }
        while (message_buffer[buffer_pos] != '-' ? (++buffer_pos, 1) : 0);
        if (!valid) break;
        message_buffer[buffer_pos] = '\0';
        message->p = strtoul(message_buffer, NULL, 10);

        read(request_queue, message_buffer, 2);
        message->g = message_buffer[buffer_pos = 0];

        do
        {
            read_bytes = read(request_queue, &message_buffer[buffer_pos], 1);
            if (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                // Call would block (maybe wait a bit?)
                continue;
            }
            else if (read_bytes > 0)
            {
                // Successfull call
            }
            else
            {
                valid = 0; // FIFO write-side closed
                break;
            }
        }
        while (message_buffer[buffer_pos] != '/' ? (++buffer_pos, 1) : 0);
        if (!valid) break;
        message_buffer[buffer_pos] = '\0';
        message->dur = strtoul(message_buffer, NULL, 10);

        message->tip = RECEBIDO;

        pthread_mutex_lock(&registry_mutex);

        clock_gettime(CLOCK_MONOTONIC, &timespec);
        message->inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

        write_bytes = snprintf(message_buffer, BUFFER_SIZE, "%.2f-%lu-%lu-%lu:%c-%u-%s\n", message->inst, (unsigned long) message->pid, (unsigned long) message->tid, message->p, message->g, message->dur, message->tip);
        write_bytes = write(registry_file, message_buffer, write_bytes);

        if (current_seats + 1 <= number_seats && (current_gender == message->g || current_gender == '\0'))
        {
            pthread_t thread;

            ++current_seats;
            current_gender = message->g;

            clock_gettime(CLOCK_MONOTONIC, &timespec);
            message->inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

            if (pthread_create(&thread, NULL, wait_seat, (void *) message) != 0 || pthread_detach(thread) != 0)
            {
                fprintf(stderr, "Failed thread creation\n");
                valid = 0;
            }
        }
        else
        {
            message->tip = REJEITADO;

            clock_gettime(CLOCK_MONOTONIC, &timespec);
            message->inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

            write_bytes = snprintf(message_buffer, BUFFER_SIZE, "%.2f-%lu-%lu-%lu:%c-%u-%s\n", message->inst, (unsigned long) message->pid, (unsigned long) message->tid, message->p, message->g, message->dur, message->tip);
            write(registry_file, message_buffer, write_bytes);

            write_bytes = snprintf(message_buffer, BUFFER_SIZE, "%lu-%c-%u/", message->p, message->g, message->dur);
            write_bytes = write(rejected_queue, message_buffer, write_bytes);
            if (write_bytes <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                valid = 0; // FIFO read-side closed
            }
            else
            {
                // Successfull call
            }

            free(message);
        }

        pthread_mutex_unlock(&registry_mutex);
    }
}

int main(int argc, char ** argv)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_cleanup;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    atexit(cleanup);

    if (argc != 2)
    {
        fprintf(stdout, "Usage: ./sauna <no. of seats>\n");
        return 0;
    }

    this_process = getpid();
    this_thread = pthread_self();
    start_inst = time(NULL);

    snprintf(PATH_REGISTRY_FILE, BUFFER_SIZE, "/tmp/bal.%u", (unsigned int) this_process);

    number_seats = strtoul(argv[1], NULL, 10);

    if (number_seats == 0 || number_seats == ULONG_MAX)
    {
        fprintf(stderr, "Invalid argument! Must be an integer greater than 0 and lesser than");
        return 1; // Runtime error - user failure
    }

    fprintf(stdout, "Initializing...\n");

    registry_file = open(PATH_REGISTRY_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (registry_file == -1)
    {
        fprintf(stderr, "Failed text file creation!\n");
        return 2; // Runtime error - program failure
    }

    ++program_state;

    if (mkfifo(PATH_REQUEST_QUEUE, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    {
        fprintf(stderr, "Failed named pipe creation!\n");
        return 2;
    }

    ++program_state;

    if (mkfifo(PATH_REJECTED_QUEUE, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    {
        fprintf(stderr, "Failed named pipe creation!\n");
        return 2;
    }

    ++program_state;

    request_queue = open(PATH_REQUEST_QUEUE, O_RDONLY);
    if (request_queue == -1)
    {
        fprintf(stderr, "Failed named pipe opening!\n");
        return 2;
    }
    int FLAGS = fcntl(request_queue, F_GETFL);
    fcntl(request_queue, F_SETFL, FLAGS | O_NONBLOCK);

    ++program_state;

    rejected_queue = open(PATH_REJECTED_QUEUE, O_WRONLY);
    if (rejected_queue == -1)
    {
        fprintf(stderr, "Failed named pipe opening!\n");
        return 2;
    }
    FLAGS = fcntl(rejected_queue, F_GETFL);
    fcntl(rejected_queue, F_SETFL, FLAGS | O_NONBLOCK);

    ++program_state;

    fprintf(stdout, "Finished initializing...\n");

    message_listener();

    pthread_exit(NULL);
}

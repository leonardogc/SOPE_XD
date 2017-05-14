#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

typedef struct timespec timespec_t;
time_t start_inst;

unsigned long numPedidos;
unsigned long maxUtilizacao;

int writeFIFO;
int readFIFO;

char * PATH_REQUEST_QUEUE = "/tmp/entrada";
char * PATH_REJECTED_QUEUE = "/tmp/rejeitados";

pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE * file;

int pedidosGeradosHomem=0;
int pedidosGeradosMulher=0;
int pedidosRejeitadosHomem=0;
int pedidosRejeitadosMulher=0;
int pedidosDescartadosHomem=0;
int pedidosDescartadosMulher=0;

void * listenerThread(void * arg){
	float inst;
	unsigned long counter = 0;
	unsigned long p;
	char g;
	unsigned long dur;
	unsigned long rejections;
	int messageLength;
	char message[BUFFER_SIZE];


	timespec_t timespec;

	while(counter<numPedidos){

		unsigned int buffer_pos = 0;

		int read_bytes;
		char message_buffer[BUFFER_SIZE];

		do
		{
			read_bytes = read(readFIFO, &message_buffer[buffer_pos], 1);
			if (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				// Call would block (maybe wait a bit?)
				fprintf(stdout, "foo");
				continue;
			}
			else if (read_bytes > 0)
			{
				fprintf(stdout, "bar");
				// Successfull call
			}
		}
		while (message_buffer[buffer_pos] != '-' ? (++buffer_pos, 1) : 0);

		message_buffer[buffer_pos] = '\0';
		p = strtoul(message_buffer, NULL, 10);

		if(p==0){
			counter++;
		}
		else{

			read(readFIFO, message_buffer, 2);
			g = message_buffer[buffer_pos = 0];

			do
			{
				read_bytes = read(readFIFO, &message_buffer[buffer_pos], 1);
				if (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
				{
					// Call would block (maybe wait a bit?)
					continue;
				}
				else if (read_bytes > 0)
				{
					// Successfull call
				}
			}
			while (message_buffer[buffer_pos] != '-' ? (++buffer_pos, 1) : 0);

			message_buffer[buffer_pos] = '\0';
			dur = strtoul(message_buffer, NULL, 10);

			read(readFIFO, message_buffer, 2);
			message_buffer[1] = '\0';
			rejections = strtoul(message_buffer, NULL, 10);

			if(rejections>=3){
				counter++;
				if(g=='M'){
					pedidosDescartadosHomem++;
				}
				else if(g=='F'){
					pedidosDescartadosMulher++;
				}

			    pthread_mutex_lock(&registry_mutex);

			    clock_gettime(CLOCK_MONOTONIC, &timespec);
			    inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

			    fprintf(file,"%-10.2f - %-10d - %-10lu:%-1c - %-10lu - %-15s\n", inst, (int)getpid(), p , g, dur,"DESCARTADO");

			    pthread_mutex_unlock(&registry_mutex);

			}
			else{

				if(g=='M'){
					pedidosRejeitadosHomem++;
				}
				else if(g=='F'){
					pedidosRejeitadosMulher++;
				}

			    pthread_mutex_lock(&registry_mutex);

			    clock_gettime(CLOCK_MONOTONIC, &timespec);
			    inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

			    fprintf(file,"%-10.2f - %-10d - %-10lu:%-1c - %-10lu - %-15s\n", inst, (int)getpid(), p , g, dur,"REJEITADO");

			    pthread_mutex_unlock(&registry_mutex);

			    messageLength= sprintf(message, "%lu-%c-%lu-%lu/",p,g,dur,rejections);
			    write(writeFIFO,message,messageLength);

			}

		}
	}
	close(writeFIFO);
	close(readFIFO);
	unlink(PATH_REQUEST_QUEUE);
	unlink(PATH_REJECTED_QUEUE);

	 pthread_mutex_lock(&registry_mutex);

	 fprintf(file,"Total Gerados:%d - F:%d - M:%d\nTotal Rejeitados:%d - F:%d - M:%d\nTotal Descartados:%d - F:%d - M:%d",
			 pedidosGeradosHomem+pedidosGeradosMulher,pedidosGeradosMulher, pedidosGeradosHomem,
			 pedidosRejeitadosHomem+pedidosRejeitadosMulher,pedidosRejeitadosMulher,pedidosRejeitadosHomem,
			 pedidosDescartadosHomem+pedidosDescartadosMulher,pedidosDescartadosMulher,pedidosDescartadosHomem);

	 fclose(file);

	 pthread_mutex_unlock(&registry_mutex);

	return NULL;
}

void * geraPedidos(void * arg){
	srand(time(NULL));
	int time;
	int serial=0;
	float inst;
	int messageLength;
	char message[BUFFER_SIZE];
	char gender;
	int counter=0;
	timespec_t timespec;


	while(counter<numPedidos)
	{

	serial++;

	if(rand() % 2 == 0){
		gender='M';
	}
	else{
		gender='F';
	}

	time= (rand() % maxUtilizacao)+1;

	messageLength= sprintf(message, "%d-%c-%d-0/", serial,gender,time);

    write(writeFIFO,message,messageLength);

    pthread_mutex_lock(&registry_mutex);

    clock_gettime(CLOCK_MONOTONIC, &timespec);
    inst = ((float) timespec.tv_nsec / 1.0e6) - start_inst;

    fprintf(file,"%-10.2f - %-10d - %-10d:%-1c - %-10d - %-15s\n", inst, (int) getpid(), serial, gender, time,"PEDIDO");

    pthread_mutex_unlock(&registry_mutex);

    counter++;

    if(gender=='M'){
    	pedidosGeradosHomem++;
    }
    else if(gender=='F'){
    	pedidosGeradosMulher++;
    }

	}

 return NULL;
}


int main(int argc, char ** argv){

	start_inst = time(NULL) * 1000;

	if (argc != 3)
	    {
	        fprintf(stdout, "Usage: ./gerador <n. pedidos> <max. utilização>\n");
	        return 0;
	    }

	numPedidos = strtoul(argv[1], NULL, 10);
	maxUtilizacao = strtoul(argv[2], NULL, 10);

	    if (numPedidos == 0 || numPedidos == ULONG_MAX || maxUtilizacao == 0 || maxUtilizacao == ULONG_MAX)
	    {
	        fprintf(stderr, "Invalid argument! Must be an integer greater than 0 and lesser than %lu", ULONG_MAX);
	        return 1; // Runtime error - user failure
	    }

	char filename[BUFFER_SIZE];
	sprintf(filename,"/tmp/ger.%d",(int)getpid());
	file=fopen(filename,"w");

	pthread_t ta, tb;
	
	writeFIFO = open(PATH_REQUEST_QUEUE, O_WRONLY);
	readFIFO = open(PATH_REJECTED_QUEUE, O_RDONLY);

	pthread_create(&ta, NULL, geraPedidos, NULL);
	pthread_create(&tb, NULL, listenerThread, NULL);
	pthread_join(ta, NULL);
	pthread_join(tb, NULL);

	return 0;
}

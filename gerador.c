#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

unsigned long numPedidos;
unsigned long maxUtilizacao;

int writeFIFO;
int readFIFO;

char * PATH_REQUEST_QUEUE = "/tmp/entrada";
char * PATH_REJECTED_QUEUE = "/tmp/rejeitados";

pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE * file;

/*
ssize_t read (int fd, char * buf, int count);
ssize_t write (int fd, char * buf, int count);
*/

void * listenerThread(void * arg){
	int fd;
	int counter = 0;
	char str[100];

	while(counter<numPedidos){













	}

	close(writeFIFO);
	close(readFIFO);


 return NULL;
}

void * geraPedidos(void * arg){
	srand(time(NULL));
	int fd;
	int time;
	int serial=0;
	int messageLength;
	char message[100];
	char gender;
	int counter=0;


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

    fprintf(file,"%-10.2f – %-10d – %-10d: %-1c – %-10d – %-15s\n", inst, getpid(), serial , gender, time,"PEDIDO");

    pthread_mutex_unlock(&registry_mutex);

    counter++;
	}

 return NULL;
}


int main(int argc, char ** argv){

	if (argc != 3)
	    {
	        fprintf(stdout, "Usage: ./sauna <no. of seats>\n");
	        return 0;
	    }

	numPedidos = strtoul(argv[1], NULL, 10);
	maxUtilizacao = strtoul(argv[2], NULL, 10);

	    if (numPedidos == 0 || numPedidos == ULONG_MAX || maxUtilizacao == 0 || maxUtilizacao == ULONG_MAX)
	    {
	        fprintf(stderr, "Invalid argument! Must be an integer greater than 0 and lesser than");
	        return 1; // Runtime error - user failure
	    }

	char* filename;
	sprintf(filename,"ger.%d",getpid());
	file=fopen(filename,"w");

	pthread_t ta, tb;

	readFIFO= open (PATH_REJECTED_QUEUE, O_RDONLY);
	writeFIFO= open (PATH_REQUEST_QUEUE,O_WRONLY);

	pthread_create(&ta, NULL, geraPedidos, NULL);
	pthread_create(&tb, NULL, listenerThread, NULL);
	pthread_join(ta, NULL);
	pthread_join(tb, NULL);

	return 0;
}

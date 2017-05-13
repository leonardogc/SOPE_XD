#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

int numPedidos=10;
int maxUtilizacao=10000;

/*
ssize_t read (int fd, char * buf, int count);
ssize_t write (int fd, char * buf, int count);
*/

void * listenerThread(void * arg){
	int fd;
	char str[100];

	fd= open (/*const char *filename*/, O_RDONLY);

	do{

	n = read(fd,str,1);

	}
	while (n>0 && *str++ != '\0');


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


     fd= open (/*const char *filename*/,O_WRONLY);

	/*falta o loop*/

	/*falta o numero de serie*/

	if(rand() % 2 == 0){
		gender='M';
	}
	else{
		gender='F';
	}

	time= (rand() % maxUtilizacao)+1;

    sprintf(message, "%d-%c-%d/", serial,gender,time);

    messageLength=strlen(message)+1;

    write(fd,message,messageLength);


 return NULL;
}


int main(){
pthread_t ta, tb;

pthread_create(&ta, NULL, geraPedidos, NULL);
pthread_create(&tb, NULL, listenerThread, NULL);
pthread_join(ta, NULL);
pthread_join(tb, NULL);


 return 0;
}

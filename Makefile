make:
	gcc gerador.c -pthread -Wall -Werror -o gerador
	gcc sauna.c -pthread -Wall -Werror -o sauna

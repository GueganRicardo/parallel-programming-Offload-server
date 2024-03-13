// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define NOME_PIPE "pipe_tarefas"
#define MAX_NOME 100

int main (int argc, char * argv[]) {
    int fd; 
    char info[MAX_NOME];
    if (argc != 5){
        perror("Erro no número de parâmetros");
        return 1;
    }
    for (int i = 1; i < 5; i++){
        if (!isdigit(*argv[i])) {
            perror("Erro num dos parâmetros da linha de comandos");
            return 1;
        }
    }
    int n_pedidos=atoi(argv[1]);
    int intervalo_entre_pedidos = atoi(argv[2]);
    if((fd = open(NOME_PIPE,O_WRONLY))<0){
        perror("Problemas na criação do pipe");
        exit(0);
    }
   	strcpy(info, "codigo_tarefa "); //identificador para o início da tarefa!
    strcat(info,argv[3]);
    strcat(info," ");
    strcat(info,argv[4]);
    strcat(info,"\n");
    for (int i = 0; i < n_pedidos; i++){
        write(fd,&info,sizeof(info));
        usleep(intervalo_entre_pedidos*1000); // a funcao recebe micro (*1000 para milis)
    }
    return 0;
}

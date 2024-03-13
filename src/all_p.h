// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/msg.h>

#define MAX_NOME 100
#define key 1000
#define NOME_PIPE "pipe_tarefas"


struct comun{
	int tarefas_espera;
	int max_time;
    int tarefas_realizadas;
    int tarefas_falhadas;
    float tempo_medio;
    int pids[20];
};

struct edge_server{
	sem_t mutex_vcpu2;
	sem_t mutex_dados;
    char nome[MAX_NOME];
    int tarefas_realizadas;
    int potencias[2];
    int manutencoes;
    int ocup[2];//0->livre, 1->ocupado
    int fd[2];
};

struct data_block{
    struct comun all;
    struct edge_server servers[];
};

struct tarefa {
    int id;
    int n_inst;
    int max_temp_exec;
    int arrival;
    int cpu_dest;
    struct tarefa *next;
};

struct mensagem{
	long destinatario;
	bool ativ;
	int tempo_manutencao;
};

struct man_servers{
	bool man;
	int tempo_man;
	int inicio_man;
};

struct man_confirm{
	long destinatario;
	int id_server;
	int tempo_inicio;
};

int id_SHM, i, n_servers, tam_fila,id_message_queue,max_time;
char mensagem_log[MAX_NOME];
sem_t *mutex_comun, *mutex_log, *mutex_monitor, *vcpu_livres;
struct data_block * SHM;

int get_time();
void task_manager();
void monitor();
void mantainance_manager();
void edge_server(int id);
void log_message(char string[]);
void time_concat(char msg[]);

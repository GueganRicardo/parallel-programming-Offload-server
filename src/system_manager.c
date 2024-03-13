// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include "all_p.h"

int n_virgulas(char * string){
    int v = 0;
    for (int i = 0; i < strlen(string); i++) if (string[i] == ',') v++;
    return v;
}

int get_time(){
	time_t tempo;
	struct tm *tempoz;
	tempo=time(0);
	int tempo_sec;
	
	if((tempoz=gmtime(&tempo))==NULL){
		perror("Erro na no calculo do tempo");
		exit(0);
	}
	tempo_sec=3600*tempoz->tm_hour+60*tempoz->tm_min+tempoz->tm_sec;//não passar messes/ meias_noites
	return tempo_sec;
}

void lefichconfig(FILE *f, int n_es){
    char linha[100];
    char parte_linha[50];
    int contador_linhas_lidas = 0;
    while(fscanf (f, "%s", linha)!=EOF && strlen(linha) > 4 && contador_linhas_lidas != n_es){
        if (n_virgulas(linha) != 2) {
            perror("ERRO nas linhas do edge server (ficheiro de config)");
            exit(1);
        }
            strcpy(SHM->servers[contador_linhas_lidas].nome,strtok(linha,","));
            strcpy(parte_linha, strtok(NULL,","));
            if (!isdigit(*parte_linha)){
                perror("ERRO nas linhas do edge server (ficheiro de config) (numa potencia de vcpu1)");
                exit(1);
            }
            SHM->servers[contador_linhas_lidas].potencias[0]=(int)strtol(parte_linha,NULL,10);
            strcpy(parte_linha, strtok(NULL,","));
            if (!isdigit(*parte_linha)){
                perror("ERRO nas linhas do edge server (ficheiro de config) (numa potencia de vcpu1)");
                exit(1);
            }
            SHM->servers[contador_linhas_lidas].potencias[1]=(int)strtol(parte_linha,NULL,10);
            contador_linhas_lidas++;
    }
    fclose(f);
    return;
}


void time_concat(char msg[]){
    time_t now_time = time(NULL);
    struct tm *now_tm = localtime(&now_time);
    char str[MAX_NOME]="";
    strftime(str, sizeof(str), "%H:%M:%S ",now_tm);
    strcat(str,msg);
    strcpy(msg, str);
}


void log_message(char string[]){
    time_concat(string);
    printf("%s", string);
    fflush(stdout);
    FILE * f = fopen("log.txt","a");
    fprintf(f, "%s", string);
    fclose(f);
}


void terminate(){
    for(i=0;i<3;i++) wait(NULL);
    raise(SIGTSTP);
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Fim do programa\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    sem_wait(mutex_log);
    for(i=0;i<n_servers;i++){
		sem_destroy(&SHM->servers[i].mutex_dados);
		sem_destroy(&SHM->servers[i].mutex_vcpu2);
	}
    strcpy(mensagem_log, "Tudo limpo\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    sem_close(mutex_comun);
    sem_close(mutex_log);
    sem_close(mutex_monitor);
    sem_close(vcpu_livres);
    sem_unlink("MUTEX_LOG");
    sem_unlink("MUTEX_COMUN");
    sem_unlink("MUTEX_MONITOR");
    sem_unlink("VCPUS_LIVRES");
    msgctl(id_message_queue,IPC_RMID,NULL);
    shmctl(id_SHM,IPC_RMID,NULL);
}


void SIG_fecha(int signum){
	sem_wait(mutex_log);
	strcpy(mensagem_log,"Inicio do fecho\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
}


void SIG_stats(int signum){
	//signal(SIGTSTP,SIG_IGN);
	sem_wait(mutex_log);
	printf("Numero total de tarefas realizadas: %d\n",SHM->all.tarefas_realizadas);
	printf("Tempo medio por tarefa: %f\n",SHM->all.tempo_medio);
	for(i=0;i<n_servers;i++){
	sem_wait(&SHM->servers[i].mutex_dados);
		printf("----------%s----------\n",SHM->servers[i].nome);
		printf("Realizei: %d tarefas\n",SHM->servers[i].tarefas_realizadas);
		printf("Fiz %d operacoes de manutencao\n",SHM->servers[i].manutencoes);
	sem_post(&SHM->servers[i].mutex_dados);
	}
	printf("O numero de tarefas que expiraram é: %d\n",SHM->all.tarefas_falhadas);
	sem_post(mutex_log);
	//signal(SIGTSTP,SIG_stats);
}

void init_Sems(){
	sem_unlink("VCPUS_LIVRES");
	vcpu_livres=sem_open("VCPUS_LIVRES",O_CREAT|O_EXCL,0700,n_servers);
	for(i=0;i<n_servers;i++){
		sem_init(&SHM->servers[i].mutex_dados,1,1);
		sem_init(&SHM->servers[i].mutex_vcpu2,1,0);
	}
	for(i=0;i<n_servers;i++){
		SHM->servers[i].ocup[0] = 0;
		SHM->servers[i].ocup[1] = 1;
	}
}

void initIPC(){
    if ((id_SHM = shmget(key,(n_servers*sizeof(struct edge_server)+sizeof(struct comun)+sizeof(struct data_block)), IPC_CREAT | 0777)) < 0) {
        perror("erro no shmat");
        exit(0);
    }
    SHM = shmat(id_SHM, NULL, 0);
	sem_unlink("MUTEX_LOG");
	mutex_log=sem_open("MUTEX_LOG",O_CREAT|O_EXCL,0700,1);
	sem_unlink("MUTEX_COMUN");
	mutex_comun=sem_open("MUTEX_COMUN",O_CREAT|O_EXCL,0700,1);
	sem_unlink("MUTEX_MONITOR");
	mutex_monitor=sem_open("MUTEX_MONITOR",O_CREAT|O_EXCL,0700,1);
	if(((id_message_queue = msgget(IPC_PRIVATE,IPC_CREAT|0777))==-1)){
		perror("erro na criacao da messague queue");
	}
}

int main (int argc, char * argv[]) {
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    //inicia os semaforos
    initIPC();
    if (argc != 2){
        perror("erro nos argumentos de entrada");
        exit(0);
    }
    FILE *file_config = NULL;
    file_config = fopen(argv[1], "r"); //******** mudar para linha de comandos

    //validação do ficheiro de configurações
    if (file_config == NULL) {
        perror("erro no ficheiro de entrada");
        exit(0);
    }
    //validação do ficheiro de configurações

    //config_zone ******
    char linha[MAX_NOME] = "";
    fgets(linha, MAX_NOME, file_config);
    if (!isdigit(*linha)){
        perror("erro na linha 1 do ficheiro de configurações");
        exit(0);
    }
    tam_fila = (int) strtol(linha, NULL, 10); //fila do task_manager
    fgets(linha, MAX_NOME, file_config);
    if (!isdigit(*linha)){
        perror("erro na linha 2 do ficheiro de configurações");
        exit(0);
    }
    max_time = (int) strtol(linha, NULL, 10);
    fgets(linha, MAX_NOME, file_config);
    if (!isdigit(*linha)){
        perror("erro na linha 3 do ficheiro de configurações");
        exit(0);
    }
    n_servers = (int) strtol(linha, NULL, 10);
    lefichconfig(file_config, n_servers);
    FILE * f = fopen("log.txt", "w");
    fclose(f);
    init_Sems();//inicia os semaforos do edge_servers
    sem_wait(mutex_log);
    strcpy(mensagem_log, "Criacao do Task Manager\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    int controlo_processos = fork();
    if (controlo_processos == 0) {  // filho
        task_manager();
        exit(0);
    }
    SHM->all.pids[0]=controlo_processos;
    sem_wait(mutex_log);
    strcpy(mensagem_log,"Criacao do Monitor\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    controlo_processos = fork();
    if (controlo_processos == 0) {  // filho
        monitor();
        exit(0);
    }
    SHM->all.pids[1]=controlo_processos;
    sem_wait(mutex_log);
    strcpy(mensagem_log,"Criacao do Maintenance Manager\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    controlo_processos = fork();
    if (controlo_processos == 0) {  // filho
        mantainance_manager();
        exit(0);
    }
    SHM->all.pids[2]=controlo_processos;
    //Ativar Sinais
    sem_wait(mutex_log);
    strcpy(mensagem_log,"Ativacao dos sinais\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    signal(SIGINT,SIG_fecha);
    signal(SIGTSTP,SIG_stats);
    //Inicio da info
    sem_wait(mutex_comun);
    SHM->all.tarefas_realizadas=0;
    SHM->all.tarefas_falhadas=0;
    sem_post(mutex_comun);
    //Limpar
    terminate();
    exit(0);
}

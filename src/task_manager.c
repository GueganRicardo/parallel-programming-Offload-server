// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include "all_p.h"

struct tarefa *raiz, *nova;
pthread_t * schd_disp;
sem_t livres, cheios, mutex_fila;
pthread_mutex_t mutex_tarefa = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_tarefa_cheia = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_tarefa_vazia = PTHREAD_COND_INITIALIZER;

void log_out_in_fila_scheduler(int out_in, int id_tarefa);


void limpa_fila_tarefas(){
	struct tarefa * cur;
	cur = raiz;
	while (cur != NULL) {
		SHM->all.tarefas_espera--;
		log_out_in_fila_scheduler(-1, cur->id);
	    cur = cur->next;
    }
    raiz = NULL;
}


void SIG_fechaTM(int signum){
	signal(SIGINT,SIG_IGN);
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Fim do Task Manager\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
	pthread_kill(schd_disp[0],SIGUSR2);
	pthread_kill(schd_disp[1],SIGUSR1);
	for(i=0;i<n_servers;i++) wait(NULL);
    pthread_join(schd_disp[0], NULL);
    pthread_join(schd_disp[1], NULL);
    sem_destroy(&cheios);
    sem_destroy(&mutex_fila);
    sem_destroy(&livres);
    pthread_mutex_destroy(&mutex_tarefa);
    pthread_cond_destroy(&cond_tarefa_vazia);
    pthread_cond_destroy(&cond_tarefa_cheia);
    pthread_exit(NULL);
    exit(0);
}


void SIG_fechasch(){
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Fim do Scheduler\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    limpa_fila_tarefas();
    pthread_exit(NULL);
}

void SIG_fechadisp(){
	pthread_exit(NULL);
}


void log_out_in_fila_scheduler(int out_in, int id_tarefa) { // out -1 / in 1 
    sem_wait(mutex_log);
	char snum[8];
	int length = sprintf(snum, "%d", id_tarefa);
	snum[length] = '\0';
	strcpy(mensagem_log, "Tarefa de ID ");
	strcat(mensagem_log, snum);
	if (out_in == -1){
		sem_wait(mutex_comun);
		SHM->all.tarefas_falhadas++;
		sem_post(mutex_comun);
		strcat(mensagem_log, " foi descartada\n");
		}
	else if (out_in == 1)
		strcat(mensagem_log, " foi escalonada\n");
    log_message(mensagem_log);
    sem_post(mutex_log); 
}


struct tarefa * adiciona(struct tarefa * n, struct tarefa * r) {
    int already_waited = 0;
    int max_time_local = 0;
    struct tarefa * cur = r;
    struct tarefa * ant = NULL;
    int aux = 0;
    if (r == NULL){
        r = n;
        r->next = NULL;
        SHM->all.tarefas_espera = 1;
        log_out_in_fila_scheduler(1, n->id);
        SHM->all.max_time = max_time_local;
        sem_post(mutex_monitor);
    }
    else {
        while (1) {
	    already_waited = get_time() - cur->arrival;
            if (cur->max_temp_exec > n->max_temp_exec) { // já encontrámos um mais demorado
                n->next = cur;
                if (ant != NULL) {
                    ant->next = n;
                    ant = ant->next;
                }
                else {
                    r = n;
                    ant = r;
                }
                log_out_in_fila_scheduler(1, n->id);
                SHM->all.tarefas_espera++;
                sem_post(mutex_monitor);
                break;
            }
            // era menos demorado
            if (cur->max_temp_exec < already_waited) {
                if (cur->next != NULL) {
                    if (ant == NULL) {
                        r = cur->next;
                        aux = 1;
                    }
                    else
                        ant->next = cur->next;
                }
                else {
                    if (ant == NULL)
                        r = n;
                    else
                        ant->next = n;
                }
                SHM->all.tarefas_espera--;
                sem_post(&livres);
                sem_wait(&cheios);
                log_out_in_fila_scheduler(-1, cur->id);
                sem_post(mutex_monitor);
            }
            if (cur == NULL || cur->next == NULL) { // já não há nada a seguir
                cur->next = n;
                SHM->all.tarefas_espera++;
                log_out_in_fila_scheduler(1, n->id);
                sem_post(mutex_monitor);
                break;
            }
            if (aux == 1){
                cur = r;
                ant = NULL;
                aux = 0;
            }
            else {
                ant = cur;
                cur = cur->next;
            }
	    if (max_time_local < already_waited) max_time_local = already_waited;
        }
        while (cur != NULL) {
	    already_waited = get_time() - cur->arrival;
            if (cur->max_temp_exec < already_waited) {
                if (cur->next != NULL) {
                    if (ant == NULL)
                        r = cur->next;
                    else
                        ant->next = cur->next;
                }
                else {
                    if (ant == NULL)
                        r = NULL;
                    else
                        ant->next = NULL;
                }
                SHM->all.tarefas_espera--;
                sem_post(&livres);
                sem_wait(&cheios);
                log_out_in_fila_scheduler(-1, cur->id);
                sem_post(mutex_monitor);
            }
        cur = cur->next;
	    if (max_time_local < already_waited) max_time_local = already_waited;
        }
    }
    SHM->all.max_time =max_time_local;
    return r;
}




void *schedueler(void *id){
	signal(SIGUSR2,SIG_fechasch);
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Inicio do Scheduler\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    while (1){
		pthread_mutex_lock(&mutex_tarefa);
	while(nova==NULL){
		pthread_cond_wait(&cond_tarefa_cheia,&mutex_tarefa);
	}
	sem_wait(&livres);
	sem_wait(&mutex_fila);
    raiz = adiciona(nova, raiz);
	sem_post(&mutex_fila);
	sem_post(&cheios);
    nova=NULL;
    pthread_cond_signal(&cond_tarefa_vazia);
    pthread_mutex_unlock(&mutex_tarefa);		
	}
}

void *dispatcher(void *id){
	struct tarefa * a_enviar;
	int k;
	signal(SIGUSR1,SIG_fechadisp);
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Inicio do Dispatcher\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    while(1) {
		sem_wait(vcpu_livres);
		sem_wait(&cheios);
		sem_wait(&mutex_fila);
		//condição de escrita
		a_enviar = raiz;
		raiz = raiz->next;
		SHM->all.tarefas_espera--;
		sem_wait(mutex_log);
        sprintf(mensagem_log, "Tarefa de ID %d selecionada para ser escalonada\n", a_enviar->id);
        log_message(mensagem_log);
        sem_post(mutex_log);
		sem_post(mutex_monitor);
		sem_post(&mutex_fila);
		sem_post(&livres);
		for (k = (n_servers * 2)-1; k >= 0 ; k--){
			sem_wait(&SHM->servers[k/2].mutex_dados);
			if (SHM->servers[k/2].ocup[k%2] == 0) { //estava e livre
				if (get_time() + a_enviar->n_inst/SHM->servers[k/2].potencias[k%2] < a_enviar->max_temp_exec+a_enviar->arrival){ // era capaz
					a_enviar->cpu_dest = k%2;
        			write(SHM->servers[k/2].fd[1],a_enviar,sizeof(struct tarefa));
        			sem_wait(mutex_log);
        			sprintf(mensagem_log, "Tarefa de ID %d foi enviada para o server %d, vcpu %d\n", a_enviar->id, k/2, k%2);
        			log_message(mensagem_log);
        			sem_post(mutex_log);
        			free(a_enviar);
        			a_enviar = NULL;
        			sem_post(&SHM->servers[k/2].mutex_dados);
        			SHM->servers[k/2].ocup[k%2] = 1;
        			break;
    			}
    		}
    		sem_post(&SHM->servers[k/2].mutex_dados);
    	}
    	if (a_enviar != NULL){
    		sem_post(vcpu_livres);
        	log_out_in_fila_scheduler(-1, a_enviar->id);
        	free(a_enviar);
        	a_enviar = NULL;
    	}
    }
}

void initTM(){
	//criar os edge servers
    for (i = 0; i < n_servers; i++){
    pipe(SHM->servers[i].fd);//0->read 1->write
        int pid = fork();
        if (pid == 0){
        sem_wait(mutex_log);
            strcpy(mensagem_log,"Criação do edge server\n");
            log_message(mensagem_log);
            sem_post(mutex_log);
            edge_server(i);
            exit(0);
        }
        SHM->all.pids[i+3]=pid;
    }
    //inicia os IPCS
    SHM = shmat(id_SHM, NULL, 0);
    sem_init(&mutex_fila,0,1);
    sem_init(&cheios,0,0);
    sem_init(&livres,0,tam_fila);
    //inicia as threads dispatcher schedueler
    raiz = NULL;
    nova = NULL;
    schd_disp = (pthread_t *) malloc(sizeof (pthread_t) * 2);
    pthread_create(&schd_disp[0], NULL, schedueler, NULL);
    pthread_create(&schd_disp[1], NULL, dispatcher, NULL);
}

void task_manager(){
	signal(SIGINT,SIG_fechaTM);
	int k,fd, conta_tarefas=0;
	struct tarefa *tarefa;
	char info[MAX_NOME];
	char comando[MAX_NOME];
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Inicio do Task Manager\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    initTM();
    sem_wait(mutex_log);
    strcpy(mensagem_log,"Abrir o pipe\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    //cria o pipe
        if((mkfifo(NOME_PIPE, O_CREAT|O_EXCL|0600)<0) && (errno!=EEXIST)){
    	perror("problemas na criação do pipe");
    	exit(0);
    }
        while(1){
        	if((fd = open(NOME_PIPE,O_RDONLY))<0)	{
    		perror("problemas na criação do pipe");
			exit(0);
    		}
    	strcpy(info, "");
    	read(fd,&info,sizeof(info));
    	strcpy(comando,strtok(info," \n"));
    	if (strcmp(comando, "EXIT") == 0){
    			sem_wait(mutex_log);
    			strcpy(mensagem_log,"Pedido de EXIT recebido por linha de comandos\n");
    			log_message(mensagem_log);
    			sem_post(mutex_log);
    			for(k=1;k<n_servers+3;k++){
    				kill(SHM->all.pids[k],SIGINT);
    			}
    			kill(getppid(),SIGINT);
    			raise(SIGINT);			
    	}
    	else if (strcmp(comando, "STATS") == 0){
    			sem_wait(mutex_log);
    			strcpy(mensagem_log,"Pedido de STATS recebido por linha de comandos\n");
    			log_message(mensagem_log);
    			sem_post(mutex_log);
    			kill(getppid(),SIGTSTP);
    	}else if (strcmp(comando, "codigo_tarefa") == 0) {
    		tarefa=(struct tarefa*) malloc(sizeof(struct tarefa));
    		tarefa->id=conta_tarefas;
    		tarefa->n_inst=atoi(strtok(NULL," "));
    		tarefa->max_temp_exec=atoi(strtok(NULL," \n"));
    		tarefa->arrival=get_time();//inserir o tempo a que chegou   
    		tarefa->next = NULL; 	
    		conta_tarefas++;
    		sem_wait(mutex_log);
    		strcpy(mensagem_log,"Nova tarefa recebida\n");
    		log_message(mensagem_log);
    		sem_post(mutex_log);
    		pthread_mutex_lock(&mutex_tarefa);
    	}
    	
    	else {
    		sem_wait(mutex_log);
    		strcpy(mensagem_log,"WRONG COMMAND\n");
    		log_message(mensagem_log);
    		sem_post(mutex_log);
    	}
    
    		while(nova!=NULL){
    			pthread_cond_wait(&cond_tarefa_vazia,&mutex_tarefa);
    		}
    	nova = tarefa;
    	tarefa = NULL;
    	pthread_cond_signal(&cond_tarefa_cheia);
    	pthread_mutex_unlock(&mutex_tarefa);
    	
    }
}

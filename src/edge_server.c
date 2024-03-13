// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include "all_p.h"

struct vcpu{
	int id;
	int potencia;
};

struct tarefa tarefa;
pthread_t * vcpus, leitor_pipes;
struct mensagem manutencao;
int ocup[2];
int id_server;
pthread_mutex_t mutex_finalizar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tarefacpu1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tarefacpu2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_final = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_vcpu1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_vcpu0 = PTHREAD_COND_INITIALIZER;

void SIG_fechaES(int signum){
	signal(SIGINT,SIG_IGN);
    while(ocup[1]!=0 || ocup[0]!=0){//garantir que não começam tarefa
    	pthread_cond_wait(&cond_final,&mutex_finalizar);
    }
	pthread_kill(vcpus[0],SIGUSR1);
	pthread_kill(vcpus[1],SIGUSR1);
	pthread_kill(leitor_pipes,SIGUSR2);
	pthread_join(vcpus[0], NULL);
    pthread_join(vcpus[1], NULL);
    pthread_join(leitor_pipes, NULL);
    sem_wait(mutex_log);
    pthread_cond_destroy(&cond_final);
    pthread_cond_destroy(&cond_vcpu1);
    pthread_cond_destroy(&cond_vcpu0);
    pthread_mutex_destroy(&mutex_finalizar);
    pthread_mutex_destroy(&mutex_tarefacpu1);
    pthread_mutex_destroy(&mutex_tarefacpu2);
    strcpy(mensagem_log,"Fim do Edgeserver\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    pthread_exit(NULL);
    exit(0);
}

void SIG_fechacpu(int signum){
	sem_wait(mutex_log);
	strcpy(mensagem_log,"Fim do Vcpu\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
	pthread_exit(NULL);
}

void SIG_fechaleitor(int signum){
	sem_wait(mutex_log);
	strcpy(mensagem_log,"Fim do Leitor do pipe\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
	pthread_exit(NULL);
}
void * leitor_pipe(void *id){
	signal(SIGUSR2,SIG_fechaleitor);
    while(1){
    	read(SHM->servers[i].fd[0],&tarefa,sizeof(tarefa));
    if(tarefa.cpu_dest==1){
    	pthread_cond_signal(&cond_vcpu1);
    }
    if(tarefa.cpu_dest==0){
    	pthread_cond_signal(&cond_vcpu0);
    	}
    }
}

void * v_cpu (void *void_info) {
	signal(SIGUSR1,SIG_fechacpu);
	int id_tarefa, n_inst,tempo_chegada;
    sem_post(mutex_log);
    struct vcpu *minha_info=(struct vcpu*) void_info;
    sem_wait(mutex_log);
    strcpy(mensagem_log,"Início do vcpu\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    while(1){
    if(minha_info->id==1){
    sem_wait(&SHM->servers[id_server].mutex_vcpu2);//permissão do monitor
    pthread_mutex_lock(&mutex_tarefacpu1);
    	while(tarefa.cpu_dest!=1){
    		pthread_cond_wait(&cond_vcpu1,&mutex_tarefacpu1);
    	}
    pthread_mutex_lock(&mutex_finalizar);
    ocup[minha_info->id]=1;
    pthread_mutex_unlock(&mutex_finalizar);
   	id_tarefa=tarefa.id;
    n_inst=tarefa.n_inst;
    tempo_chegada=tarefa.arrival;
    tarefa.cpu_dest=-1;
    pthread_mutex_unlock(&mutex_tarefacpu1);
    }
    if(minha_info->id==0){
    pthread_mutex_lock(&mutex_tarefacpu2);
    	while(tarefa.cpu_dest!=0){
    		pthread_cond_wait(&cond_vcpu0,&mutex_tarefacpu2);
    	}
    pthread_mutex_lock(&mutex_finalizar);
    ocup[minha_info->id]=1;
    pthread_mutex_unlock(&mutex_finalizar);
    id_tarefa=tarefa.id;
    n_inst=tarefa.n_inst;
    tempo_chegada=tarefa.arrival;
    tarefa.cpu_dest=-1;
    pthread_mutex_unlock(&mutex_tarefacpu2);
    }
    sleep(n_inst/minha_info->potencia);//resolveu a tarefa
    if(minha_info->id==1){
    sem_post(&SHM->servers[id_server].mutex_vcpu2);
    }
    pthread_mutex_lock(&mutex_finalizar);
    ocup[minha_info->id]=0;
    pthread_mutex_unlock(&mutex_finalizar);
    pthread_cond_signal(&cond_final);
    sem_wait(mutex_comun);
    SHM->all.tarefas_realizadas++;
    //calcular o tempo medio
    SHM->all.tempo_medio = (float)(((SHM->all.tarefas_realizadas-1)*SHM->all.tempo_medio)/SHM->all.tarefas_realizadas)+(float)((get_time()-tempo_chegada)/SHM->all.tarefas_realizadas);
    sem_post(mutex_comun);
    sem_wait(&SHM->servers[id_server].mutex_dados);
    SHM->servers[id_server].ocup[minha_info->id]=0;
    SHM->servers[id_server].tarefas_realizadas++;
    sem_post(&SHM->servers[id_server].mutex_dados);
    sem_post(vcpu_livres);
    sem_wait(mutex_log);
	sprintf(mensagem_log, "Fim da execução da tarefa %d\n", id_tarefa);
	log_message(mensagem_log);
    sem_post(mutex_log);
    }
}

void edge_server(int id){
	SHM = shmat(id_SHM, NULL, 0);
	id_server=id;
	struct man_confirm confirm;
    manutencao.ativ=false;
    tarefa.cpu_dest=-1;
    pthread_create(&leitor_pipes,NULL,leitor_pipe,NULL);
    vcpus = (pthread_t *) malloc(sizeof (pthread_t) * 2);
    struct vcpu vcpu1;
    vcpu1.potencia=SHM->servers[id].potencias[0];
    vcpu1.id=1;
    pthread_create(&vcpus[0],NULL,v_cpu,&vcpu1); //vcpu1
    struct vcpu vcpu2;
    vcpu2.potencia=SHM->servers[id].potencias[1];
    vcpu2.id=0;
    pthread_create(&vcpus[1],NULL,v_cpu,&vcpu2); //vcpu2
    signal(SIGINT,SIG_fechaES);
    signal(SIGUSR1,SIG_IGN);
    while(1){
    while(!manutencao.ativ){
    	msgrcv(id_message_queue,&manutencao,sizeof(struct mensagem)-sizeof(long),id+1,0);
    	}
    	if(manutencao.ativ){
    	confirm.id_server=id;
    	confirm.destinatario=n_servers+1;
    	confirm.tempo_inicio=get_time();
    	msgsnd(id_message_queue,&confirm,sizeof(struct man_confirm)-sizeof(long),0);
    	sem_wait(mutex_log);
		sprintf(mensagem_log, "A iniciar manutenção de %d segundos no edge server %d\n", manutencao.tempo_manutencao, id);
		log_message(mensagem_log);
    	sem_post(mutex_log);
    	pthread_mutex_lock(&mutex_finalizar);//garantir que não começam tarefas
    	while(ocup[1]!=0 && ocup[0]!=0){
    		pthread_cond_wait(&cond_final,&mutex_finalizar);
    	}
    	SHM->servers[id].manutencoes++;
    	sleep(manutencao.tempo_manutencao);
    	while(manutencao.ativ){
    			msgrcv(id_message_queue,&manutencao,sizeof(struct mensagem)-sizeof(long),id+1,0);
    		}
    	}
    	pthread_mutex_unlock(&mutex_finalizar);//podem continuar
    }
}

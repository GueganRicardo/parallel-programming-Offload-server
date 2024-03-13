// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include "all_p.h"

void SIG_fecham(int signum){
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Fim do Monitor\n");
    log_message(mensagem_log);
    sem_post(mutex_log);	
	signal(SIGINT,SIG_IGN);
	exit(0);
}

void monitor(){
	int time, fila_espera ,k;
	float percent_fila;
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Inicio do Monitor\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
    signal(SIGINT,SIG_fecham);
    SHM = shmat(id_SHM, NULL, 0);
    while(1){
    	while(1){
    		sem_wait(mutex_monitor);
    		sem_wait(mutex_comun);
    		time=SHM->all.max_time;
    		fila_espera=SHM->all.tarefas_espera;
    		sem_post(mutex_comun);
    		percent_fila=fila_espera*100/tam_fila;
    		if(time>max_time || (percent_fila>=80)){
    			break;
    		}
    	}
    	sem_wait(mutex_log);
        sprintf(mensagem_log, "A iniciar modo alta performance\n");
        log_message(mensagem_log);
        sem_post(mutex_log);
    	for(k=0;k<n_servers;k++){//desbloquear os mutex Dos vcpu 2
    		sem_wait(&SHM->servers[k].mutex_dados);
    		sem_post(&SHM->servers[k].mutex_vcpu2);
    		SHM->servers[k].ocup[1]=0;
    		sem_post(vcpu_livres);
    		sem_post(&SHM->servers[k].mutex_dados);	
    	}
    	while(1){
    		sem_wait(mutex_monitor);
    		sem_wait(mutex_comun);
    		time=SHM->all.max_time;
    		fila_espera=SHM->all.tarefas_espera;
    		sem_post(mutex_comun);
    		percent_fila=fila_espera*100/tam_fila;
    		if(time<max_time && (percent_fila<=80)){
    			break;
    		}
    	}
    	sem_wait(mutex_log);
        sprintf(mensagem_log, "Fim do modo alta performance\n");
        log_message(mensagem_log);
        sem_post(mutex_log);
    	for(k=0;k<n_servers;k++){//bloquear os mutex Dos vcpu 2
    		sem_wait(&SHM->servers[k].mutex_dados);//risco de deathlock
    		sem_wait(&SHM->servers[k].mutex_vcpu2);
    		SHM->servers[k].ocup[1]=1;
    		sem_wait(vcpu_livres);
    		sem_post(&SHM->servers[k].mutex_dados);
    	}
    }
}


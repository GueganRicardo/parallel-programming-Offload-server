// SISTEMAS OPERATIVOS 2021-2022
//Ricardo Guegan 2020211358
//Samuel Machado 2020219391
#include "all_p.h"

void SIG_fecha_mm(int signum){
	sem_wait(mutex_log);
    strcpy(mensagem_log,"Fecho do Maintenance Manager\n");
    log_message(mensagem_log);
    sem_post(mutex_log);
	signal(SIGINT,SIG_IGN);
	exit(0);
}

void mantainance_manager(){
	int k,tempo, tempodeman;
	struct man_confirm em_man;
	struct mensagem enviar;
	struct man_servers servers[n_servers];
	signal(SIGINT,SIG_fecha_mm);
	int sleepy, next_server=0;
	int servers_on=n_servers;
	em_man.id_server=-1;
	for(k=0;k<n_servers;k++){//iniciar os dados dos servers
		servers[k].man=false;//garantir que não colocamos um em manutenção se já estiver em
	}
    while(1){
    sleepy=rand()%5+1;
    enviar.destinatario=next_server+1;
    enviar.tempo_manutencao=rand()%5+1;
    enviar.ativ=true;
    if(servers_on>1 && !servers[next_server].man){
    servers[next_server].tempo_man=enviar.tempo_manutencao;
    msgsnd(id_message_queue,&enviar,sizeof(struct mensagem)-sizeof(long),0);//envia manutencao
    servers_on--;
    next_server++;
    next_server=(next_server%n_servers);
    }
    msgrcv(id_message_queue,&em_man,sizeof(struct mensagem)-sizeof(long),n_servers+1,IPC_NOWAIT);//confirma que está em man
    if(em_man.id_server!=-1){
    	servers[em_man.id_server].man=true;
    	servers[em_man.id_server].inicio_man=em_man.tempo_inicio;
    	em_man.id_server=-1;
    }
    for(k=0;k<n_servers;k++){//percorrer e libertar os que o tempo já passou
    	if(servers[k].man){
    		tempodeman=servers[k].inicio_man+servers[k].tempo_man;
    		tempo=get_time();
    		if(tempodeman<tempo){//libertar
    		enviar.destinatario=k+1;
    		enviar.ativ=false;
    		msgsnd(id_message_queue,&enviar,sizeof(struct mensagem)-sizeof(long),0);
    		servers[k].man=false;
    		servers_on++;
    		}
    	}
    }
    sleep(sleepy);
    }
}



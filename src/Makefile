FLAGS  = -Wall -lpthread -D_REENTRANT
CC     = gcc
PROG   = system_manager
PROGG  = mobilenode
OBJS   = system_manager.o monitor.o edge_server.o task_manager.o maintenance_manager.o
OBJSS  = mobilenode.o

all:	${PROG} ${PROGG}

clean:
	rm ${OBJS} ${PROG} *~

${PROG}:	${OBJS}
	${CC} ${OBJS} ${FLAGS} -o $@

${PROGG}:	${OBJSS}
	${CC} ${OBJSS} ${FLAGS} -o $@

.c.o:
	${CC} ${FLAGS} $< -c

##########################

system_manager.o: all_p.h system_manager.c

system_manager: system_manager.o maintenance_manager.o monitor.o task_manager.o edge_server.o

mobilenode.o: mobilenode.c

mobilenode: mobilenode.o

monitor.o: monitor.c all_p.h

task_manager.o: task_manager.c all_p.h

edge_server.o: edge_server.c all_p.h

maintenance_manager.o: maintenance_manager.c  all_p.h

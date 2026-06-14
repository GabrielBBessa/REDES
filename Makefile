CC = g++
CFLAGS = -Wall -g
COMMON_OBJS = protocolo.o rede.o jogo.o

all: cliente servidor

cliente: cliente.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o cliente cliente.o $(COMMON_OBJS)

servidor: servidor.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o servidor servidor.o $(COMMON_OBJS)

# Regras de compilação individual (.cpp para .o)
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o cliente servidor
	


# Variáveis
CC = g++
CFLAGS = -Wall -g
TARGET = redes_exec

# Comando principal: apenas digite 'make' no terminal
all: $(TARGET)

$(TARGET): main.cpp
	$(CC) $(CFLAGS) main.cpp -o $(TARGET)

# Comando para limpar a pasta: digite 'make clean'
clean:
	rm -f $(TARGET)

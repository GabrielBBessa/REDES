#ifndef _PROTOCOLO_
#define _PROTOCOLO_

#include <iostream>
#include <cstring>         
#include <arpa/inet.h>     
#include <sys/socket.h>         
#include <net/ethernet.h>       
#include <linux/if_packet.h>    
#include <net/if.h>   
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <unistd.h>
#include "jogo.h"


struct __attribute__((packed)) Pacote {
    uint8_t marcador;  
    uint8_t tamanho;  
    uint8_t sequencia;
    uint8_t tipo;      
    uint8_t dados[63];
    uint8_t crc;       
};


int cria_raw_socket(char* nome_interface_rede);
uint8_t calcula_crc(struct Pacote *p);
void inicializa_pacote(struct Pacote *meu_pacote, uint8_t num_sequencia , size_t lidos , uint8_t *vetor_leitura);

#endif

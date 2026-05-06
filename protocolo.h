#ifndef _PROTOCOLO_
#define _PROTOCOLO_

#include <iostream>
#include <cstring>         // Para funcoes como memset e memcpy
#include <arpa/inet.h>     // Para funções de rede como htons e tipo uint8_t

// Estrutura passada pelos professores
// tipo uint8_t garante que terá exatos 8 bits
struct __attribute__((packed)) Pacote {
    uint8_t marcador;  
    uint8_t tamanho;  
    uint8_t sequencia;
    uint8_t tipo;      
    uint8_t dados[63];
    uint8_t crc;       
};

uint8_t calcula_crc(struct Pacote *p);
void inicializa_pacote(struct Pacote *meu_pacote, uint8_t num_sequencia , size_t lidos , uint8_t *vetor_leitura);

#endif
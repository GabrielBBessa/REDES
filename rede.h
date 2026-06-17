#ifndef _REDE_
#define _REDE_

#include <iostream>
#include <net/ethernet.h>       // Definições de protocolos Ethernet
#include <linux/if_packet.h>    // Estruturas para pacotes de nível de enlace (sockaddr_ll)
#include <net/if.h>             // Para a função if_nametoindex
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <unistd.h>
#include "protocolo.h"
#include "math.h"

          

void enviar_arquivo(int socket, const char *nome_do_arquivo, uint8_t tipo_arquivo);
bool receber_pacote(int socket, struct Pacote *target);
void enviar_mensagem(int socket, uint8_t tipo, uint8_t sequencia, uint8_t tamanho, uint8_t *dados);


#endif

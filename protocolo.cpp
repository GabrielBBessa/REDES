#include "protocolo.h"

int cria_raw_socket(char* nome_interface_rede) {
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        std::cerr << "Erro ao criar socket: Verifique se você é root (sudo)!" << std::endl;
        exit(-1);
    }
    int ifindex = if_nametoindex(nome_interface_rede);
    struct sockaddr_ll endereco = {0}; 
    endereco.sll_family = AF_PACKET;       
    endereco.sll_protocol = htons(ETH_P_ALL); 
    endereco.sll_ifindex = ifindex;        

    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        std::cerr << "Erro ao fazer bind no socket: Interface inválida!" << std::endl;
        exit(-1);
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex; 
    mr.mr_type = PACKET_MR_PROMISC; 

    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        std::cerr << "Erro ao ativar modo promíscuo." << std::endl;
        exit(-1);
    }
    return soquete;
}

uint8_t calcula_crc(struct Pacote *p) {
    uint8_t crc = 0x00;
    uint8_t *dados = (uint8_t *)p; // Trata a struct como um array de bytes
    
	
	// Já ignorando o proprio CRC
    int tamanho_total = sizeof(struct Pacote) - 1;

    for (int i = 0; i < tamanho_total; i++) {
        crc ^= dados[i]; // Operação XOR
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) { // Se o bit mais significativo for 1
                crc = (crc << 1) ^ 0x07; // Shift e XOR com o polinômio
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// Preenche o pacote 
void inicializa_pacote(struct Pacote *meu_pacote, uint8_t num_sequencia , size_t lidos , uint8_t *vetor_leitura){

	memset(meu_pacote, 0, sizeof(struct Pacote)); 	// Preenche tudo com zero
	
	meu_pacote->marcador = 0x7E;           			// O 01111110 fixo do protocolo
	meu_pacote->tamanho = (uint8_t)lidos; 			// Salva quantos bytes reais de dados há aqui
	meu_pacote->sequencia = num_sequencia; 			// Uma variável que você incrementa (0, 1, 2...)
	meu_pacote->tipo = (lidos > 0) ? 0x08 : 0x0A;              			


	if (vetor_leitura != NULL && lidos > 0) {
        memcpy(meu_pacote->dados, vetor_leitura, lidos);
    }	
	
	meu_pacote->crc = calcula_crc(meu_pacote);  
}

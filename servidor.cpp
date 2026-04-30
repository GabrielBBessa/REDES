#include <cstring>         // Para funções como memset (equivalente ao string.h)
#include "rede.h"

// Preenche o pacote 
void inicializa_pacote(struct Pacote *meu_pacote, uint8_t num_sequencia , size_t lidos , uint8_t *vetor_leitura){

	memset(meu_pacote, 0, sizeof(struct Pacote)); 	// Preenche tudo com zero
	
	meu_pacote->marcador = 0x7E;           			// O 01111110 fixo do protocolo
	meu_pacote->tamanho = (uint8_t)lidos; 			// Salva quantos bytes reais de dados há aqui
	meu_pacote->sequencia = num_sequencia; 			// Uma variável que você incrementa (0, 1, 2...)
	meu_pacote->tipo = 0x08;               			// Exemplo: código para "Dados de Arquivo"


	// Copia os dados do vetor de leitura para dentro da struct
	for (size_t i = 0; i < lidos; i++) {
    	meu_pacote->dados[i] = vetor_leitura[i];
	}	
	
	meu_pacote->crc = calcula_crc(meu_pacote);
   
}	

// para rodar baixe o g++ , descubra o nome da sua interface com o comando *ip addr*
//e rode com sudo ./executável "nome da interface" 
int main(int argc, char *argv[]) {
    // Verifica se você passou o nome da interface (ex: lp) ao rodar
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }
    
    // Chama a sua função para testar se ela funciona
    int sock = cria_raw_socket(argv[1]);

    if (sock != -1)	std::cout << "Socket criado com sucesso no index do sistema!" << std::endl;
    
    struct Pacote meu_pacote;
    
    // Arquivo de teste
    FILE *arq;
    
    // Vetor para guardar os dados lidos , tendo como limite o tamanho do pacote 
	uint8_t vetor_temporario[63];
    
    arq = fopen("teste" , "rb");
    
    if (arq == NULL) {
    	perror("Erro ao abrir arquivo"); 
    	exit(-1); 
	}
	
	
	uint8_t cont = 0;
	
	// Enquanto não acabar o arquivo
	while (!feof(arq)){ 
	
		// Variável para saber se foram usados todos os 63 bits ou menos
		size_t lidos = fread(vetor_temporario, 1, 63, arq);
		
		if(lidos > 0){
			inicializa_pacote(&meu_pacote,cont,lidos,vetor_temporario);
			
			bool sucesso = false;
            while (!sucesso) {
                // Envia o pacote pelo socket
                send(sock, &meu_pacote, sizeof(meu_pacote), 0);
                
                // Espera o ACK (Aqui você precisaria de um recv com timeout)
                // Se receber ACK com a mesma seq:
                sucesso = true; 
                // Se der erro de CRC no receptor ou timeout, o loop repete o send
                
            }
            cont++;
		}
	}	
	
	// BOA PRÁTICA: Enviar o pacote de FIM (EOF)
    struct Pacote pacote_fim;
    
    // Usamos o contador de sequência atual para manter a ordem
    // lidos = 0 porque não há dados de arquivo aqui
    // vetor_temporario pode ser NULL 
    inicializa_pacote(&pacote_fim, cont, 0, NULL); 
    
    // Mudamos o tipo manualmente para o código de FIM
    pacote_fim.tipo = 0x09; 
    
    // Recalcula o CRC porque mudamos o tipo!
    pacote_fim.crc = calcula_crc(&pacote_fim);

    send(sock, &pacote_fim, sizeof(pacote_fim), 0);
    			
    	    
    fclose(arq);

    return 0;
}
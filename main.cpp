#include "rede.h"

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
	
	struct timeval tv;
	tv.tv_sec = 0;       // Segundos
	tv.tv_usec = 200000; // Microsegundos (0.2 segundos)

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	struct Pacote pacote_ack;
	
	uint8_t cont = 0;
	int bytes;
	
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
                
                bytes = recv(sock,&pacote_ack,sizeof(pacote_ack),0);
                
                if(bytes > 0){
                	if(pacote_ack.tipo == 0x0a && pacote_ack.sequencia == meu_pacote.sequencia) sucesso = true;
                }
            	else std::cout << "Timeout" << std::endl;       
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

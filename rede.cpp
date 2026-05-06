#include "rede.h"

/*
 * Função para criar um socket "cru" (Raw Socket)
 * Recebe o nome da interface (ex: "eth0", "enp0s3")
 */
 
 /* Socket é a estrutura que permite passar informações de um computador para outro , 
a raw socket permite interagir com a placa de rede quase que diretamente , lendo e escrevendo byte a byte na rede */
int cria_raw_socket(char* nome_interface_rede) {
    
    // 1. CRIAÇÃO DO SOCKET
    // AF_PACKET: Indica comunicação na camada de enlace (Layer 2)
    // SOCK_RAW: Indica que o pacote será entregue "cru", sem cabeçalhos IP/TCP automáticos
    // htons(ETH_P_ALL): Grante que o socket irá capturar todos os protocolos que passarem pela interface de rede
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    
    // Verificação de erro: Raw Sockets exigem permissão de administrador (root)
    if (soquete == -1) {
        std::cerr << "Erro ao criar socket: Verifique se você é root (sudo)!" << std::endl;
        exit(-1);
    }

    // Retorna o número correspondente ao nome de interface passado 
    int ifindex = if_nametoindex(nome_interface_rede);

    // 3. CONFIGURAÇÃO DO ENDEREÇO
    // Define o caminho dos dados
    struct sockaddr_ll endereco = {0}; 
    //Os dois primeiros são os mesmos parametros usados em soquete e o terceiro é o do número da interface
    endereco.sll_family = AF_PACKET;       
    endereco.sll_protocol = htons(ETH_P_ALL); 
    endereco.sll_ifindex = ifindex;        

    // 4. VINCULAÇÃO (BIND)
    // Associa o socket criado especificamente à placa de rede escolhida (pra isso a struct sockaddr)
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        std::cerr << "Erro ao fazer bind no socket: Interface inválida!" << std::endl;
        exit(-1);
    }

    // 5. CONFIGURAÇÃO DO MODO PROMÍSCUO
    // Garante que não vai jogar nada no lixo automaticamente(modo promiscuo)
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex; //O número de interface 
    mr.mr_type = PACKET_MR_PROMISC; // Define co tipo de operação como promíscua

    // setsockopt(SET SOCK OPT): Aplica a configuração promíscua no hardware da placa
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        std::cerr << "Erro ao ativar modo promíscuo. Verifique o nome da interface." << std::endl;
        exit(-1);
    }

    // Retorna o identificador do socket 
    return soquete;
}

bool receber_pacote(int socket, struct Pacote *target) {
    // Usei um buffer grande para ler o arquivo
    // Ler direto na struct pode dar segfault se o arquivo enviado for > 67 bytes
    uint8_t buffer[2048];
    ssize_t bytes_lidos;

    while (true) {
        // Ler os dados crus da placa de rede e colocar no buffer
        bytes_lidos = recv(socket, buffer, sizeof(buffer), 0);

        // Verifica se os bytes lidos sao pelo menos do tamanho da struct
        // Se nao for, eh lixo
        if (bytes_lidos >= (ssize_t)sizeof(struct Pacote)) {

            // Verifica se o marcador eh o tipo certo (0x7E)
            if (buffer[0] == 0x7E) {
                std::cout << "Pacote valido recebido" << std::endl;

                // Copia os bytes do buffer na struct
                struct Pacote pacote_recebido;
                memcpy(&pacote_recebido, buffer, sizeof(struct Pacote));

                // Recalcula o CRC e depois compara com o CRC salvo
                if (pacote_recebido.crc == calcula_crc(&pacote_recebido)) {
                    std::cout << "Pacote integro recebido" << std::endl;

                    // Salva o pacote recebido em target e retorna
                    *target = pacote_recebido;
                    return true;
                }
                else {
                    std::cout << "Pacote corrompido" << std::endl;
                }
            }
        }
        else {
            // Pacote invalido recebido
            continue;
        }
    }
}

void enviar_arquivo(int socket, const char *nome_do_arquivo) {
    struct Pacote meu_pacote;
    
    // Arquivo de teste
    FILE *arq;
    
    // Vetor para guardar os dados lidos , tendo como limite o tamanho do pacote 
	uint8_t vetor_temporario[63];
    
    arq = fopen(nome_do_arquivo, "rb");
    
    if (arq == NULL) {
    	perror("Erro ao abrir arquivo"); 
    	exit(-1); 
	}
	
	// variaveis para fazer o timeout:
    struct timeval tv;
	tv.tv_sec = 0;       // Segundos
	tv.tv_usec = 200000; // Microsegundos (0.2 segundos)	
	struct Pacote pacote_ack;	
	size_t bytes;
	
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


	uint8_t cont = 0;
	size_t lidos;
	
	// Enquanto não acabar o arquivo
	// 1. MUDANÇA: Use o retorno do fread diretamente para evitar o erro do feof.
	while ((lidos = fread(vetor_temporario, 1, 63, arq)) > 0) {
		
		// Isso joga fora qualquer "eco" de pacotes anteriores que ainda esteja na placa.
		while(recv(socket, &pacote_ack, sizeof(pacote_ack), MSG_DONTWAIT) > 0);

		inicializa_pacote(&meu_pacote, cont, lidos, vetor_temporario);
		
		bool sucesso = false;
		while (!sucesso) {
		    send(socket, &meu_pacote, sizeof(meu_pacote), 0);
		    
		    while(true) {
		        bytes = recv(socket, &pacote_ack, sizeof(pacote_ack), 0);
		        
		        if (bytes < 0) {
		        	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		        		std::cout << "Timeout Real - Reenviando SEQ " << (int)cont << std::endl;
		    		}
		            break; 
		        }

		        if (pacote_ack.tipo == 0x0a && pacote_ack.sequencia == meu_pacote.sequencia) {
		            sucesso = true;
		            break;
		        }
		    }
		}
		cont++;
		// 4. MUDANÇA: Zere o buffer para a próxima leitura do arquivo.
		memset(vetor_temporario, 0, 63);
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

    send(socket, &pacote_fim, sizeof(pacote_fim), 0);
    			
    	    
    fclose(arq);
}

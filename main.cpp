#include <iostream>        // Biblioteca padrão de entrada/saída do C++
#include <cstring>         // Para funções como memset (equivalente ao string.h)
#include <arpa/inet.h>     // Para funções de rede como htons
#include <net/ethernet.h>  // Definições de protocolos Ethernet
#include <linux/if_packet.h> // Estruturas para pacotes de nível de enlace (sockaddr_ll)
#include <net/if.h>        // Para a função if_nametoindex


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

uint8_t calcula_crc(struct Pacote *p) {
    uint8_t crc = 0x00;
    uint8_t *dados = (uint8_t *)p; // Trata a struct como um array de bytes
    

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
	meu_pacote->tipo = 0x08;               			// Exemplo: código para "Dados de Arquivo"


	// Copia os dados do vetor de leitura para dentro da struct
	for (size_t i = 0; i < lidos; i++) {
    	meu_pacote->dados[i] = vetor_leitura[i];
	}	
	
	meu_pacote->crc = calcula_crc(meu_pacote);
   
}
    
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
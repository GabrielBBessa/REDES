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
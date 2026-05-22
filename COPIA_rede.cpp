#include "rede.h"

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

bool receber_pacote(int socket, struct Pacote *target) {
    uint8_t buffer[2048];
    ssize_t bytes_lidos;

    while (true) {
        bytes_lidos = recv(socket, buffer, sizeof(buffer), 0);

        if (bytes_lidos >= (ssize_t)sizeof(struct Pacote)) {
            
            if (buffer[0] == 0x7E) { // Verifica o Marcador
                
                // Mapeamento do Buffer:
                // buffer[0] = marcador
                // buffer[1] = tamanho
                // buffer[2] = sequencia
                // buffer[3] = tipo
                // buffer[4] até buffer[66] = dados (63 bytes)
                // buffer[67] = crc

                // BYTE UNSTUFFING DIRETO NO BUFFER BRUTO
                // Lemos qual é o tamanho que o emissor diz que os dados têm agora
                uint8_t tamanho_stuffed = buffer[1];

                // varremos apenas a região onde ficam os dados (indice 4)
                for (int i = 4; i < 4 + tamanho_stuffed; i++) {
                    
                    // Se achar o byte problemático e o próximo for o 0xFF da injeção
                    if ((buffer[i] == 0x81 || buffer[i] == 0x88) && buffer[i + 1] == 0xFF) {
                        
                        // Puxa APENAS a região de dados para a esquerda para esmagar o 0xFF
                        // Não podemos puxar o buffer inteiro, senão o CRC (índice 67) sai do lugar
                        for (int j = i + 1; j < 66; j++) {
                            buffer[j] = buffer[j + 1];
                        }
                        buffer[66] = 0; // Zera a sujeira no final dos dados

                        // Reduzimos o campo 'tamanho' (índice 1) do buffer
                        buffer[1]--;
                        tamanho_stuffed--; // Atualiza a variável do loop para não ler lixo
                    }
                }

                // COPIA PARA A STRUCT E VALIDAÇÃO DE CRC
                // Agora o buffer está perfeitamente limpo. Copiamos para a struct.
                struct Pacote pacote_recebido;
                memcpy(&pacote_recebido, buffer, sizeof(struct Pacote));

                uint8_t crc_recebido = pacote_recebido.crc;
                pacote_recebido.crc = 0; // Zera para o cálculo

                // O CRC vai bater com o emissor porque o emissor calculou 
                // o CRC *ANTES* de injetar o 0xFF
                if (crc_recebido == calcula_crc(&pacote_recebido)) {
                    *target = pacote_recebido;
                    return true;
                } else {
                    std::cout << "Pacote corrompido (Erro de CRC)" << std::endl;
                }
            }
        }
    }
}

void enviar_arquivo(int socket, const char *nome_do_arquivo) {
    struct Pacote meu_pacote;
    
    int arq = open(nome_do_arquivo, O_RDONLY);
    if (arq == -1) {
        perror("Erro ao abrir arquivo"); 
        return; 
    }
    
    uint8_t vetor_temporario[63];
    
    struct timeval tv;
    tv.tv_sec = 0;       
    tv.tv_usec = 200000; // 0.2s
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct Pacote pacote_ack;   
    ssize_t bytes;  
    uint8_t cont = 0;
    ssize_t lidos;
    
    // Lemos no max 30 bytes
    // Assim, se todos precisarem de 0xFF, o pacote vai para 60 bytes, nunca ultrapassando o limite físico de 63 bytes da struct
    while ((lidos = read(arq, vetor_temporario, 30)) > 0) {
        
        // A inicializa_pacote monta os dados e calcula o crc sobre os dados limpos
        inicializa_pacote(&meu_pacote, cont, lidos, vetor_temporario);
            
        // BYTE STUFFING fora do loop de envio
        for(int i = 0; i < meu_pacote.tamanho; i++){
            if((meu_pacote.dados[i] == 0x81) || (meu_pacote.dados[i] == 0x88)){ 
                
                // Desloca os bytes
                // 62 é o último índice do vetor de dados[63]
                for(int j = 62; j > i + 1; j--){
                    meu_pacote.dados[j] = meu_pacote.dados[j - 1];
                }
                
                // Injeta o 0xFF depois do byte problematico
                meu_pacote.dados[i + 1] = 0xFF;
                meu_pacote.tamanho++; // O tamanho do pacote aumentou
                
                // Pula o 0xFF que acabamos de adicionar
                i++;
            }                                   
        }
        
        bool sucesso = false;
        
        while (!sucesso) {
            
            // Limpa o lixo da placa de rede antes de enviar
            uint8_t buffer_lixo[2048];
            while(recv(socket, buffer_lixo, sizeof(buffer_lixo), MSG_DONTWAIT) > 0);

            send(socket, &meu_pacote, sizeof(meu_pacote), 0);
                    
            // Espera pelo ACK
            while(true) {
                uint8_t buffer_ack[2048];
                bytes = recv(socket, buffer_ack, sizeof(buffer_ack), 0);
                    
                if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::cout << "Timeout Real - Reenviando SEQ " << (int)cont << std::endl;
                    }
                    break; 
                }

                if (bytes >= (ssize_t)sizeof(struct Pacote) && buffer_ack[0] == 0x7E) {
                    struct Pacote pacote_ack;
                    memcpy(&pacote_ack, buffer_ack, sizeof(struct Pacote));
                    
                    if (pacote_ack.tipo == 0x0A && pacote_ack.sequencia == meu_pacote.sequencia) {
                        if (pacote_ack.crc == calcula_crc(&pacote_ack)) {
                            sucesso = true;
                            break;
                        }
                    }
                }
            }
        }
        cont++;
        memset(vetor_temporario, 0, 63);
    }
    
    // PACOTE DE FIM DE ARQUIVO (EOF)
    struct Pacote pacote_fim;
    inicializa_pacote(&pacote_fim, cont, 0, NULL); 
    pacote_fim.tipo = 0x09; 
    pacote_fim.crc = calcula_crc(&pacote_fim);
    send(socket, &pacote_fim, sizeof(pacote_fim), 0);
                
    close(arq);
}
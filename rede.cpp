#include "rede.h"

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
        
        while(recv(socket, &pacote_ack, sizeof(pacote_ack), MSG_DONTWAIT) > 0);
        
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
                	if (pacote_recebido.tipo == 0x00 || pacote_recebido.tipo == 0x01) {	//Tirar esse código
                        continue; 
                    }
                    
                    *target = pacote_recebido;
                    
                    // 2. A MÁGICA DO PROTOCOLO: OBRIGATÓRIO ENVIAR ACK!
                    struct Pacote pacote_ack;
                    inicializa_pacote(&pacote_ack, pacote_recebido.sequencia, 0, NULL);
                    pacote_ack.tipo = 0x00; // 0x00 é o código de ACK (Confirmação)
                    pacote_ack.crc = calcula_crc(&pacote_ack);
                    
                    // Atira a confirmação de volta pelo mesmo socket
                    send(socket, &pacote_ack, sizeof(pacote_ack), 0);

                    return true;
                } else {
                    std::cout << "Pacote corrompido (Erro de CRC)" << std::endl;
                    // Se o professor exigir no futuro, você envia um pacote NACK (0x01) aqui!
                }
            }
        }
    }
}

void enviar_mensagem(int socket, uint8_t tipo, uint8_t sequencia, uint8_t tamanho, uint8_t *dados) {
    struct Pacote meu_pacote;
    
    // Inicializa o pacote com os dados crus da memória (ex: vetor do mapa)
    inicializa_pacote(&meu_pacote, sequencia, tamanho, dados);
    
    // Sobrescreve o tipo com o que foi passado no parâmetro (ex: 0x02 para Visualiza)
    meu_pacote.tipo = tipo; 

    // APLICA O BYTE STUFFING
    // Varre o payload procurando os bytes proibidos (0x81 e 0x88)
    for(int i = 0; i < meu_pacote.tamanho; i++) {
        if((meu_pacote.dados[i] == 0x81) || (meu_pacote.dados[i] == 0x88)) { 
            
            // Desloca os bytes para a direita para abrir espaço
            for(int j = 62; j > i + 1; j--) {
                meu_pacote.dados[j] = meu_pacote.dados[j - 1];
            }
            
            // Injeta o 0xFF logo após o byte problemático
            meu_pacote.dados[i + 1] = 0xFF;
            meu_pacote.tamanho++; // O pacote ficou um byte mais gordo
            
            // Pula o 0xFF que acabamos de inserir para não analisá-lo
            i++;
        }                                   
    }

    // Recalcula o CRC (Crucial: deve ser feito DEPOIS do Byte Stuffing)
    meu_pacote.crc = calcula_crc(&meu_pacote);

    // Configura o Timeout do socket para 0.2 segundos (Regra do trabalho)
    struct timeval tv;
    tv.tv_sec = 0;       
    tv.tv_usec = 200000; 
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    bool sucesso = false;
    struct Pacote pacote_ack;

    while (!sucesso) {
        
        // Limpa qualquer lixo residual da placa de rede antes de enviar
        uint8_t buffer_lixo[2048];
        while(recv(socket, buffer_lixo, sizeof(buffer_lixo), MSG_DONTWAIT) > 0);

        // Dispara o pacote para a rede
        send(socket, &meu_pacote, sizeof(meu_pacote), 0);

        // Fica travado aguardando a resposta do Cliente
        while(true) {
            uint8_t buffer_ack[2048];
            ssize_t bytes = recv(socket, buffer_ack, sizeof(buffer_ack), 0);
                
            // Tratamento de Timeout (Não recebeu nada em 0.2s)
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cout << "[Timeout] Reenviando pacote avulso SEQ " << (int)sequencia << std::endl;
                }
                break; // Quebra este laço para o loop principal reenviar o pacote
            }

            // Se recebeu um pacote válido (tamanho correto e marcador 0x7E)
            if (bytes >= (ssize_t)sizeof(struct Pacote) && buffer_ack[0] == 0x7E) {
                
                // Copia o buffer bruto para a struct para podermos ler os campos
                memcpy(&pacote_ack, buffer_ack, sizeof(struct Pacote));
                
                // Verifica se é um ACK (Tipo 0x00) e se a sequência bate com o que enviamos
                if (pacote_ack.tipo == 0x00 && pacote_ack.sequencia == sequencia) { 
                    
                    // Valida o CRC da resposta para garantir que o ACK não corrompeu
                    if (pacote_ack.crc == calcula_crc(&pacote_ack)) {
                        sucesso = true; // Pacote entregue e confirmado!
                        break;
                    }
                }
            }
        }
    }
}



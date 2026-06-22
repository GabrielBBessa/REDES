#include "rede.h"

void enviar_arquivo(int socket, const char *nome_do_arquivo, uint8_t tipo_arquivo) {
    struct Pacote meu_pacote;
    
    // Tenta abrir o arquivo no HD
    int arq = open(nome_do_arquivo, O_RDONLY);
    if (arq == -1) {
        perror("Erro ao abrir arquivo"); 
        return; 
    }
    
    uint8_t vetor_temporario[63];
    
    // Configura o Timeout de 0.2s
    struct timeval tv;
    tv.tv_sec = 0;       
    tv.tv_usec = 200000; 
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct Pacote pacote_ack;   
    ssize_t bytes;  
    uint8_t cont = 0;
    ssize_t lidos;
    
    // Lemos no max 30 bytes do arquivo por vez
    while ((lidos = read(arq, vetor_temporario, 30)) > 0) {
        
        // Limpa lixo da placa de rede
        while(recv(socket, &pacote_ack, sizeof(pacote_ack), MSG_DONTWAIT) > 0);
        
        // Monta os dados base (tamanho, sequencia, etc)
        inicializa_pacote(&meu_pacote, cont, lidos, vetor_temporario);
        
        // --- AS DUAS LINHAS MÁGICAS ---
        // Força o tipo correto (0x05, 0x06 ou 0x07) e recalcula o CRC com o novo tipo
        meu_pacote.tipo = tipo_arquivo; 
        meu_pacote.crc = calcula_crc(&meu_pacote); 
            
        // BYTE STUFFING
        for(int i = 0; i < meu_pacote.tamanho; i++){
            if((meu_pacote.dados[i] == 0x81) || (meu_pacote.dados[i] == 0x88)){ 
                
                for(int j = 62; j > i + 1; j--){
                    meu_pacote.dados[j] = meu_pacote.dados[j - 1];
                }
                
                meu_pacote.dados[i + 1] = 0xFF;
                meu_pacote.tamanho++; 
                i++;
            }                                   
        }
        
        bool sucesso = false;
        
        // PARA-E-ESPERA
        while (!sucesso) {
            
            uint8_t buffer_lixo[2048];
            while(recv(socket, buffer_lixo, sizeof(buffer_lixo), MSG_DONTWAIT) > 0);

            send(socket, &meu_pacote, sizeof(meu_pacote), 0);
                    
            while(true) {
                uint8_t buffer_ack[2048];
                bytes = recv(socket, buffer_ack, sizeof(buffer_ack), 0);
                    
                if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::cout << "[Timeout] Reenviando pacote de arquivo SEQ " << (int)cont << std::endl;
                    }
                    break; 
                }

                if (bytes >= (ssize_t)sizeof(struct Pacote) && buffer_ack[0] == 0x7E) {
                    memcpy(&pacote_ack, buffer_ack, sizeof(struct Pacote));
                    
                    // CORREÇÃO DO BUG DO ACK: Agora espera 0x00 (ACK) em vez de 0x0A
                    if (pacote_ack.tipo == 0x00 && pacote_ack.sequencia == meu_pacote.sequencia) {
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
    pacote_fim.tipo = 0x10;
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
            
            if (buffer[0] == 0x7E) {
                
                // BYTE UNSTUFFING
                uint8_t tamanho_stuffed = buffer[1];
                uint8_t dados_limpos[63] = {0};
                int indice_limpo = 0;
                int bytes_removidos = 0;

                for (int i = 4; i < 4 + tamanho_stuffed; i++) {
                    dados_limpos[indice_limpo++] = buffer[i];

                    if (buffer[i] == 0x81 || buffer[i] == 0x88) {
                        if (i + 1 < 4 + tamanho_stuffed && buffer[i + 1] == 0xFF) {
                            bytes_removidos++;
                            i++;
                        }
                    }
                }

                buffer[1] = buffer[1] - bytes_removidos; // Corrige o tamanho total do payload
                memcpy(&buffer[4], dados_limpos, buffer[1]);
                memset(&buffer[4 + buffer[1]], 0, 63 - buffer[1]); // Zera o espaço restante

                // COPIA PARA A STRUCT E VALIDAÇÃO DE CRC
                struct Pacote pacote_recebido;
                memcpy(&pacote_recebido, buffer, sizeof(struct Pacote));

                uint8_t crc_recebido = pacote_recebido.crc;
                pacote_recebido.crc = 0; // Zera para o cálculo

                if (crc_recebido == calcula_crc(&pacote_recebido)) {
                    if (pacote_recebido.tipo == 0x00 || pacote_recebido.tipo == 0x01) {
                        continue; 
                    }
                    
                    *target = pacote_recebido;
                    
                    // Enviar ACK
                    struct Pacote pacote_ack;
                    inicializa_pacote(&pacote_ack, pacote_recebido.sequencia, 0, NULL);
                    pacote_ack.tipo = 0x00; // 0x00 é o código de ACK (Confirmação)
                    pacote_ack.crc = calcula_crc(&pacote_ack);
                    
                    // Atira a confirmação de volta pelo mesmo socket
                    send(socket, &pacote_ack, sizeof(pacote_ack), 0);

                    return true;
                } else {
                    std::cout << "Pacote corrompido (Erro de CRC)" << std::endl;
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
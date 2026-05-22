#include "rede.h"

int main(int argc, char *argv[]) {
    // Verifica se o usuário passou o nome da interface ao rodar
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }
    
    // Inicia raw socket
    int socket = cria_raw_socket(argv[1]);
    if (socket != -1) {
        std::cout << "Socket criado com sucesso no index do sistema!" << std::endl;
    }

    struct Pacote pacote_recebido;
    std::cout << "Servidor iniciado. Aguardando pacote" << std::endl;
    
    // Prepara o arquivo para receber os dados
    int arq_recebido = open("arquivo_final.mp4", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (arq_recebido < 0) {
        perror("Erro ao criar arquivo de destino");
        return -1;
    }
    

    // Loop do servidor
    while (true) {
        // Execucao fica travada enquanto servidor espera receber algo valido
        if (receber_pacote(socket, &pacote_recebido)) {

            // Verifica o tipo da mensagem (movimento, etc)
            if (pacote_recebido.tipo == 0x08) {

                write(arq_recebido, pacote_recebido.dados, pacote_recebido.tamanho);
            	std::cout << "Pacote recebido com sucesso" << std::endl;
            
                // Resposta (ACK), nao sei se sempre vai ser isso, foi para teste
                struct Pacote pacote_ack;
                inicializa_pacote(&pacote_ack, pacote_recebido.sequencia, 0, NULL);
                send(socket, &pacote_ack, sizeof(pacote_ack), 0);
                std::cout << "Pacote ACK enviado" << std::endl;
            }
            else if (pacote_recebido.tipo == 0x09) {
                std::cout << "Fim do arquivo recebido com sucesso!" << std::endl;
                break; // Sai do loop para fechar o arquivo
            }
        }
    }
	close(arq_recebido);
    return 0;
}

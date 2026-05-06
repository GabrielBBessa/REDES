#include "rede.h"

// para rodar baixe o g++ , descubra o nome da sua interface com o comando *ip addr*
//e rode com sudo ./executável "nome da interface" 
int main(int argc, char *argv[]) {
    // Verifica se você passou o nome da interface (ex: lp) ao rodar
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

    // Loop do servidor
    while (true) {
        // Execucao fica travada enquanto servidor espera receber algo valido
        if (receber_pacote(socket, &pacote_recebido)) {

            // Verifica o tipo da mensagem (movimento, etc)
            if (pacote_recebido.tipo == 10) {
                std::cout << "Pacote recebido com sucesso" << std::endl;

                // Resposta (ACK), nao sei se sempre vai ser isso, foi para teste
                struct Pacote pacote_ack;
                inicializa_pacote(&pacote_ack, pacote_recebido.sequencia, 0, NULL);
                pacote_ack.tipo = 1;

                // Precisamos recalcular o CRC pois mudei o tipo
                pacote_ack.crc = calcula_crc(&pacote_ack);

                send(socket, &pacote_ack, sizeof(pacote_ack), 0);
                std::cout << "Pacote ACK enviado" << std::endl;
            }
        }
    }

    return 0;
}
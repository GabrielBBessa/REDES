#include "rede.h"

int main(int argc, char *argv[]) {
    // Verifica se o usuário passou a interface
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }

    // Cria o Raw Socket usando a sua função de rede
    int sock = cria_raw_socket(argv[1]);

    if (sock != -1) {
        std::cout << "Conectado à interface: " << argv[1] << std::endl;
        std::cout << "Iniciando envio do arquivo..." << std::endl;

		// Envia o arquivo , precisa ser do mesmo tipo que a parte que vai receber
        enviar_arquivo(sock, "longo.mp4");

        std::cout << "Processo de envio finalizado." << std::endl;
    }

    return 0;
}

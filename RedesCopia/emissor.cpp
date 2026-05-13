#include "rede.h"

int main(int argc, char *argv[]) {
    // 1. Verifica se o usuário passou a interface (ex: enp0s3)
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }

    // 2. Cria o Raw Socket usando a sua função de rede
    // Lembre-se: precisa de sudo para funcionar![cite: 4]
    int sock = cria_raw_socket(argv[1]);

    if (sock != -1) {
        std::cout << "Conectado à interface: " << argv[1] << std::endl;
        std::cout << "Iniciando envio do arquivo..." << std::endl;

        // 3. Chama a função que você já criou.
        // Você pode mudar "teste.txt" para o arquivo que quiser enviar.[cite: 4]
        enviar_arquivo(sock, "longo.mp4");

        std::cout << "Processo de envio finalizado." << std::endl;
    }

    return 0;
}

#include "rede.h"

// Função auxiliar para desenhar o mapa quadradinho na tela
void imprimir_mapa(uint8_t *dados, int tamanho) {

	int lado = sqrt(tamanho);
	int indice = 0;
	
	for (int i = 0; i < lado; i++) {
		for (int j = 0; j < lado; j++) {
			std::cout << dados[indice] << " ";
			indice++;
		}
		std::cout << std::endl; // Quebra a linha ao fim de cada coluna
	}
}

int main(int argc, char *argv[]) {
    // Verifica se o usuário passou o nome da interface ao rodar
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }
    
    // Inicia raw socket
    int socket = cria_raw_socket(argv[1]);
    if (socket == -1) return -1;
    
    uint8_t seq = 0;
    uint8_t tipo_movimento;
    char jogada;
    bool resposta_recebida;
    
	enviar_mensagem(socket, 0x03, seq, 0, NULL);
	seq++;
	
	struct Pacote pacote_inicial;
	while(true) {
		if (receber_pacote(socket, &pacote_inicial)) {
			if (pacote_inicial.tipo == 0x02) { // 0x02 é o Visualiza
				imprimir_mapa(pacote_inicial.dados, pacote_inicial.tamanho);
				break; // Recebeu a visão inicial! Pode começar o jogo.
			}
		}
	}
	
	// LOOP PRINCIPAL DO JOGADOR
	while (true) {  
		std::cin >> jogada; // Fica esperando o usuário digitar e dar Enter

		tipo_movimento = 0;

		// Traduz a letra digitada para o código hexadecimal 
		if (jogada == 'w' || jogada == 'W') tipo_movimento = 0x0C;
		else if (jogada == 's' || jogada == 'S') tipo_movimento = 0x0D;
		else if (jogada == 'a' || jogada == 'A') tipo_movimento = 0x0B;
		else if (jogada == 'd' || jogada == 'D') tipo_movimento = 0x0A;
		else {
			std::cout << "Comando inválido!" << std::endl;
			continue; // Pula o resto e pede a tecla de novo
		}

		// Envia o movimento para o Servidor
		enviar_mensagem(socket, tipo_movimento, seq, 0, NULL);
		seq++; 

		// Fica travado esperando a consequência do seu movimento
		struct Pacote pacote_recebido;
		resposta_recebida = false;
		
		// Guarda o nome do arquivo que está sendo montado
		std::string nome_coringa = ""; 
		
		// Controla se apaga o arquivo velho ou cola no final
		bool primeiro_pacote = true;   
		
		int flags;

		while (!resposta_recebida) {
			if (receber_pacote(socket, &pacote_recebido)) {
                
				// Se for o pacote do Mapa (Tipo 0x02)
				if (pacote_recebido.tipo == 0x02) {
					std::cout << "\nVisão do PacMan:" << std::endl;
					imprimir_mapa(pacote_recebido.dados, pacote_recebido.tamanho);
					resposta_recebida = true; // Quebra o while interno e volta a pedir tecla
				}
				// Recebeu pacote Tipo 5 (txt), 6 (jpg) ou 7 (mp4)
				else if (pacote_recebido.tipo >= 0x05 && pacote_recebido.tipo <= 0x07) {
				
					nome_coringa = "recompensa";
					
					if (pacote_recebido.tipo == 0x05)	nome_coringa += ".txt";

					if (pacote_recebido.tipo == 0x06)	nome_coringa += ".jpg";
					
						
					if (pacote_recebido.tipo == 0x07)	nome_coringa += ".mp4";
					
				}
				
				// Se for o 1º pacote, usa O_TRUNC para zerar o arquivo velho. Senão, usa O_APPEND.
				flags = O_WRONLY | O_CREAT | (primeiro_pacote ? O_TRUNC : O_APPEND);
				primeiro_pacote = false; 

				int arq = open(nome_coringa.c_str(), flags, 0666);
				if (arq != -1) {
					write(arq, pacote_recebido.dados, pacote_recebido.tamanho);
					close(arq);
				}
			}
			// Recebeu o Fim da Transmissão (Tipo 0x10)
			else if (pacote_recebido.tipo == 0x10) {
				std::cout << "\nMídia recebida com sucesso! Abrindo " << nome_coringa << "..." << std::endl;
					
				// Dispara o comando para o Ubuntu abrir o arquivo
				std::string comando = "xdg-open " + nome_coringa + " > /dev/null 2>&1 &";
				system(comando.c_str());
					
				// Se for texto, garante que imprima no terminal também
				if (nome_coringa == "recompensa.txt") { 
					std::cout << "Conteúdo do texto:\n";
					system(("cat " + nome_coringa).c_str());
				}
			}
		}
	}
    return 0;
}

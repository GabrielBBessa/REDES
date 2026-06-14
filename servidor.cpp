#include "rede.h"

int main(int argc, char *argv[]) {
	// Verifica se o usuário passou a interface
	if (argc < 2) {
		std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
		return 1;
	}
	
	char mapa[40][40];
	
	if (carregar_mapa(mapa , "mapa_do_jogo.csv")){ 	
		sortear_entidades(mapa);
		std::cout << "Mapa carregado com sucesso" << std::endl;
	}
	
	else {
		std::cout << "Aviso: Arquivo labirinto.csv não encontrado!" << std::endl;
		std::cout << "Carregando o labirinto padrão da UFPR..." << std::endl;
		
		if (carregar_mapa(mapa , "mapa_ufpr.csv")) {
			sortear_entidades(mapa);
			std::cout << "Mapa padrão carregado com sucesso" << std::endl;
		} 
		else {
			std::cerr << "ERRO FATAL: Nem o mapa padrão foi encontrado na pasta!" << std::endl;
			return -1;
		}
	}

    // Cria o Raw Socket usando a sua função de rede
    int sock = cria_raw_socket(argv[1]);
    if (sock == -1) return -1;
    
	struct coordenada pos_pacman = encontrar_pacman(mapa);
	
	int rodadas = 0; 
	uint8_t seq = 0;
	int raio_atual = 1;
	char visao[50];
	
	char item_pisado;
	int tamanho_visao;

	while (true) {
		struct Pacote pacote_recebido;
		
		// Fica travado aqui até o cliente mandar uma seta
        if (receber_pacote(sock, &pacote_recebido)) {
        
        	// Caso inicial
			if (pacote_recebido.tipo == 0x03) {
				// Reseta o estado do jogo caso o cliente tenha sido reiniciado
				rodadas = 0;
				raio_atual = 1;
				pos_pacman = encontrar_pacman(mapa);                
				tamanho_visao = gerar_visao(mapa, pos_pacman, raio_atual, visao);
				enviar_mensagem(sock, 0x02, seq, tamanho_visao, (uint8_t*)visao);
				seq++;
			}
        
			// Verifica se é uma das 4 setas de Movimento
			if (pacote_recebido.tipo >= 0x0A && pacote_recebido.tipo <= 0x0D) {
			
				// Move o personagem na matriz
				item_pisado = mover_pacman(mapa, &pos_pacman, pacote_recebido.tipo);
			
				// Verificações de itens
				if (item_pisado >= '1' && item_pisado <= '6') {
					std::cout << "ENVIAR ARQUIVO " << std::endl;
					if (item_pisado == 1)	enviar_arquivo(sock, "1.txt");
					if (item_pisado == 2)	enviar_arquivo(sock, "2.txt");
					if (item_pisado == 3)	enviar_arquivo(sock, "3.jpg");
					if (item_pisado == 4)	enviar_arquivo(sock, "4.jpg");
					if (item_pisado == 5)	enviar_arquivo(sock, "5.mp4");
					if (item_pisado == 6)	enviar_arquivo(sock, "6.mp4");
				}
				else if (item_pisado == 'R' || item_pisado == 'B' || item_pisado == 'G' || item_pisado == 'Y') {
					if (item_pisado == 'R')	enviar_arquivo(sock, "vermelho.txt");
					if (item_pisado == 'B')	enviar_arquivo(sock, "azul.txt");
					if (item_pisado == 'G')	enviar_arquivo(sock, "verde.jpg");
					if (item_pisado == 'Y')	enviar_arquivo(sock, "amarelo.jpg");
				}
			
				// Atualiza o estado da visão
				rodadas++; 
				raio_atual = 1 + (rodadas / 5);
				// Não deixa passar do limite máximo de crescimento
				if (raio_atual > 3) raio_atual = 3;

				// Recorta o mapa
				tamanho_visao = gerar_visao(mapa, pos_pacman, raio_atual, visao);
                    
				// Envia o mapa de volta para o cliente
				enviar_mensagem(sock, 0x02, seq, tamanho_visao, (uint8_t*)visao);
				seq++;
			}
		}
	}
	
    return 0;
}




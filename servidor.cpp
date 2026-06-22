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

    // Cria o Raw Socket
    int sock = cria_raw_socket(argv[1]);
    if (sock == -1) return -1;
    
    // Salva a coordenada de cada entidade
	struct coordenada pos_pacman = encontrar_entidade(mapa,'P');
	struct coordenada pos_vermelho= encontrar_entidade(mapa, 'R');
	struct coordenada pos_azul = encontrar_entidade(mapa, 'B');
	struct coordenada pos_verde = encontrar_entidade(mapa, 'G');
	struct coordenada pos_amarelo = encontrar_entidade(mapa, 'Y');
	
	// Variáveis de memória do Fantasma vermelho
	int dir_vermelho = 0;     
	char chao_vermelho = '0';
	bool vermelho_vivo = true;
	
	// Variáveis de memória do Fantasma azul
	int dir_azul = 0;     
	char chao_azul = '0';
	bool azul_vivo = true;
	
	// Variáveis de memória do Fantasma verde
	int lado_verde = 1;
	int dir_verde = 0;     
	char chao_verde = '0';
	bool verde_vivo = true;
	
	// Variáveis de memória do Fantasma amarelo     
	char chao_amarelo = '0';
	bool amarelo_vivo = true;
	
	// Variáveis para controle da lanterna
	int rodadas = 0; 
	uint8_t seq = 0;
	int raio_atual = 1;  
	char visao[2000];
	
	// Variáveis para a lógica interna do jogo 
	char item_pisado;
	int tamanho_visao;
	int fim_jogo = 0;

	while (true) {
		struct Pacote pacote_recebido;
		
        if (receber_pacote(sock, &pacote_recebido)) {
        
        	// Caso inicial
			if (pacote_recebido.tipo == 0x03) {
				rodadas = 0;
				raio_atual = 1;
				dir_vermelho = 0;
				chao_vermelho = '0';
				vermelho_vivo = true;
				dir_azul = 0;
				chao_azul = '0';
				azul_vivo = true;
				lado_verde = 0;
				dir_verde = 0;
				chao_verde = '0';
				verde_vivo = true;
				chao_amarelo = '0';
				amarelo_vivo = true;
				pos_pacman = encontrar_entidade(mapa,'P'); 
				pos_vermelho = encontrar_entidade(mapa, 'R'); 
				pos_azul = encontrar_entidade(mapa, 'B');    
				pos_verde = encontrar_entidade(mapa, 'G');       
				pos_amarelo = encontrar_entidade(mapa, 'Y');  
                
				tamanho_visao = gerar_visao(mapa, pos_pacman, raio_atual, visao);
                
                // Lógica do envio do mapa 
				int enviados = 0;
				while (enviados < tamanho_visao) {
					int lote = (tamanho_visao - enviados > 30) ? 30 : (tamanho_visao - enviados);
					enviar_mensagem(sock, 0x02, seq, lote, (uint8_t*)&visao[enviados]);
					seq++;
					enviados += lote;
				}
				enviar_mensagem(sock, 0x04, seq, 0, NULL);
				seq++;
			}
					      
			// Verifica se é uma das 4 setas de Movimento
			if (pacote_recebido.tipo >= 0x0A && pacote_recebido.tipo <= 0x0D) {
			
				// Move o personagem na matriz
				item_pisado = mover_pacman(mapa, &pos_pacman, pacote_recebido.tipo);
			
				// Verificações dos itens				
				if (item_pisado == '1'){ 
					enviar_arquivo(sock, "1.txt",0x05); 
					fim_jogo++; 
				}
				if (item_pisado == '2'){ 
					enviar_arquivo(sock, "2.txt",0x05); 
					fim_jogo++; 
				}
				if (item_pisado == '3'){ 
					enviar_arquivo(sock, "3.jpg",0x06); 
					fim_jogo++; 
				}
				if (item_pisado == '4'){ 
					enviar_arquivo(sock, "4.jpg",0x06); 
					fim_jogo++; 
				}
				if (item_pisado == '5'){ 
					enviar_arquivo(sock, "5.mp4",0x07); 
					fim_jogo++; 
				}
				if (item_pisado == '6'){ 
					enviar_arquivo(sock, "6.mp4",0x07); 
					fim_jogo++; 
				}
				
				// Pacman pega os fantasmas				
				if (item_pisado == 'R'){ 
					enviar_arquivo(sock, "vermelho.jpg",0x06); 
					vermelho_vivo = false; 
				}
				if (item_pisado == 'B'){ 
					enviar_arquivo(sock, "azul.jpg",0x06); 
					azul_vivo = false; 
				}
				if (item_pisado == 'G'){ 
					enviar_arquivo(sock, "verde.jpg",0x06); 
					verde_vivo = false; 
				}
				if (item_pisado == 'Y'){ 
					enviar_arquivo(sock, "amarelo.jpg",0x06); 
					amarelo_vivo = false; 
				}
				
				// Finaliza o jogo
				if(fim_jogo == 6){ 
					enviar_mensagem(sock, 0x15, seq, 0, NULL);
					std::cout << "\nEnviou mensagem fim de jogo" << std::endl;
					break;
				}
				
				//Fantasma vermelho encontra o pacman
				if(vermelho_vivo){	
					chao_vermelho = mover_fantasma_vermelho(mapa, &pos_vermelho, &dir_vermelho, chao_vermelho);					
					if (chao_vermelho == 'P') {
						enviar_arquivo(sock, "vermelho.jpg", 0x06);					
						mapa[pos_vermelho.linha][pos_vermelho.coluna] = 'P';						
						vermelho_vivo = false;
					}
				}
				
				// Fantasma azul encontra o pacman
				if(azul_vivo){	
					chao_azul = mover_fantasma_azul(mapa, &pos_azul, &dir_azul, chao_azul);					
					if (chao_azul == 'P') {
						enviar_arquivo(sock, "azul.jpg", 0x06);					
						mapa[pos_azul.linha][pos_azul.coluna] = 'P';						
						azul_vivo = false;
					}
				}
				
				// Fantasma verde encontra o pacman
				if(verde_vivo){	
					chao_verde = mover_fantasma_verde(mapa, &pos_verde, &dir_verde, chao_verde,&lado_verde);
					if (chao_verde == 'P') {
						enviar_arquivo(sock, "verde.jpg", 0x06);					
						mapa[pos_verde.linha][pos_verde.coluna] = 'P';						
						verde_vivo = false;
					}
				}
				
				// Fantasma amarelo encontra o pacman
				if(amarelo_vivo){	
					chao_amarelo = mover_fantasma_amarelo(mapa, &pos_amarelo, chao_amarelo);					
					if (chao_amarelo == 'P') {
						enviar_arquivo(sock, "amarelo.jpg", 0x06);					
						mapa[pos_amarelo.linha][pos_amarelo.coluna] = 'P';						
						amarelo_vivo = false;
					}
				}
			
				// Atualiza o estado da visão
				rodadas++; 
				raio_atual = 1 + (rodadas / 5);
                
				if (raio_atual > 20) raio_atual = 20; 

				// Recorta o mapa
				tamanho_visao = gerar_visao(mapa, pos_pacman, raio_atual, visao);
                    
				// Fragmentação do turno
				int enviados = 0;
				while (enviados < tamanho_visao) {
					int lote = (tamanho_visao - enviados > 30) ? 30 : (tamanho_visao - enviados);
					enviar_mensagem(sock, 0x02, seq, lote, (uint8_t*)&visao[enviados]);
					seq++;
					enviados += lote;
				}
				// Envia pacote 0x04 para avisar que a tela fechou
				enviar_mensagem(sock, 0x04, seq, 0, NULL);
				seq++;
			}
		}
	}
	
    return 0;
}

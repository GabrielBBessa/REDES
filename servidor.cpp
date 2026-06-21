#include "rede.h"

int main(int argc, char *argv[]) {
	// Verifica se o usuário passou a interface
	if (argc < 2) {
		std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
		return 1;
	}
	
	char mapa[40][40];
	
	if (carregar_mapa(mapa , "mapa_do_jogo.csv")) 	std::cout << "Mapa carregado com sucesso" << std::endl;

	
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
    
	struct coordenada pos_pacman = encontrar_entidade(mapa,'P');
	struct coordenada pos_vermelho= encontrar_entidade(mapa, 'R');
	struct coordenada pos_azul = encontrar_entidade(mapa, 'B');
	struct coordenada pos_amarelo = encontrar_entidade(mapa, 'Y');
	
	// Variáveis de memória do Fantasma vermelho
	int dir_vermelho = 0;     
	char chao_vermelho = '0';
	bool vermelho_vivo = true;
	
	// Variáveis de memória do Fantasma azul
	int dir_azul = 0;     
	char chao_azul = '0';
	bool azul_vivo = true;
	
	// Variáveis de memória do Fantasma amarelo     
	char chao_amarelo = '0';
	bool amarelo_vivo = true;
	
	int rodadas = 0; 
	uint8_t seq = 0;
	int raio_atual = 1;
	char visao[50];
	
	char item_pisado;
	int tamanho_visao;
	
	int fim_jogo = 0;

	while (true) {
		struct Pacote pacote_recebido;
		
		// Fica travado aqui até o cliente mandar uma seta
        if (receber_pacote(sock, &pacote_recebido)) {
        
        	// Caso inicial
			if (pacote_recebido.tipo == 0x03) {
				// Reseta o estado do jogo caso o cliente tenha sido reiniciado
				rodadas = 0;
				raio_atual = 1;
				dir_vermelho = 0;
				chao_vermelho = '0';
				vermelho_vivo = true;
				dir_azul = 0;
				chao_azul = '0';
				azul_vivo = true;
				chao_amarelo = '0';
				amarelo_vivo = true;
				pos_pacman = encontrar_entidade(mapa,'P'); 
				pos_vermelho = encontrar_entidade(mapa, 'R');       
				pos_azul = encontrar_entidade(mapa, 'B');    
				pos_amarelo = encontrar_entidade(mapa, 'Y');        
				tamanho_visao = gerar_visao(mapa, pos_pacman, raio_atual, visao);
				enviar_mensagem(sock, 0x02, seq, tamanho_visao, (uint8_t*)visao);
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
				
				
				// Pacman pega o fantasma
				if (item_pisado == 'R'){
					enviar_arquivo(sock, "vermelho.jpg",0x06);
					vermelho_vivo= true;
				}
				if (item_pisado == 'B'){
					enviar_arquivo(sock, "azul.jpg",0x06);
					azul_vivo= false;
				}
				if (item_pisado == 'G')	enviar_arquivo(sock, "verde.jpg",0x06);
				if (item_pisado == 'Y'){	
					enviar_arquivo(sock, "amarelo.jpg",0x06);
					amarelo_vivo= false;
				}

				
				if( fim_jogo == 2){ 
					enviar_mensagem(sock, 0x15, seq, 0, NULL);
					std::cout << "\nEnviou mensagem fim de jogo" << std::endl;
					break;
				}
				
				//Fantasma vermelho pega o pacman
				if(vermelho_vivo){	
					chao_vermelho = mover_fantasma_vermelho(mapa, &pos_vermelho, &dir_vermelho, chao_vermelho);					
					if (chao_vermelho == 'P') {
						enviar_arquivo(sock, "vermelho.jpg", 0x06);					
						// Devolve o pacman pro mapa 
						mapa[pos_vermelho.linha][pos_vermelho.coluna] = 'P';						
						// Mata o fantasma!
						vermelho_vivo = false;
					}
				}
				
				// Fantasma azul pega o pacman
				if(azul_vivo){	
					chao_azul = mover_fantasma_azul(mapa, &pos_azul, &dir_azul, chao_azul);					
					if (chao_azul == 'P') {
						enviar_arquivo(sock, "azul.jpg", 0x06);					
						// Devolve o pacman pro mapa 
						mapa[pos_azul.linha][pos_azul.coluna] = 'P';						
						// Mata o fantasma!
						azul_vivo = false;
					}
				}
				
				// Fantasma amarelo pega o pacman
				if(amarelo_vivo){	
					chao_amarelo = mover_fantasma_amarelo(mapa, &pos_amarelo, chao_amarelo);					
					if (chao_amarelo == 'P') {
						enviar_arquivo(sock, "amarelo.jpg", 0x06);					
						// Devolve o pacman pro mapa 
						mapa[pos_amarelo.linha][pos_amarelo.coluna] = 'P';						
						// Mata o fantasma!
						amarelo_vivo = false;
					}
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




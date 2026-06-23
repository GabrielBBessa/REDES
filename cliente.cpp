#include "rede.h"

// Função auxiliar para desenhar o mapa na tela
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
    if (argc < 2) {
        std::cerr << "Uso: sudo " << argv[0] << " <interface>" << std::endl;
        return 1;
    }
    
    int socket = cria_raw_socket(argv[1]);
    if (socket == -1) return -1;
    
    uint8_t seq = 0;
    uint8_t tipo_movimento;
    char jogada;
    bool resposta_recebida;
    
    // Rastreador de pacotes repetidos do Servidor
    int ultimo_seq_recebido = -1;
    
    uint8_t buffer_visao[2000];
    int tamanho_acumulado = 0;
    
	enviar_mensagem(socket, 0x03, seq, 0, NULL);
	seq++;
	
	int contador_premios = 1;
	
	struct Pacote pacote_inicial;
	while(true) {
		if (receber_pacote(socket, &pacote_inicial)) {
            
            // Se for a mesma sequência do último, ignora (é retransmissão)
            if (pacote_inicial.sequencia == ultimo_seq_recebido) continue;
            ultimo_seq_recebido = pacote_inicial.sequencia;
            
			if (pacote_inicial.tipo == 0x02) { 
                memcpy(&buffer_visao[tamanho_acumulado], pacote_inicial.dados, pacote_inicial.tamanho);
				tamanho_acumulado += pacote_inicial.tamanho;
			}
            else if (pacote_inicial.tipo == 0x04) { 
				imprimir_mapa(buffer_visao, tamanho_acumulado);
                tamanho_acumulado = 0; 
				break; 
			}
		}
	}
	
	// LOOP PRINCIPAL DO JOGADOR
	while (true) {  
		std::cin >> jogada; 

		tipo_movimento = 0;

		// Traduz a letra digitada para o código hexadecimal 
		if (jogada == 'w' || jogada == 'W') tipo_movimento = 0x0C;
		else if (jogada == 's' || jogada == 'S') tipo_movimento = 0x0D;
		else if (jogada == 'a' || jogada == 'A') tipo_movimento = 0x0B;
		else if (jogada == 'd' || jogada == 'D') tipo_movimento = 0x0A;
		else {
			std::cout << "Comando inválido!" << std::endl;
			continue; 
		}

		// Envia o movimento para o Servidor
		enviar_mensagem(socket, tipo_movimento, seq, 0, NULL);
		seq++; 

		// Fica travado esperando a consequência do seu movimento
		struct Pacote pacote_recebido;
		resposta_recebida = false;
		
		std::string nome_coringa = ""; 
		bool primeiro_pacote = true;   
		int flags;

		while (!resposta_recebida) {
			if (receber_pacote(socket, &pacote_recebido)) {
                
                // Filtra as retransmissões no jogo rodando
                if (pacote_recebido.sequencia == ultimo_seq_recebido) continue;
                ultimo_seq_recebido = pacote_recebido.sequencia;

				if (pacote_recebido.tipo == 0x02) {
					memcpy(&buffer_visao[tamanho_acumulado], pacote_recebido.dados, pacote_recebido.tamanho);
					tamanho_acumulado += pacote_recebido.tamanho;
				}
				
                else if (pacote_recebido.tipo == 0x04) {
					std::cout << "\nVisão do PacMan:" << std::endl;
					imprimir_mapa(buffer_visao, tamanho_acumulado);
                    tamanho_acumulado = 0; 
					resposta_recebida = true; // Devolve o controle pro terminal
				}
                
				// Recebeu pacote de dados de arquivo
				else if (pacote_recebido.tipo >= 0x05 && pacote_recebido.tipo <= 0x07) {
					
					if(primeiro_pacote){
						if(contador_premios == 1)	nome_coringa = "primeiro_prêmio";
						else if(contador_premios == 2)	nome_coringa = "segundo_prêmio";
						else if(contador_premios == 3)	nome_coringa = "terceiro_prêmio";
						else if(contador_premios == 4)	nome_coringa = "quarto_prêmio";
						else if(contador_premios == 5)	nome_coringa = "quinto_prêmio";
						else if(contador_premios == 6)	nome_coringa = "sexto_prêmio";
						else if(contador_premios == 7)	nome_coringa = "setimo_prêmio";
						else if(contador_premios == 8)	nome_coringa = "oitavo_prêmio";
						else if(contador_premios == 9)	nome_coringa = "nono_prêmio";
						else if(contador_premios == 10)	nome_coringa = "decimo_prêmio";
						
						
						if (pacote_recebido.tipo == 0x05)	nome_coringa += ".txt";
						if (pacote_recebido.tipo == 0x06)	nome_coringa += ".jpg";
						if (pacote_recebido.tipo == 0x07)	nome_coringa += ".mp4";
					
					
						contador_premios++;
					}
					
					flags = O_WRONLY | O_CREAT | (primeiro_pacote ? O_TRUNC : O_APPEND);
					primeiro_pacote = false; 

					int arq = open(nome_coringa.c_str(), flags, 0666);
					if (arq != -1) {
						write(arq, pacote_recebido.dados, pacote_recebido.tamanho);
						close(arq);
					}
				}
				
				// Recebeu o Fim da Transmissão (Arquivos)
				else if (pacote_recebido.tipo == 0x10) {
					std::cout << "\nMídia recebida com sucesso! Abrindo " << nome_coringa << "..." << std::endl;
						
					std::string comando = "xdg-open " + nome_coringa + " > /dev/null 2>&1 &";
					system(comando.c_str());
				}			

				else if (pacote_recebido.tipo == 0x15) {
					std::cout << "\nO Servidor encerrou a partida! Fechando o jogo..." << std::endl;
					return 0; 
				}
			} 
		} 
	} 
    return 0;
}

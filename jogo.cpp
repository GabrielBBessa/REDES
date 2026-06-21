#include "jogo.h"

// Função auxiliar de sorteio
void coloca_aleatorio(char mapa[40][40],char item) {
	int x, y;
	do {
		x = rand() % 40;
		y = rand() % 40;
	} while (mapa[x][y] != '0'); // Só substitui se for chão livre
		
	mapa[x][y] = item;
}

int pode_pisar(char destino) {

    if (destino == 'X' || destino == 'R' || destino == 'B' || destino == 'G' || destino == 'Y') {
        return 0; 
    }
    
    return 1; 
}


// Abre o arquivo e coloca em 'mapa'
bool carregar_mapa(char mapa[40][40],const char* nome_arquivo){

	//Le o arquivo
	std::ifstream arquivo(nome_arquivo);
	
	// Se não conseguiu abrir, retorna false imediatamente
	if (!arquivo.is_open()) {
		return false;
	}
	
	std::string linha, celula;
	
	int i = 0; // Controle da linha 
	int j = 0; // Controle da coluna 
	
	// Lê linha por linha do arquivo
	while (std::getline(arquivo, linha) && i < 40) {
		
		// Separador é o conteudo da linha no formato necessário para ser tratado como um csv
		std::stringstream separador(linha);
		j = 0; 

		// Quebra a linha toda vez que achar um ';' (Celula recebe todo o conteúdo até quebrar a linha)
		while (std::getline(separador, celula, ';') && j < 40) {
			if (!celula.empty()) {
				// Pega a primeira letra da string e joga na matriz
				mapa[i][j] = celula[0];
			} 
			else {
				// Se a célula estiver vazia no CSV, assume como espaço vazio
				mapa[i][j] = '0';
			}
			j++;
		}
		i++;
	}
	
	arquivo.close();
	return true; 
}

// Coloca aleatoriamente todas as entidades do jogo no mapa
void sortear_entidades(char mapa[40][40]) {
    srand(time(NULL));

    coloca_aleatorio(mapa,'P');
    coloca_aleatorio(mapa,'R');
    coloca_aleatorio(mapa,'B');
    coloca_aleatorio(mapa,'G');
    coloca_aleatorio(mapa,'Y');

    coloca_aleatorio(mapa,'1');
    coloca_aleatorio(mapa,'2');
    coloca_aleatorio(mapa,'3');
    coloca_aleatorio(mapa,'4');
    coloca_aleatorio(mapa,'5');
    coloca_aleatorio(mapa,'6');
}

// Retorna a coordenada do pacman
struct coordenada encontrar_entidade(char mapa[40][40],char entidade){
	struct coordenada pos = {-1, -1}; 

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < 40; j++) {
			if (mapa[i][j] == entidade) { 
				pos.linha = i;
				pos.coluna = j;
				return pos;
			}
		}
	}
	return pos; 
}

// Retorna a quantidade exata de bytes preenchidos (9, 25 ou 49)
int gerar_visao (char mapa[40][40], struct coordenada centro, int raio, char visao_cliente[2000]){

    int indice = 0;

    
    for (int i = centro.linha - raio; i <= centro.linha + raio; i++) {
        
        
        for (int j = centro.coluna - raio; j <= centro.coluna + raio; j++) {
            
            // Se tentar olhar para fora do mapa 40x40, manda uma parede ('X')
            if (i < 0 || i >= 40 || j < 0 || j >= 40) {
                visao_cliente[indice] = 'X'; 
            } 
            else {
                // Caso contrário, manda o que estiver no mapa
                visao_cliente[indice] = mapa[i][j]; 
            }
            
            indice++; 
        }
    }
  
    return indice; 
}

// Retorna o char do item que o PacMan encontrou na nova casa
char mover_pacman(char mapa[40][40], struct coordenada *pos, uint8_t direcao){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;

    if (direcao == 0x0A) nova_coluna++;      // 10 (0x0A): Direita -> Avança na coluna
    else if (direcao == 0x0B) nova_coluna--; // 11 (0x0B): Esquerda -> Volta na coluna
    else if (direcao == 0x0C) nova_linha--;   // 12 (0x0C): Cima -> Volta na linha
    else if (direcao == 0x0D) nova_linha++;   // 13 (0x0D): Baixo -> Avança na linha

    // Verifica se o jogador tentou sair dos limites da matriz 40x40
    if (nova_linha < 0 || nova_linha >= 40 || nova_coluna < 0 || nova_coluna >= 40) {
        return 'X'; 
    }

    char item_destino = mapa[nova_linha][nova_coluna];

    // Se for 'X', o movimento é cancelado
    // Retorna 'X' e a coordenada oficial não muda
    if (item_destino == 'X') {
        return 'X';
    }

    // Apaga o PacMan da casa antiga, deixando um rastro de chão '0'
    mapa[pos->linha][pos->coluna] = '0';
    
    // Atualiza a coordenada oficial do jogo
    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    // Coloca o PacMan na nova casa
    mapa[pos->linha][pos->coluna] = 'P';

    // Retorna o que tinha lá antes do PacMan pisar
    return item_destino;
}

char mover_fantasma_vermelho(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    
    int tentativas = 0;

	/* Direções: 0 é olhando para cima
		1 é olhando para esquerda
		2 é olhando para baixo 
		3 é olhando para direita
	*/

    while (!andou && tentativas < 4){
        if (*direcao == 0){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna - 1])){
                nova_coluna--;
                andou = 1;
            }
            else
                *direcao = 1;
        }

        if (*direcao == 1){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha + 1][pos->coluna])){
                nova_linha++;
                andou = 1;
            }
            else
                *direcao = 2;
        }

        if (*direcao == 2){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna + 1])){
                nova_coluna++;
                andou = 1;
            }
            else
                *direcao = 3;
        }

        if (*direcao == 3){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha - 1][pos->coluna])){
                nova_linha--;
                andou = 1;
            }
            else
                *direcao = 0;
        }
    }
    
    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    // Atualiza a coordenada oficial do jogo
    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    // Coloca o fantasma na nova casa
    mapa[pos->linha][pos->coluna] = 'R';

    // Retorna o que tinha lá antes do fantasma pisar
    return item_destino;
}


char mover_fantasma_azul(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    
    int tentativas = 0;
    
	/* Direções: 0 é olhando para cima
		1 é olhando para esquerda
		2 é olhando para baixo 
		3 é olhando para direita
	*/


    while (!andou && tentativas < 4){
        if (*direcao == 0){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha - 1][pos->coluna])){
                nova_linha--;
                andou = 1;
            }
            else
                *direcao = 1;
        }

        if (*direcao == 1){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna - 1])){
                nova_coluna--;
                andou = 1;
            }
            else
                *direcao = 2;
        }

        if (*direcao == 2){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha + 1][pos->coluna])){
                nova_linha++;
                andou = 1;
            }
            else
                *direcao = 3;
        }

        if (*direcao == 3){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna + 1])){
                nova_coluna++;
                andou = 1;
            }
            else
                *direcao = 0;
        }
    }
    
    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    // Atualiza a coordenada oficial do jogo
    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    // Coloca o fantasma na nova casa
    mapa[pos->linha][pos->coluna] = 'B';

    // Retorna o que tinha lá antes do fantasma pisar
    return item_destino;
}

char mover_fantasma_verde(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior,int* lado){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    
    int direcao_antiga = *direcao;
    
    int tentativas = 0;

    while (!andou && tentativas < 4){
    	// Lado 0 mão esquerda , lado 1 mão direita
    	if(*lado == 0){
    	
			if (*direcao == 0) *direcao = 3;
			else if (*direcao == 1) *direcao = 0;
			else if (*direcao == 2) *direcao = 1;
			else if (*direcao == 3) *direcao = 2;
			
				if (*direcao == 0){
				tentativas++;
		        if (pode_pisar(mapa[pos->linha - 1][pos->coluna])){
		            nova_linha--;
		            andou = 1;
		        }
		        else
		            *direcao = 1;
		    }

		    if (*direcao == 1){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha][pos->coluna + 1])){
		            nova_coluna++;
		            andou = 1;
		        }
		        else
		            *direcao = 2;
		    }

		    if (*direcao == 2){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha + 1][pos->coluna])){
		            nova_linha++;
		            andou = 1;
		        }
		        else
		            *direcao = 3;
		    }

		    if (*direcao == 3){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha][pos->coluna - 1])){
		            nova_coluna--;
		            andou = 1;
		        }
		        else
		            *direcao = 0;
		    }
        }
    	if(*lado == 1){
    	
			if (*direcao == 0) *direcao = 1;
			else if (*direcao == 1) *direcao = 2;
			else if (*direcao == 2) *direcao = 3;
			else if (*direcao == 3) *direcao = 0;
			
		    if (*direcao == 0){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha - 1][pos->coluna])){
		            nova_linha--;
		            andou = 1;
		        }
		        else
		            *direcao = 1;
		    }

		    if (*direcao == 1){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha][pos->coluna - 1])){
		            nova_coluna--;
		            andou = 1;
		        }
		        else
		            *direcao = 2;
		    }

		    if (*direcao == 2){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha + 1][pos->coluna])){
		            nova_linha++;
		            andou = 1;
		        }
		        else
		            *direcao = 3;
		    }

		    if (*direcao == 3){
		    tentativas++;
		        if (pode_pisar(mapa[pos->linha][pos->coluna + 1])){
		            nova_coluna++;
		            andou = 1;
		        }
		        else
		            *direcao = 0;
		    }
		}		  
    }
    
    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    // Atualiza a coordenada oficial do jogo
    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    // Coloca o fantasma na nova casa
    mapa[pos->linha][pos->coluna] = 'G';

	if (*direcao != direcao_antiga) *lado = 1 - *lado;

    // Retorna o que tinha lá antes do fantasma pisar
    return item_destino;
}

char mover_fantasma_amarelo(char mapa[40][40], struct coordenada *pos, char item_anterior){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    
    int direcao = rand() % 4;
    
    int tentativas = 0;


    while (!andou && tentativas < 4){
        if (direcao == 0){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha - 1][pos->coluna])){
                nova_linha--;
                andou = 1;
            }
            else
				direcao = rand() % 4;
        }

        if (direcao == 1){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna - 1])){
                nova_coluna--;
                andou = 1;
            }
            else
                direcao = rand() % 4;
        }

        if (direcao == 2){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha + 1][pos->coluna])){
                nova_linha++;
                andou = 1;
            }
            else
                direcao = rand() % 4;
        }

        if (direcao == 3){
        	tentativas++;
            if (pode_pisar(mapa[pos->linha][pos->coluna + 1])){
                nova_coluna++;
                andou = 1;
            }
            else
                direcao = rand() % 4;
        }
	}
    
    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    // Atualiza a coordenada oficial do jogo
    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    // Coloca o fantasma na nova casa
    mapa[pos->linha][pos->coluna] = 'Y';

    // Retorna o que tinha lá antes do fantasma pisar
    return item_destino;
}




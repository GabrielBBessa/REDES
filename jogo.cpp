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

    //coloca_aleatorio(mapa,'P');

    mapa[21][21] = 'P'; 
    
	//coloca_aleatorio(mapa,'R');
    //coloca_aleatorio(mapa,'B');
    //coloca_aleatorio(mapa,'G');
    //coloca_aleatorio(mapa,'Y');
    
    mapa[17][22] = 'G'; 
    mapa[25][21] = 'B'; 
    mapa[21][17] = 'R'; 
    mapa[21][25] = 'Y'; 

    coloca_aleatorio(mapa,'1');
    coloca_aleatorio(mapa,'2');
    coloca_aleatorio(mapa,'3');
    coloca_aleatorio(mapa,'4');
    coloca_aleatorio(mapa,'5');
    //coloca_aleatorio(mapa,'6');
    mapa[21][22] = '6'; 
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

    while (!andou && tentativas < 4){

        tentativas++;

        int dir_teste;

        if (tentativas == 1)
            dir_teste = (*direcao + 3) % 4;   // direita
        else if (tentativas == 2)
            dir_teste = *direcao;            // frente
        else if (tentativas == 3)
            dir_teste = (*direcao + 1) % 4;  // esquerda
        else
            dir_teste = (*direcao + 2) % 4;  // volta


        int linha = pos->linha;
        int coluna = pos->coluna;

        if (dir_teste == 0)
            linha--;
        else if (dir_teste == 1)
            coluna--;
        else if (dir_teste == 2)
            linha++;
        else if (dir_teste == 3)
            coluna++;


        if (pode_pisar(mapa[linha][coluna])){
            nova_linha = linha;
            nova_coluna = coluna;
            *direcao = dir_teste;
            andou = 1;
        }
    }

    /* só atualiza se andou de verdade */
    if (andou){
        mapa[pos->linha][pos->coluna] = item_anterior;

        item_anterior = mapa[nova_linha][nova_coluna];

        pos->linha = nova_linha;
        pos->coluna = nova_coluna;

        mapa[pos->linha][pos->coluna] = 'R';
    }

    return item_anterior;
}

char mover_fantasma_azul(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    int tentativas = 0;

    while (!andou && tentativas < 4){

        tentativas++;

        int dir_teste;

        /* MÃO DIREITA:
           direita -> frente -> esquerda -> volta
        */

        if (tentativas == 1)
            dir_teste = (*direcao + 3) % 4;   // direita
        else if (tentativas == 2)
            dir_teste = *direcao;            // frente
        else if (tentativas == 3)
            dir_teste = (*direcao + 1) % 4;  // esquerda
        else
            dir_teste = (*direcao + 2) % 4;  // volta


        /* checagem de movimento */

        if (dir_teste == 0 && pode_pisar(mapa[pos->linha - 1][pos->coluna])){
            nova_linha--;
            andou = 1;
        }
        else if (dir_teste == 1 && pode_pisar(mapa[pos->linha][pos->coluna - 1])){
            nova_coluna--;
            andou = 1;
        }
        else if (dir_teste == 2 && pode_pisar(mapa[pos->linha + 1][pos->coluna])){
            nova_linha++;
            andou = 1;
        }
        else if (dir_teste == 3 && pode_pisar(mapa[pos->linha][pos->coluna + 1])){
            nova_coluna++;
            andou = 1;
        }

        if (andou)
            *direcao = dir_teste;
    }

    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    mapa[pos->linha][pos->coluna] = 'B';

    return item_destino;
}

char mover_fantasma_verde(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior,int* lado){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    int tentativas = 0;
    
    int livres = 0;
    if (pode_pisar(mapa[pos->linha - 1][pos->coluna])) livres++; // Cima
    if (pode_pisar(mapa[pos->linha + 1][pos->coluna])) livres++; // Baixo
    if (pode_pisar(mapa[pos->linha][pos->coluna - 1])) livres++; // Esquerda
    if (pode_pisar(mapa[pos->linha][pos->coluna + 1])) livres++; // Direita
    
    // Se for uma bifurcação real (> 2), alterna o lado imediatamente
    if (livres > 2) {
        *lado = (*lado == 1) ? 0 : 1;
    }

    while (!andou && tentativas < 4){

        tentativas++;

        int dir_teste;

        /* escolhe regra atual */
        if (*lado == 1){
            /* MÃO ESQUERDA: esquerda -> frente -> direita -> volta */
            if (tentativas == 1)
                dir_teste = (*direcao + 1) % 4;
            else if (tentativas == 2)
                dir_teste = *direcao;
            else if (tentativas == 3)
                dir_teste = (*direcao + 3) % 4;
            else
                dir_teste = (*direcao + 2) % 4;
        }
        else{
            /* MÃO DIREITA: direita -> frente -> esquerda -> volta */
            if (tentativas == 1)
                dir_teste = (*direcao + 3) % 4;
            else if (tentativas == 2)
                dir_teste = *direcao;
            else if (tentativas == 3)
                dir_teste = (*direcao + 1) % 4;
            else
                dir_teste = (*direcao + 2) % 4;
        }


        /* tenta mover */

        if (dir_teste == 0 && pode_pisar(mapa[pos->linha - 1][pos->coluna])){
            nova_linha--;
            andou = 1;
        }
        else if (dir_teste == 1 && pode_pisar(mapa[pos->linha][pos->coluna - 1])){
            nova_coluna--;
            andou = 1;
        }
        else if (dir_teste == 2 && pode_pisar(mapa[pos->linha + 1][pos->coluna])){
            nova_linha++;
            andou = 1;
        }
        else if (dir_teste == 3 && pode_pisar(mapa[pos->linha][pos->coluna + 1])){
            nova_coluna++;
            andou = 1;
        }

        if (andou)
            *direcao = dir_teste;
    }

    char item_destino = mapa[nova_linha][nova_coluna];

    mapa[pos->linha][pos->coluna] = item_anterior;

    pos->linha = nova_linha;
    pos->coluna = nova_coluna;

    mapa[pos->linha][pos->coluna] = 'G';

    return item_destino;
}

char mover_fantasma_amarelo(char mapa[40][40], struct coordenada *pos, char item_anterior){

    int nova_linha = pos->linha;
    int nova_coluna = pos->coluna;
    int andou = 0;
    
    int direcao = rand() % 4;
	int linha_sorteada;
	int col_sorteada;
    
    
    int tentativas = 0;


    while (!andou && tentativas < 4){
    
		linha_sorteada = pos->linha;
		col_sorteada = pos->coluna;
		
		if (direcao == 0) linha_sorteada--;      // Cima
        else if (direcao == 1) col_sorteada--;   // Esquerda
        else if (direcao == 2) linha_sorteada++; // Baixo
        else if (direcao == 3) col_sorteada++;   // Direita		
        
        // Verifica se pode pisar na casa alvo
        if (pode_pisar(mapa[linha_sorteada][col_sorteada])) {
            nova_linha = linha_sorteada;
            nova_coluna = col_sorteada;
            andou = 1;
        } 
        else 	direcao = rand() % 4;
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




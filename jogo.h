#ifndef _JOGO_
#define _JOGO_

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> 
#include <ctime>   
#include <sstream>
#include <cstdint>

struct coordenada {
    int linha;
    int coluna;
};

// Abre o arquivo e coloca em 'mapa'
bool carregar_mapa(char mapa[40][40],const char* nome_arquivo);

// Coloca aleatoriamente todas as entidades do jogo no mapa
void sortear_entidades(char mapa[40][40]);

// Retorna a coordenada do pacman
struct coordenada encontrar_entidade(char mapa[40][40],char entidade);

// Retorna a quantidade exata de bytes preenchidos (9, 25 ou 49) 
int gerar_visao(char mapa[40][40], coordenada centro, int raio, char visao_cliente[2000]);

// Retorna o char do item que o PacMan encontrou na nova casa
char mover_pacman(char mapa[40][40], struct coordenada *pos, uint8_t direcao);

char mover_fantasma_vermelho(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior);

char mover_fantasma_azul(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior);

char mover_fantasma_verde(char mapa[40][40], struct coordenada *pos, int* direcao, char item_anterior,int* lado);

char mover_fantasma_amarelo(char mapa[40][40], struct coordenada *pos, char item_anterior);

void imprimir_mapa(uint8_t *dados, int tamanho);

#endif

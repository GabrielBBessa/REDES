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
struct coordenada encontrar_pacman(char mapa[40][40]);

// Retorna a quantidade exata de bytes preenchidos (9, 25 ou 49) 
int gerar_visao(char mapa[40][40], coordenada centro, int raio, char visao_cliente[50]);

// Retorna o char do item que o PacMan encontrou na nova casa
char mover_pacman(char mapa[40][40], struct coordenada *pos, uint8_t direcao);

void imprimir_mapa(uint8_t *dados, int tamanho);

#endif

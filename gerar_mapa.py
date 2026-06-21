import random
import sys

# Aumenta o limite de recursão para escavar o mapa inteiro
sys.setrecursionlimit(3000)
TAMANHO = 40

# Preenche toda a matriz inicial com paredes sóbrias 'X'
mapa = [['X' for _ in range(TAMANHO)] for _ in range(TAMANHO)]

def posicao_valida(linha, coluna):
    # Proíbe tocar nas bordas externas do mapa 40x40
    if linha <= 0 or linha >= TAMANHO - 1 or coluna <= 0 or coluna >= TAMANHO - 1:
        return False
    
    # [BLOQUEIO CRUCIAL]: Proíbe a escavação no bloco de 3x3 central
    # Isso garante que as 4 paredes em cruz em volta do PacMan nunca sejam derrubadas
    if 18 <= linha <= 20 and 18 <= coluna <= 20:
        return False
        
    return True

def escavar_labirinto(linha, coluna):
    mapa[linha][coluna] = '0'
    
    # Caminha de 2 em 2 blocos para manter a estrutura correta de corredores
    direcoes = [(0, 2), (2, 0), (0, -2), (-2, 0)]
    random.shuffle(direcoes)
    
    for d_linha, d_coluna in direcoes:
        nova_linha, nova_coluna = linha + d_linha, coluna + d_coluna
        
        if posicao_valida(nova_linha, nova_coluna) and mapa[nova_linha][nova_coluna] == 'X':
            # Derruba a parede intermediária abrindo caminho continuo
            mapa[linha + d_linha // 2][coluna + d_coluna // 2] = '0'
            escavar_labirinto(nova_linha, nova_coluna)

# Inicia a geração a partir do canto superior esquerdo
escavar_labirinto(1, 1)

# Abre manualmente apenas a casa interna do PacMan (totalmente cercada por X)
mapa[19][19] = '0'

# Garante que as 4 saídas externas onde os fantasmas nascem sejam caminhos livres no labirinto
spawns_fantasmas = [(17, 19), (21, 19), (19, 17), (19, 21)]
for l, c in spawns_fantasmas:
    mapa[l][c] = '0'

# Garante espaço para os itens 1 e 2 longe da jaula
mapa[3][35] = '0'
mapa[35][3] = '0'

# Grava no arquivo esperado pelo seu servidor.cpp
nome_arquivo = 'mapa_do_jogo.csv'
with open(nome_arquivo, 'w') as f:
    for linha in mapa:
        f.write(';'.join(linha) + '\n')

print(f"[{nome_arquivo}] com Jaula de Isolamento em Cruz gerado com sucesso!")

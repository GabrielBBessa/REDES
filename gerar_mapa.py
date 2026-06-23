import random
import csv
import sys

# Aumenta o limite de recursão por segurança (o DFS pode descer fundo no labirinto)
sys.setrecursionlimit(2000)

TAM = 40
# Cria a matriz 40x40 preenchida com 'X'
mapa = [['X' for _ in range(TAM)] for _ in range(TAM)]

# Direções para pular de 2 em 2
dx = [-2, 0, 2, 0]
dy = [0, 2, 0, -2]

def dentro_limites(x, y):
    return 0 < x < TAM - 1 and 0 < y < TAM - 1

def gerar_labirinto_dfs(x, y):
    mapa[x][y] = '0'
    
    # Embaralha as direções
    dirs = [0, 1, 2, 3]
    random.shuffle(dirs)
    
    for d in dirs:
        nx = x + dx[d]
        ny = y + dy[d]
        
        # Se a próxima casa está dentro dos limites e não foi visitada ('X')
        if dentro_limites(nx, ny) and mapa[nx][ny] == 'X':
            # Quebra a parede do meio
            mapa[x + dx[d] // 2][y + dy[d] // 2] = '0'
            gerar_labirinto_dfs(nx, ny)

def colocar_entidade(entidade):
    while True:
        x = random.randint(1, TAM - 2)
        y = random.randint(1, TAM - 2)
        if mapa[x][y] == '0':
            mapa[x][y] = entidade
            break

# 1. Gera a base do labirinto
gerar_labirinto_dfs(1, 1)

# 2. Quebra de paredes para criar cruzamentos (15% do mapa)
paredes_para_quebrar = int((TAM * TAM) * 0.15)

while paredes_para_quebrar > 0:
    x = random.randint(1, TAM - 2)
    y = random.randint(1, TAM - 2)
    
    if mapa[x][y] == 'X':
        horizontal = (mapa[x][y-1] == '0' and mapa[x][y+1] == '0')
        vertical = (mapa[x-1][y] == '0' and mapa[x+1][y] == '0')
        
        if horizontal or vertical:
            mapa[x][y] = '0'
            paredes_para_quebrar -= 1

# 3. Coloca as entidades
entidades = ['P', 'R', 'B', 'G', 'Y', '1', '2', '3', '4', '5', '6']
for e in entidades:
    colocar_entidade(e)

# 4. Exporta para CSV usando o delimitador correto
with open('mapa_do_jogo.csv', mode='w', newline='') as arquivo:
    escritor = csv.writer(arquivo, delimiter=';')
    escritor.writerows(mapa)

print("Mapa gerado com sucesso em 'mapa_do_jogo.csv'!")

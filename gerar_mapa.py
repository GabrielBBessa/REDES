import random

TAM = 40

# Inicializa tudo como parede
lab = [['X' for _ in range(TAM)] for _ in range(TAM)]
visitado = [[False for _ in range(TAM)] for _ in range(TAM)]

def dentro(x, y):
    return 1 <= x < TAM - 1 and 1 <= y < TAM - 1

def dfs(x, y):
    visitado[x][y] = True
    lab[x][y] = '0'

    direcoes = [(0, 2), (0, -2), (2, 0), (-2, 0)]
    random.shuffle(direcoes)

    for dx, dy in direcoes:
        nx = x + dx
        ny = y + dy

        if dentro(nx, ny) and not visitado[nx][ny]:
            lab[x + dx // 2][y + dy // 2] = '0'
            dfs(nx, ny)

# Gera o labirinto
dfs(1, 1)

# Centro da jaula
cx = 20
cy = 20

# Cria uma jaula 5x5 totalmente fechada
for i in range(cx - 2, cx + 3):
    for j in range(cy - 2, cy + 3):
        lab[i][j] = 'X'

# Pac-Man preso no centro
lab[cx][cy] = '0'

# Espaços garantidos para os fantasmas
lab[cx - 4][cy] = '0'  # norte
lab[cx + 4][cy] = '0'  # sul
lab[cx][cy - 4] = '0'  # oeste
lab[cx][cy + 4] = '0'  # leste

# Garante conexão com o restante do labirinto
lab[cx - 5][cy] = '0'
lab[cx + 5][cy] = '0'
lab[cx][cy - 5] = '0'
lab[cx][cy + 5] = '0'

# Bordas fechadas
for i in range(TAM):
    lab[0][i] = 'X'
    lab[TAM - 1][i] = 'X'
    lab[i][0] = 'X'
    lab[i][TAM - 1] = 'X'

# Salva CSV usando ;
with open("mapa_do_jogo.csv", "w") as arquivo:
    for linha in lab:
        arquivo.write(";".join(linha) + "\n")

print("Mapa salvo em mapa_do_jogo.csv")

#include "protocolo.h"

uint8_t calcula_crc(struct Pacote *p) {
    uint8_t crc = 0x00;
    uint8_t *dados = (uint8_t *)p; // Trata a struct como um array de bytes
    

    int tamanho_total = sizeof(struct Pacote) - 1;

    for (int i = 0; i < tamanho_total; i++) {
        crc ^= dados[i]; // Operação XOR
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) { // Se o bit mais significativo for 1
                crc = (crc << 1) ^ 0x07; // Shift e XOR com o polinômio
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
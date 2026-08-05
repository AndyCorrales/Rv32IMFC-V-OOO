#ifndef SOC_CONFIG_H
#define SOC_CONFIG_H

// Parametros de la microarquitectura: latencias de las unidades
// funcionales y tamanos de las estructuras. Es el archivo que se toca para
// explorar el espacio de diseno (mas ALUs, ROB mas profundo, etc.).
#include "soc_top.h"
#include "rv32i_defs.h"


// ---- latencias de las unidades funcionales (en ticks) ----
static const int ALU_LAT   = 1;
static const int MUL_LAT   = 3;
static const int DIV_LAT   = 8;
static const int BR_LAT    = 1;
static const int FPU_LAT_ADDMUL = 3; // FADD/FSUB/FMUL
static const int FPU_LAT_DIV    = 8; // FDIV/FSQRT
static const int FPU_LAT_FMA    = 4; // familia FMADD (R4)
static const int FPU_LAT_MISC   = 2; // sgnj/minmax/cmp/cvt/mv/class

static const int ROB_SZ  = 8;
static const int N_ALU   = 2;
// latencias del COPROCESADOR, por unidad funcional (ver el diagrama):
static const int VALU_LAT  = 2; // aritmetica/logica/mascaras/reducciones
static const int VMUL_LAT  = 4; // multiplicacion/division vectorial
static const int VSLDU_LAT = 2; // permutaciones (slide/gather/compress)

// ---- RVV: opcodes/campos verificados contra la especificacion oficial
// (RISC-V "V" Vector Extension v1.0), mismos valores que rv32_vector.cpp.
// UART mapeado en memoria: un store a esta direccion imprime el byte bajo
// por consola (lo minimo para putchar/printf). MMIO alto, fuera del rango de
// dmem y en la misma direccion que en la pista TLM, para correr los mismos
// binarios.
static const uint32_t UART_TX_ADDR = 0x20000000;

#endif // SOC_CONFIG_H

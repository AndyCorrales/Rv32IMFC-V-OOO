#ifndef RV32_OOO_H
#define RV32_OOO_H

#include <ap_int.h>

// Core RV32IMFC fuera de orden (Tomasulo), sintetizable con Vitis HLS.
//
//   - Fetch de 16 bits: expande comprimidas (C) en cualquier alineacion par.
//   - Renombrado con RAT (enteros) y FRAT (flotantes) sobre un ROB unificado
//     de 8 entradas -> las dependencias int<->float usan el mismo mecanismo
//     de tags que una entera.
//   - Unidades con reservation station propia: 2x ALU, MUL/DIV, FPU (ext. F
//     completa, incluye familia FMADD R4), LSU, BR y una unidad vectorial VEC.
//   - CDB: al completar, cada unidad difunde (tag, valor) y despierta las RS
//     que esperaban ese tag. Los valores F viajan como bits IEEE-754 crudos.
//   - Coprocesamiento vectorial RVV integrado al mismo flujo OOO: una vectorial
//     ejecuta mientras instrucciones escalares independientes completan
//     alrededor. Una sola RS VEC serializa las vectoriales entre si (sin VRAT);
//     vle/vse se resuelven en la cabeza del ROB.
//
// Alcance (ver LIMITACIONES.md): RVV Zve32x, SEW=32 / LMUL=1 / VLEN=128
// (VLMAX=4); rm=RNE fijo. Fin de programa: halfword 0x0000 con el ROB vacio.
//
// Un tick = un ciclo de reloj; el estado vive en variables static. Las salidas
// de dispatch/completion/commit dejan al testbench reconstruir el estado solo
// desde el retiro (sin backdoor). vregs_out expone el banco vectorial.

// Memorias de instrucciones y datos: 16384 palabras = 64 KB cada una
// (dimensionadas para un binario con el printf de newlib).
#define OOO_IMEM_WORDS 16384
#define OOO_DMEM_WORDS 16384
#define OOO_VEC_NUM_VREGS 32
#define OOO_VEC_LANES 4
#define OOO_VEC_REGFILE_LEN (OOO_VEC_NUM_VREGS * OOO_VEC_LANES)

void riscv_soc_tick(
    ap_uint<1>  reset,
    ap_uint<32> imem[OOO_IMEM_WORDS],
    ap_uint<32> dmem[OOO_DMEM_WORDS],
    // evento de dispatch (para que el TB mapee tag ROB -> instruccion)
    ap_uint<1>&  disp_valid,
    ap_uint<3>&  disp_tag,
    ap_uint<32>& disp_pc,
    // eventos de completion, uno por unidad funcional
    ap_uint<1>& alu0_done, ap_uint<3>& alu0_tag,
    ap_uint<1>& alu1_done, ap_uint<3>& alu1_tag,
    ap_uint<1>& md_done,   ap_uint<3>& md_tag,
    ap_uint<1>& fpu_done,  ap_uint<3>& fpu_tag,
    ap_uint<1>& lsu_done,  ap_uint<3>& lsu_tag,
    ap_uint<1>& br_done,   ap_uint<3>& br_tag,
    ap_uint<1>& vec_done,  ap_uint<3>& vec_tag,
    // stream de commit (retiro en orden)
    ap_uint<1>&  commit_valid,
    ap_uint<1>&  commit_is_fp, // 1: escribe f[rd]; 0: escribe x[rd] (rd=0 => nada)
    ap_uint<5>&  commit_rd,
    ap_uint<32>& commit_value,
    // banco vectorial (estado completo, actualizado cada ciclo)
    ap_uint<32>  vregs_out[OOO_VEC_REGFILE_LEN],
    ap_uint<1>&  halted
);

// contadores del predictor TAGE (definidos en soc_top.cpp). Son de
// OBSERVABILIDAD para el testbench: no existen en la sintesis (en
// hardware se expondrian como registros del esclavo AXI4-Lite).
#ifndef __SYNTHESIS__
unsigned int soc_stat_branches();
unsigned int soc_stat_mispredicts();
#endif

#endif // RV32_OOO_H

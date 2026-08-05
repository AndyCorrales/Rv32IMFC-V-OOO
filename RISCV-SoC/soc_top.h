#ifndef RV32_OOO_H
#define RV32_OOO_H

#include <ap_int.h>

// core rv32imfc fuera de orden (tomasulo), sintetizable en vitis hls.
// fetch de 16b (expande las comprimidas C). renombrado rat/frat sobre un rob
// unico de 8, asi que las dependencias int<->float usan los mismos tags.
// unidades con su reservation station: 2x alu, mul/div, fpu, lsu, br y la vec.
// el cdb difunde (tag, valor) al terminar y despierta las rs que lo esperaban.
// la rvv va integrada al mismo flujo ooo: la vectorial corre mientras el escalar
// avanza. una sola rs vec las serializa entre si (sin vrat), vle/vse en la cabeza del rob.
// alcance (ver LIMITACIONES.md): rvv zve32x, sew32/lmul1/vlen128, rm=rne fijo.
// un tick = un ciclo, el estado vive en static. las salidas dispatch/commit dejan
// al tb reconstruir todo desde el retiro, sin backdoor. vregs_out saca el banco vec.

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

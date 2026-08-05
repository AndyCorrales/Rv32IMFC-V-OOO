#ifndef AXI_INTERCONNECT_H
#define AXI_INTERCONNECT_H

// AXI interconnect + arbitro. Los dos maestros -- LSU escalar y VLSU
// vectorial -- comparten el unico puerto de la memoria de datos: el arbitro
// concede un acceso por ciclo con prioridad commit > VLSU > LSU (el perdedor
// reintenta al siguiente). Por eso la VLSU mueve un elemento por ciclo.
//
// El plano de control es AXI4-Lite (s_axilite, ver soc_top.cpp): el PS del Kria
// arranca el core y lee su estado. La IMEM no pasa por aqui (es BRAM local del
// frontend). dmem_load/dmem_store acceden por byte/half/word sobre la memoria,
// que es por palabras.
#include "soc_top.h"
#include "rv32i_defs.h"
#include "soc_state.h"

// ---- el arbitro: una concesion por ciclo ----
static bool mem_port_used;   // lo resetea el tick al empezar cada ciclo

static bool axi_grant() {
#pragma HLS INLINE
    if (mem_port_used) return false;  // el puerto ya se uso este ciclo
    mem_port_used = true;
    return true;
}

static ap_uint<32> dmem_load(ap_uint<32> dmem[OOO_DMEM_WORDS], ap_uint<32> addr, ap_uint<3> f3) {
    ap_uint<32> word = dmem[(addr >> 2) & (OOO_DMEM_WORDS - 1)];
    ap_uint<5> shift = ap_uint<5>(addr.range(1, 0)) * 8;
    ap_uint<32> sh = word >> shift;
    switch (f3.to_uint()) {
        case rv32i::Funct3_LOAD::LB:
            return static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(ap_uint<8>(sh.range(7, 0)).to_uint())));
        case rv32i::Funct3_LOAD::LH:
            return static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(ap_uint<16>(sh.range(15, 0)).to_uint())));
        case rv32i::Funct3_LOAD::LW:  return word;
        case rv32i::Funct3_LOAD::LBU: return ap_uint<8>(sh.range(7, 0)).to_uint();
        case rv32i::Funct3_LOAD::LHU: return ap_uint<16>(sh.range(15, 0)).to_uint();
        default:                      return 0;
    }
}

static void dmem_store(ap_uint<32> dmem[OOO_DMEM_WORDS], ap_uint<32> addr, ap_uint<32> wdata, ap_uint<3> f3) {
    ap_uint<32> wa = (addr >> 2) & (OOO_DMEM_WORDS - 1);
    ap_uint<5> shift = ap_uint<5>(addr.range(1, 0)) * 8;
    switch (f3.to_uint()) {
        case rv32i::Funct3_STORE::SW:
            dmem[wa] = wdata;
            break;
        case rv32i::Funct3_STORE::SH: {
            ap_uint<32> word = dmem[wa];
            ap_uint<32> mask = ap_uint<32>(0xFFFF) << shift;
            dmem[wa] = (word & ~mask) | ((wdata & ap_uint<32>(0xFFFF)) << shift);
            break;
        }
        case rv32i::Funct3_STORE::SB: {
            ap_uint<32> word = dmem[wa];
            ap_uint<32> mask = ap_uint<32>(0xFF) << shift;
            dmem[wa] = (word & ~mask) | ((wdata & ap_uint<32>(0xFF)) << shift);
            break;
        }
    }
}

// Descarta todo lo que hay en vuelo y reinicia el frente del pipeline.
// En este core es SUFICIENTE para excepciones precisas: como no hay
// especulacion, un trap solo puede tomarse en la CABEZA del ROB, momento
// en que todo lo anterior ya committeo y todo lo posterior todavia no ha
// modificado el estado arquitectonico. Por eso un flush total es
// correcto y no hace falta maquinaria de recuperacion especulativa.

#endif // AXI_INTERCONNECT_H

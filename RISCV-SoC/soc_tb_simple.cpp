// soc_tb_simple.cpp — testbench LEGIBLE del SoC, función por función.
//
// A diferencia de soc_tb.cpp (la regresión rigurosa de 8 suites y 218
// checks, con programas hand-assembled densos), este testbench está
// pensado para LEERSE y ENTENDERSE: cada función se prueba con un
// programita corto escrito con mnemónicos tipo ensamblador.
//
// En vez de:
//     push32(i_type(OP_IMM, 20, F3A::ADD_SUB, 0, 3));   // addi x20,x0,3
// se escribe:
//     P p; p.li(x20, 3);                                // x20 = 3
//
// Compilar (sin Vitis, con los ap_int open-source):
//   g++ -std=c++14 -I . -I ../third_party/ap_types/include \
//       -o soc_tb_simple soc_tb_simple.cpp soc_top.cpp && ./soc_tb_simple
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include "soc_top.h"
#include "rv32i_defs.h"
#include "rv32c_defs.h"

using namespace rv32c;                       // i_type/r_type/s_type/b_type/u_type/j_type
namespace A  = rv32i::Funct3_ALU;
namespace M  = rv32i::Funct3_MULDIV;
namespace F7 = rv32i::Funct7_FP;
enum { x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15,
       x16,x17,x18,x19,x20,x21,x22,x23,x24,x25,x26,x27,x28,x29,x30,x31 };

// =====================================================================
//  Ensamblador legible: un método por instrucción, appendea al programa
// =====================================================================
struct P {
    std::vector<uint16_t> prog;
    void w(uint32_t x)  { prog.push_back(x & 0xFFFF); prog.push_back(x >> 16); }
    void h(uint16_t x)  { prog.push_back(x); }                 // comprimida (16b)
    void end()          { prog.push_back(0x0000); }            // fin de programa

    // --- enteros (I) ---
    void li  (int rd,int imm)             { w(i_type(rv32i::Opcode::OP_IMM, rd, A::ADD_SUB, x0, imm)); }
    void addi(int rd,int rs1,int imm)     { w(i_type(rv32i::Opcode::OP_IMM, rd, A::ADD_SUB, rs1, imm)); }
    void andi(int rd,int rs1,int imm)     { w(i_type(rv32i::Opcode::OP_IMM, rd, A::AND, rs1, imm)); }
    void slli(int rd,int rs1,int sh)      { w(i_type(rv32i::Opcode::OP_IMM, rd, A::SLL, rs1, sh)); }
    void add (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::ADD_SUB, rs1, rs2, 0)); }
    void sub (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::ADD_SUB, rs1, rs2, rv32i::Funct7::ALT)); }
    void and_(int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::AND, rs1, rs2, 0)); }
    void or_ (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::OR,  rs1, rs2, 0)); }
    void xor_(int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::XOR, rs1, rs2, 0)); }
    void slt (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, A::SLT, rs1, rs2, 0)); }
    void lui (int rd,int imm20)           { w(u_type(rv32i::Opcode::LUI, rd, imm20)); }

    // --- multiplicación / división (M) ---
    void mul (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, M::MUL,  rs1, rs2, rv32i::Funct7::MULDIV)); }
    void div (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, M::DIV,  rs1, rs2, rv32i::Funct7::MULDIV)); }
    void rem (int rd,int rs1,int rs2)     { w(r_type(rv32i::Opcode::OP, rd, M::REM,  rs1, rs2, rv32i::Funct7::MULDIV)); }

    // --- memoria ---
    void sw  (int rs2,int base,int off)   { w(s_type(rv32i::Opcode::STORE, rv32i::Funct3_STORE::SW, base, rs2, off)); }
    void lw  (int rd, int base,int off)   { w(i_type(rv32i::Opcode::LOAD,  rd, rv32i::Funct3_LOAD::LW, base, off)); }
    void sb  (int rs2,int base,int off)   { w(s_type(rv32i::Opcode::STORE, rv32i::Funct3_STORE::SB, base, rs2, off)); }
    void lbu (int rd, int base,int off)   { w(i_type(rv32i::Opcode::LOAD,  rd, rv32i::Funct3_LOAD::LBU, base, off)); }

    // --- saltos ---
    void beq (int rs1,int rs2,int off)    { w(b_type(rv32i::Funct3_BRANCH::BEQ, rs1, rs2, off)); }
    void bne (int rs1,int rs2,int off)    { w(b_type(rv32i::Funct3_BRANCH::BNE, rs1, rs2, off)); }
    void blt (int rs1,int rs2,int off)    { w(b_type(rv32i::Funct3_BRANCH::BLT, rs1, rs2, off)); }
    void jal (int rd,int off)             { w(j_type(rd, off)); }

    // --- punto flotante (F) ---
    void fcvt_s_w(int fd,int rs1)         { w(r_type(rv32i::Opcode::OP_FP, fd, 0, rs1, rv32i::Rs2_FCVT::W, F7::FCVT_S_W)); }
    void fadd_s (int fd,int f1,int f2)    { w(r_type(rv32i::Opcode::OP_FP, fd, 0, f1, f2, F7::FADD_S)); }
    void fmul_s (int fd,int f1,int f2)    { w(r_type(rv32i::Opcode::OP_FP, fd, 0, f1, f2, F7::FMUL_S)); }
    void fdiv_s (int fd,int f1,int f2)    { w(r_type(rv32i::Opcode::OP_FP, fd, 0, f1, f2, F7::FDIV_S)); }
    void feq_s  (int rd,int f1,int f2)    { w(r_type(rv32i::Opcode::OP_FP, rd, rv32i::Funct3_FCMP::FEQ, f1, f2, F7::FCMP_S)); }

    // --- vectorial (RVV) ---
    // SEW=32, LMUL=1  ->  vl = min(AVL, 4)
    void vsetvli(int rd,int rs1)          { w(((0b010u<<3) << 20) | (rs1<<15) | (0b111u<<12) | (rd<<7) | 0b1010111u); }
    void vle32  (int vd,int base)         { w((1u<<25) | (base<<15) | (0b110u<<12) | (vd<<7) | rv32i::Opcode::LOAD_FP); }
    void vse32  (int vs3,int base)        { w((1u<<25) | (base<<15) | (0b110u<<12) | (vs3<<7) | rv32i::Opcode::STORE_FP); }
    void vadd_vv(int vd,int vs2,int vs1)  { w((0b000000u<<26)|(1u<<25)|(vs2<<20)|(vs1<<15)|(0b000u<<12)|(vd<<7)|0b1010111u); }
    void vmul_vv(int vd,int vs2,int vs1)  { w((0b100101u<<26)|(1u<<25)|(vs2<<20)|(vs1<<15)|(0b010u<<12)|(vd<<7)|0b1010111u); }
    void vmul_vx(int vd,int vs2,int rs1)  { w((0b100101u<<26)|(1u<<25)|(vs2<<20)|(rs1<<15)|(0b110u<<12)|(vd<<7)|0b1010111u); }
};

// =====================================================================
//  Arnés: corre el programa hasta halt y devuelve el estado observado
//  (reconstruido SOLO desde el stream de commit — sin backdoor)
// =====================================================================
struct State {
    uint32_t x[32] = {0};          // banco entero, reconstruido de los commits
    uint32_t f[32] = {0};          // banco flotante (bits IEEE-754)
    ap_uint<32> dmem[OOO_DMEM_WORDS];
    ap_uint<32> vregs[OOO_VEC_REGFILE_LEN];
    int cycles = 0;
    // helpers de lectura
    float     fp(int i)            { float v; uint32_t b=f[i]; std::memcpy(&v,&b,4); return v; }
    uint32_t  vec(int vreg,int el) { return vregs[vreg*OOO_VEC_LANES + el].to_uint(); }  // SEW=32
    uint32_t  mem(uint32_t byte)   { return dmem[byte/4].to_uint(); }
};

static State run(P& p, void (*init_dmem)(ap_uint<32>*) = nullptr) {
    static ap_uint<32> imem[OOO_IMEM_WORDS];
    State st;
    for (int i = 0; i < OOO_IMEM_WORDS; i++) imem[i] = 0;
    for (int i = 0; i < OOO_DMEM_WORDS; i++) st.dmem[i] = 0;
    if (init_dmem) init_dmem(st.dmem);
    for (size_t i = 0; i < p.prog.size(); i++) {           // cargar el programa
        uint32_t wrd = imem[i/2].to_uint();
        if (i % 2 == 0) wrd = (wrd & 0xFFFF0000u) | p.prog[i];
        else            wrd = (wrd & 0x0000FFFFu) | (uint32_t(p.prog[i]) << 16);
        imem[i/2] = wrd;
    }
    // señales del tick
    ap_uint<1> dv; ap_uint<3> dt; ap_uint<32> dp;
    ap_uint<1> a0d,a1d,mdd,fpd,lsd,brd,vcd; ap_uint<3> a0t,a1t,mdt,fpt,lst,brt,vct;
    ap_uint<1> cv,cfp; ap_uint<5> crd; ap_uint<32> cval; ap_uint<1> halted;
    riscv_soc_tick(1, imem, st.dmem, dv,dt,dp, a0d,a0t,a1d,a1t,mdd,mdt,fpd,fpt,
                   lsd,lst,brd,brt,vcd,vct, cv,cfp,crd,cval, st.vregs, halted);
    while (!halted && st.cycles < 4000) {
        st.cycles++;
        riscv_soc_tick(0, imem, st.dmem, dv,dt,dp, a0d,a0t,a1d,a1t,mdd,mdt,fpd,fpt,
                       lsd,lst,brd,brt,vcd,vct, cv,cfp,crd,cval, st.vregs, halted);
        if (cv) {                                          // un commit: actualiza el banco
            if (cfp) st.f[crd.to_uint()] = cval.to_uint();
            else if (crd != 0) st.x[crd.to_uint()] = cval.to_uint();
        }
    }
    return st;
}

// =====================================================================
//  Chequeo legible: nombre + valor obtenido vs esperado
// =====================================================================
static int fails = 0;
static void ck(const char* what, uint32_t got, uint32_t exp) {
    if (got == exp) std::printf("  OK    %-34s = %u\n", what, got);
    else { std::printf("  FAIL  %-34s = %u  (esperado %u)\n", what, got, exp); fails++; }
}
static void ckf(const char* what, float got, float exp) {
    if (got == exp) std::printf("  OK    %-34s = %g\n", what, got);
    else { std::printf("  FAIL  %-34s = %g  (esperado %g)\n", what, got, exp); fails++; }
}
static void title(const char* t) { std::printf("\n── %s ──────────────────────────────\n", t); }

// =====================================================================
//  Los tests: cada función, un programita claro
// =====================================================================
int main() {
    std::printf("Testbench simple del SoC — prueba cada función por separado\n");

    // ---- I: aritmética y lógica enteras ----
    title("Enteros (I)");
    { P p; p.li(x1,5); p.li(x2,10); p.add(x3,x1,x2); p.end();
      ck("add   5 + 10", run(p).x[x3], 15); }
    { P p; p.li(x1,10); p.li(x2,3); p.sub(x3,x1,x2); p.end();
      ck("sub   10 - 3", run(p).x[x3], 7); }
    { P p; p.li(x1,0b1100); p.li(x2,0b1010); p.and_(x3,x1,x2); p.end();
      ck("and   0b1100 & 0b1010", run(p).x[x3], 0b1000); }
    { P p; p.li(x1,0b1100); p.li(x2,0b1010); p.or_(x3,x1,x2); p.end();
      ck("or    0b1100 | 0b1010", run(p).x[x3], 0b1110); }
    { P p; p.li(x1,1); p.slli(x2,x1,4); p.end();
      ck("slli  1 << 4", run(p).x[x2], 16); }
    { P p; p.li(x1,3); p.li(x2,7); p.slt(x3,x1,x2); p.end();
      ck("slt   3 < 7", run(p).x[x3], 1); }
    { P p; p.lui(x1,0x12345); p.end();
      ck("lui   0x12345", run(p).x[x1], 0x12345000u); }

    // ---- M: multiplicación y división ----
    title("Mul / Div (M)");
    { P p; p.li(x1,6); p.li(x2,7); p.mul(x3,x1,x2); p.end();
      ck("mul   6 * 7", run(p).x[x3], 42); }
    { P p; p.li(x1,100); p.li(x2,7); p.div(x3,x1,x2); p.end();
      ck("div   100 / 7", run(p).x[x3], 14); }
    { P p; p.li(x1,100); p.li(x2,7); p.rem(x3,x1,x2); p.end();
      ck("rem   100 % 7", run(p).x[x3], 2); }
    { P p; p.li(x1,5); p.li(x2,0); p.div(x3,x1,x2); p.end();
      ck("div   5 / 0  (= -1, por el ISA)", run(p).x[x3], 0xFFFFFFFFu); }

    // ---- Memoria: load / store ----
    title("Memoria (load / store)");
    { P p; p.li(x1,42); p.sw(x1,x0,64); p.lw(x2,x0,64); p.end();
      ck("sw + lw   guarda 42 y lo relee", run(p).x[x2], 42); }
    { P p; p.li(x1,0xAB); p.sb(x1,x0,80); p.lbu(x2,x0,80); p.end();
      ck("sb + lbu  guarda un byte", run(p).x[x2], 0xAB); }

    // ---- Saltos: branch y jal ----
    title("Saltos (branch / jal)");
    { P p; p.li(x1,5); p.li(x7,0); p.beq(x1,x1,8); p.li(x7,99); p.li(x7,1); p.end();
      ck("beq   tomado salta el 99", run(p).x[x7], 1); }     // x7 = 1, no 99
    { P p; p.li(x1,5); p.li(x2,3); p.li(x7,0); p.beq(x1,x2,8); p.li(x7,7); p.li(x7,8); p.end();
      ck("beq   NO tomado (5≠3)", run(p).x[x7], 8); }        // pasa por ambos addi: 7 luego 8
    { P p; p.jal(x8,8); p.li(x9,88); p.li(x9,1); p.end();
      ck("jal   salta el 88", run(p).x[x9], 1); }

    // ---- F: punto flotante ----
    title("Punto flotante (F)");
    { P p; p.li(x1,3); p.fcvt_s_w(x2/*f2*/,x1); p.li(x3,4); p.fcvt_s_w(x4/*f4*/,x3);
      p.fadd_s(x5/*f5*/,x2,x4); p.end();
      ckf("fadd  3.0 + 4.0", run(p).fp(5), 7.0f); }
    { P p; p.li(x1,3); p.fcvt_s_w(x2,x1); p.li(x3,4); p.fcvt_s_w(x4,x3);
      p.fmul_s(x5,x2,x4); p.end();
      ckf("fmul  3.0 * 4.0", run(p).fp(5), 12.0f); }
    { P p; p.li(x1,12); p.fcvt_s_w(x2,x1); p.li(x3,4); p.fcvt_s_w(x4,x3);
      p.fdiv_s(x5,x2,x4); p.end();
      ckf("fdiv  12.0 / 4.0", run(p).fp(5), 3.0f); }
    { P p; p.li(x1,5); p.fcvt_s_w(x2,x1); p.feq_s(x6,x2,x2); p.end();
      ck("feq   5.0 == 5.0", run(p).x[x6], 1); }

    // ---- C: instrucciones comprimidas (16 bits) ----
    title("Comprimidas (C)");
    { P p; p.h(0x47A5); p.end();                              // c.li x15,9
      ck("c.li  x15 = 9  (instr. de 16 bits)", run(p).x[x15], 9); }
    { P p; p.h(0x47A5); p.h(0x078D); p.end();                 // c.li x15,9 ; c.addi x15,3
      ck("c.addi  9 + 3  (dos comprimidas)", run(p).x[x15], 12); }

    // ---- RVV: coprocesamiento vectorial ----
    title("Vectorial (RVV)");
    // v1 = {10,20,30,40} desde memoria; v2 = {1,2,3,4}; v3 = v1 + v2
    auto load_vecs = [](ap_uint<32>* d){
        d[0]=10; d[1]=20; d[2]=30; d[3]=40;      // v1 (byte 0)
        d[8]=1;  d[9]=2;  d[10]=3; d[11]=4;      // v2 (byte 32)
    };
    { P p; p.vsetvli(x19,x0);                      // vl = 4
      p.vle32(1,x0);                               // v1 = mem[0..3]  = {10,20,30,40}
      p.li(x13,32); p.vle32(2,x13);                // v2 = mem[32..]  = {1,2,3,4}
      p.vadd_vv(3,2,1);                            // v3 = v2 + v1
      p.end();
      State s = run(p, load_vecs);
      ck("vadd.vv v3[0] = 10+1", s.vec(3,0), 11);
      ck("vadd.vv v3[3] = 40+4", s.vec(3,3), 44); }
    { P p; p.vsetvli(x19,x0);
      p.vle32(1,x0);                               // v1 = {10,20,30,40}
      p.li(x5,3); p.vmul_vx(4,1,x5);               // v4 = v1 * 3  (escalar)
      p.end();
      State s = run(p, load_vecs);
      ck("vmul.vx v4[0] = 10*3", s.vec(4,0), 30);
      ck("vmul.vx v4[2] = 30*3", s.vec(4,2), 90); }
    { P p; p.vsetvli(x19,x0);
      p.vle32(1,x0);                               // v1 = {10,20,30,40}
      p.li(x6,64); p.vse32(1,x6);                  // guarda v1 en mem[64..]
      p.end();
      State s = run(p, load_vecs);
      ck("vse32   v1[0] -> mem[64]", s.mem(64), 10); }

    // ---- resumen ----
    std::printf("\n================================================\n");
    if (fails == 0) std::printf("  Todas las funciones pasaron.\n");
    else            std::printf("  %d check(s) fallaron.\n", fails);
    std::printf("================================================\n");
    return fails == 0 ? 0 : 1;
}

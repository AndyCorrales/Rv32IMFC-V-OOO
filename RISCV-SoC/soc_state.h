#ifndef SOC_STATE_H
#define SOC_STATE_H

// estado del procesador (en el rtl son registros). todo static: el diseno es
// una sola unidad de traduccion, asi que las etapas lo ven directo sin params.
#include "soc_top.h"
#include "rv32i_defs.h"
#include "soc_config.h"
#include "rvv_encoding.h"

// ================= estado persistente (registros en RTL) =================

struct RatEntry {
    bool       has_tag;
    ap_uint<3> tag;
};

// entrada del rob: resultado en vuelo + causa de trap + estado del branch especulado
struct RobEntry {
    bool        valid, ready, is_store, dest_is_fp;
    bool        takes_trap;   // causa en `cause`: 2 ilegal, 3 ebreak, 11/8 ecall
    bool        is_mret;
    bool        is_branch, pred_taken, br_taken_real, br_mispred;  // especulacion tage
    ap_uint<16> ghr_snap;     // historia global al predecir (update + reparar ghr)
    ap_uint<32> br_target, pc;
    ap_uint<5>  cause, dest;
    ap_uint<32> value;        // para F: bits IEEE-754 crudos
    ap_uint<32> addr, sdata;  // solo stores
    ap_uint<3>  mem_f3;
};

struct Operand {
    bool        ready;
    ap_uint<32> val;
    ap_uint<3>  tag; // tag ROB esperado cuando !ready
};

struct AluRs {
    bool        busy, executing;
    ap_uint<1>  remaining;
    ap_uint<3>  f3;
    bool        alt;      // SUB/SRA(I)
    ap_uint<3>  rob_tag;
    Operand     s1, s2;
};

struct MdRs {
    bool        busy, executing;
    ap_uint<4>  remaining;
    ap_uint<3>  f3;
    ap_uint<3>  rob_tag;
    Operand     s1, s2;
};

struct LsuRs {
    bool        busy;
    bool        is_load;
    ap_uint<3>  f3;
    ap_uint<3>  rob_tag;
    int32_t     imm;
    Operand     s1;  // base de direccion
    Operand     s2;  // dato del store (loads: ready=true)
};

struct BrRs {
    bool        busy, executing;
    ap_uint<1>  remaining;
    bool        is_jalr;
    ap_uint<3>  f3;
    ap_uint<3>  rob_tag;
    ap_uint<32> br_pc;
    ap_uint<3>  size;   // 2 o 4 (extension C): link y fall-through = pc+size
    int32_t     imm;
    Operand     s1, s2;
};

struct FpuRs {
    bool        busy, executing;
    ap_uint<4>  remaining;
    ap_uint<3>  r4op;   // 0=OP_FP; 1..4 = FMADD/FMSUB/FNMSUB/FNMADD
    ap_uint<7>  f7;
    ap_uint<3>  f3;     // selector dentro del grupo (sgnj/minmax/cmp/mv-class)
    ap_uint<5>  rs2f;   // campo rs2 crudo (selector W/WU de FCVT)
    ap_uint<3>  rob_tag;
    Operand     s1, s2, s3; // s3: tercer operando de la familia R4
};

// Unidad de sistema (bare-metal, items 2-5): una sola reservation
// station para las instrucciones CSR, que ejecuta SOLO en la cabeza del
// ROB (como un load) -- asi el acceso a CSR es en orden de programa y ve
// el estado arquitectonico correcto, sin necesitar el mecanismo de traps
// precisas (item 6, fuera de alcance). ECALL/EBREAK no pasan por aca:
// se marcan en el ROB (is_ecall) y detienen el programa al retirarse.
struct SysRs {
    bool        busy;
    ap_uint<3>  f3;        // CSRRW/S/C y variantes con inmediato
    ap_uint<12> csr_addr;  // instr[31:20]
    ap_uint<3>  rob_tag;
    Operand     s1;        // valor de rs1 (CSRRW/S/C) o zimm (variantes I, ready=true)
};

// rs de la unidad vectorial: una sola, serializa las vectoriales entre si (sin
// vrat), pero se solapan con las escalares independientes.
struct VecRs {
    bool        busy, executing;
    ap_uint<3>  remaining;  // aguanta la latencia max del coproc (vmul=4); con 2b se envolvia a 0
    bool        is_load, is_store, is_arith;
    ap_uint<5>  vd_or_vs3, vs1, vs2;
    ap_uint<6>  funct6;
    ap_uint<3>  funct3;
    bool        vm;         // 1 sin mascara; 0 predicado por v0
    ap_uint<3>  rob_tag;
    Operand     s1, s2;     // base y paso (banco entero, solo memoria/strided)
    // direccionamiento de memoria
    ap_uint<2>  mop;        // 00 unit-stride, 01/11 indexado, 10 strided
    ap_uint<3>  nf, fld;
    ap_uint<5>  elem;       // elemento en curso (la vlsu mueve uno por ciclo)
    ap_uint<2>  lsmode;
    ap_uint<3>  idx_b;
    // vl/vtype capturados en el dispatch (en orden, sin especular -> preciso)
    ap_uint<5>  vl;
    ap_uint<3>  sew_b;      // ancho de elemento en bytes (1/2/4)
    ap_uint<4>  vcat;       // vcat_*: que destino escribe
    ap_uint<5>  vstart, vs1_field;
};

static ap_uint<32> regfile[32];
static ap_uint<32> fregs[32];   // banco F como bits IEEE-754 crudos
static RatEntry    rat[32];
static RatEntry    frat[32];    // RAT del banco flotante (mismo ROB)
static RobEntry    rob[ROB_SZ];
static ap_uint<3>  rob_head, rob_tail;
static ap_uint<4>  rob_count;
static AluRs       alu_rs[N_ALU];
static MdRs        md_rs;
static FpuRs       fpu_rs;
static LsuRs       lsu_rs;
static BrRs        br_rs;
static VecRs       vec_rs;
// viq: cola del coprocesador (fifo 4). el dispatch encola y sigue; el
// coprocesador saca de la cabeza. es el desacople escalar<->vectorial.
static const int  VIQ_SZ = 4;
static VecRs      viq[VIQ_SZ];
static ap_uint<2> viq_head, viq_tail;
static ap_uint<3> viq_count;
static void viq_reset() { viq_head = 0; viq_tail = 0; viq_count = 0; }
static bool viq_full()  { return viq_count == VIQ_SZ; }
static void viq_push(const VecRs& v) {
    viq[viq_tail] = v;
    viq_tail = viq_tail + 1;
    viq_count = viq_count + 1;
}
static SysRs       sys_rs;
static ap_uint<32> vregs[OOO_VEC_REGFILE_LEN]; // banco vectorial, sin renombrar
// csrs de modo maquina (subset minimo bare-metal): solo almacenamiento, sin warl.
static ap_uint<32> csr_mstatus, csr_mie, csr_mtvec, csr_mscratch;
static ap_uint<32> csr_mepc, csr_mcause, csr_mtval, csr_mip;
// csrs vectoriales: vtype/vl (vsetvli...) y vstart
static ap_uint<32> csr_vtype, csr_vl;
static ap_uint<32> csr_vstart;
static bool        ecall_halt;
// interrupciones (timer) y modos de privilegio
static ap_uint<32> csr_mtime;    // contador libre
static ap_uint<32> csr_mtimecmp; // mtime >= mtimecmp -> interrupcion
static ap_uint<2>  cur_priv;     // 3 M-mode, 0 U-mode
static ap_uint<32> fetch_pc;
static bool        fetch_stalled; // esperando jalr (los branches ya no frenan: tage)
// branches en vuelo: mientras haya, lo que escribe estado sin poder deshacerse
// (vset*, vregs) espera; las unidades renombradas si especulan (el flush las borra).
static ap_uint<4>  branch_pending;
static ap_uint<32> stat_branches, stat_mispredicts;  // observabilidad del predictor
static bool        fetch_done;    // se encontro la palabra 0 (fin de programa)


#endif // SOC_STATE_H

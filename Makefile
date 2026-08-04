# =====================================================================
# Makefile — version_final_martes
# Compila y ejecuta las dos pistas del SoC:
#   - HLS  : C-simulation con g++ (usa ap_int open-source, NO requiere Vitis)
#   - TLM  : SystemC (requiere libsystemc instalada)
#
#   make check            -> corre todo (HLS + TLM + paridad)
#   make check-soc        -> solo la pista HLS
#   make check-soc-tlm    -> solo la pista TLM
#   make check-axpy-parity-> compara HLS vs TLM ciclo a ciclo
#   make clean
# =====================================================================
CXX     := g++
SOCHLS  := RISCV-SoC
SOCTLM  := RISCV-SoC-TLM
AP_INC  := -I third_party/ap_types/include
HLS_STD := -std=c++14 -O2

.PHONY: all check check-soc check-soc-tlm check-axpy-parity clean

all: check
check: check-soc check-soc-tlm check-axpy-parity

# ---------- pista HLS (g++, sin Vitis) ----------
$(SOCHLS)/soc_tb: $(SOCHLS)/soc_tb.cpp $(SOCHLS)/soc_top.cpp
	$(CXX) $(HLS_STD) -I $(SOCHLS) $(AP_INC) -o $@ $^

$(SOCHLS)/axpy_soc_tb: $(SOCHLS)/axpy_soc_tb.cpp $(SOCHLS)/soc_top.cpp
	$(CXX) $(HLS_STD) -I $(SOCHLS) $(AP_INC) -o $@ $^

$(SOCHLS)/soc_tb_simple: $(SOCHLS)/soc_tb_simple.cpp $(SOCHLS)/soc_top.cpp
	$(CXX) $(HLS_STD) -I $(SOCHLS) $(AP_INC) -o $@ $^

check-soc: $(SOCHLS)/soc_tb $(SOCHLS)/soc_tb_simple $(SOCHLS)/axpy_soc_tb
	@echo "=== [HLS] batería de verificación ==="
	@$(SOCHLS)/soc_tb
	@echo "=== [HLS] testbench legible (mnemónicos) ==="
	@$(SOCHLS)/soc_tb_simple
	@echo "=== [HLS] AXPY (IPC / speedup) ==="
	@$(SOCHLS)/axpy_soc_tb | grep -E "IPC|speedup|escalar|vectorial" || true

# ---------- pista TLM (SystemC) ----------
$(SOCTLM)/riscv_soc_sim: $(SOCTLM)/src/main.cpp
	$(CXX) -std=c++17 -O2 -I $(SOCTLM)/src -o $@ $< -lsystemc -lpthread

$(SOCTLM)/axpy_soc_tlm: $(SOCTLM)/src/axpy_soc_tlm.cpp
	$(CXX) -std=c++17 -O2 -I $(SOCTLM)/src -o $@ $< -lsystemc -lpthread

check-soc-tlm: $(SOCTLM)/riscv_soc_sim $(SOCTLM)/axpy_soc_tlm
	@echo "=== [TLM] batería de verificación ==="
	@$(SOCTLM)/riscv_soc_sim
	@echo "=== [TLM] AXPY (IPC / speedup) ==="
	@$(SOCTLM)/axpy_soc_tlm | grep -E "IPC|speedup" || true

# ---------- paridad de ciclos HLS vs TLM ----------
check-axpy-parity: $(SOCHLS)/axpy_soc_tb $(SOCTLM)/axpy_soc_tlm
	@echo "=== paridad de ciclos HLS vs TLM (AXPY) ==="
	@$(SOCHLS)/axpy_soc_tb  | grep -E "^escalar|^vectorial" > /tmp/par_h.txt
	@$(SOCTLM)/axpy_soc_tlm | grep -E "^escalar|^vectorial" > /tmp/par_t.txt
	@diff /tmp/par_h.txt /tmp/par_t.txt && echo "PASS: paridad exacta." || \
	    { echo "FAIL: las pistas divergieron." ; exit 1 ; }

clean:
	rm -f $(SOCHLS)/soc_tb $(SOCHLS)/soc_tb_simple $(SOCHLS)/axpy_soc_tb \
	      $(SOCTLM)/riscv_soc_sim $(SOCTLM)/axpy_soc_tlm

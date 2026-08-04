# How to run

## Requisitos
- `g++` (C++14/17) y **SystemC 2.3.x** → para simular.
- **Vitis HLS** y **Vivado** 2024.x → solo para síntesis/P&R (opcional).

---

## HLS (pista sintetizable)

**Simular (no requiere Vitis):**
```bash
make check-soc
```

**Sintetizar a RTL + área/potencia/timing (requiere Vitis + Vivado):**
```bash
cd RISCV-SoC
vitis_hls -f run_hls.tcl                          # C-sim + csynth (genera el RTL)
source <ruta-Vivado>/settings64.sh
vivado -mode batch -source run_vivado.tcl          # síntesis + place & route
# resultados en RISCV-SoC/reports/ : util_impl (área), power_impl, timing_impl
```

---

## TLM (pista de verificación funcional)

**Simular (requiere SystemC):**
```bash
make check-soc-tlm
```

---

## Todo junto (HLS + TLM + paridad de ciclos)
```bash
make check
```

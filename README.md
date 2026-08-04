# SoC RISC-V RV32IMFC + RVV, fuera de orden, bare-metal

Procesador **RISC-V RV32IMFC** de **ejecución fuera de orden** (Tomasulo) con un
**coprocesador vectorial RVV** desacoplado, predicción de saltos **TAGE** e
**interconnect AXI con árbitro**, capaz de ejecutar software **bare-metal**.

El mismo diseño se describe **dos veces** y ambas se validan entre sí ciclo a
ciclo:

- **`RISCV-SoC/`** — descripción **sintetizable en HLS** (C++ para Vitis HLS).
  Se simula con `g++` usando los tipos `ap_int` de código abierto (**sin Vitis**).
- **`RISCV-SoC-TLM/`** — descripción en **SystemC / TLM-2.0** (rápida de simular).

Ver la arquitectura en **[`nueva.png`](nueva.png)** y el alcance/limitaciones en
**[`LIMITACIONES.md`](LIMITACIONES.md)**.

---

## Estructura

```
.
├── README.md            · este archivo
├── LIMITACIONES.md      · alcance y limitaciones del diseño
├── nueva.png            · diagrama de bloques de la arquitectura
├── Makefile             · compila y corre todo
├── third_party/ap_types · tipos ap_int open-source (dependencia del g++ del HLS)
├── RISCV-SoC/           · pista HLS (fuentes + testbenches + scripts + baremetal)
└── RISCV-SoC-TLM/       · pista TLM (SystemC)
```

## Dependencias

| Para… | Necesitás |
|---|---|
| Correr la pista **HLS** (C-sim) | `g++` (C++14) — nada más; `ap_int` ya viene en `third_party/` |
| Correr la pista **TLM** | `g++` (C++17) + **SystemC 2.3.x** (`libsystemc`) |
| **Sintetizar** el HLS a RTL (opcional) | **Vitis HLS** 2024.x |
| **Estimar recursos/potencia** (opcional) | **Vivado** 2024.x |
| **Regenerar** los binarios bare-metal (opcional) | RISC-V GNU Toolchain (`riscv64-unknown-elf-gcc`) |

> Para **correr y verificar** el diseño solo hacen falta `g++` y SystemC. Vitis
> y Vivado son opcionales (síntesis y estimación de área/potencia). El toolchain
> RISC-V solo se necesita si querés recompilar los programas bare-metal; los ELF
> ya vienen embebidos en los headers `*_elf.h`.

## Cómo correr todo (desde cero)

```bash
make check              # HLS + TLM + paridad de ciclos
```

O por partes:

```bash
make check-soc          # pista HLS: batería de checks + testbench legible + AXPY
make check-soc-tlm      # pista TLM: batería de checks + AXPY
make check-axpy-parity  # compara HLS vs TLM ciclo a ciclo (debe dar PASS)
make clean              # borra los binarios compilados
```

### Síntesis y potencia (opcional, requiere Vitis/Vivado)

```bash
cd RISCV-SoC
vitis_hls -f run_hls.tcl        # C-sim + csynth (genera el RTL)
source <ruta-a-Vivado>/settings64.sh
vivado -mode batch -source run_vivado.tcl   # síntesis + place & route completo
# -> reportes en RISCV-SoC/reports/: util_impl (área), power_impl, timing_impl
```

## Detalle de cada pista

- **`RISCV-SoC/`** — el DUT es la función `riscv_soc_tick` en `soc_top.cpp`; una
  llamada = un ciclo de reloj, con el estado en variables `static`. Los
  testbenches (`soc_tb.cpp`, `soc_tb_simple.cpp`, `axpy_soc_tb.cpp`) reconstruyen
  el estado solo desde el flujo de *commit*. Ver [`RISCV-SoC/README.md`](RISCV-SoC/README.md).
- **`RISCV-SoC-TLM/`** — módulos SystemC (`ProcessorOOO`, `Bus`, `Memory`,
  `Uart`) sobre un bus TLM. Ver [`RISCV-SoC-TLM/README.md`](RISCV-SoC-TLM/README.md).

Ambas ejecutan **los mismos binarios** sobre el **mismo mapa de direcciones**, y
`make check-axpy-parity` verifica que producen exactamente los mismos ciclos.

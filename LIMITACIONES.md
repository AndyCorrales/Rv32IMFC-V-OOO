# Limitaciones conocidas

Limitaciones de alcance del SoC RV32IMFC+RVV fuera de orden de esta carpeta
(pistas `RISCV-SoC/` en HLS y `RISCV-SoC-TLM/` en SystemC). La mayoría son
**decisiones de diseño deliberadas** para acotar el trabajo y mantener la
verificación tratable, no bugs. Donde aplica se indica la justificación y qué
haría falta para levantarlas.

---

## Núcleo escalar

- **Fetch de una instrucción por ciclo.** El frente no es superescalar; se
  busca un *halfword* por ciclo y se expande la comprimida (extensión C) a
  cualquier alineación par. Levantarlo requiere un fetch de 2 vías + un buffer
  de decodificación ancho.
- **Unidades funcionales multiciclo, no pipelinizadas.** Cada unidad (MUL/DIV,
  FPU, …) procesa **una** instrucción a la vez con un contador de latencia
  (`remaining`); no acepta una nueva cada ciclo. Un multiplicador pipelinizado
  daría *throughput* 1 aunque la latencia sea >1, a costa de más área.
- **Sin CSRs de punto flotante (`frm`/`fflags`).** El modo de redondeo es fijo
  a *round-to-nearest-even*; no se exponen banderas de excepción de FP.

## Memoria

- **Sin cachés.** La memoria de instrucciones (I-TCM) y la de datos son **BRAM
  on-chip**; no hay jerarquía de caché ni memoria externa (DDR/HBM). Es
  adecuado para el nicho *edge*/bare-metal, pero limita el tamaño de los
  programas y datos.
- **Memoria de datos de un puerto, arbitrada.** Un **árbitro** concede **un
  acceso por ciclo** con prioridad fija (retiro > VLSU > LSU escalar); el
  perdedor reintenta al ciclo siguiente. Por el mismo motivo la **VLSU mueve un
  elemento por ciclo** — modela un puerto de memoria real, no accesos
  simultáneos.
- **Stores parciales (`SB`/`SH`) hacen *read-modify-write*** de la palabra (la
  memoria es por palabras de 32 bits), un ciclo extra por store parcial. Los
  *loads* con desplazamiento dentro de la palabra sí se resuelven con
  *shift*/máscara.

## Coprocesador vectorial (RVV)

- **Perfil entero `Zve32x`: sin punto flotante vectorial.** Solo instrucciones
  vectoriales de enteros (SEW 8/16/32).
- **`VLEN` = 128 bits fijo, `LMUL` = 1.** No es parametrizable en tiempo de
  ejecución más allá de `SEW`; `VLMAX` = 4 para SEW=32. Otra configuración
  activa `vill` y deja `vl` = 0.
- **Una sola estación de reserva vectorial.** Las instrucciones vectoriales se
  **serializan entre sí** en orden de programa (banco vectorial **sin
  renombrar**, no hay VRAT), aunque **sí se solapan** libremente con las
  escalares independientes. Esto explica que el IPC vectorial sea menor que el
  escalar, y es el principal punto a levantar para subir el *throughput*.
- **`vle`/`vse` se resuelven en la cabeza del ROB** (como un *load* escalar,
  pero también para el *store*), más conservador que el store escalar para no
  ampliar el ROB a 128 bits.

## Flujo de implementación

- **El flujo llega hasta síntesis, no hasta *place & route* completo.** Las
  cifras de frecuencia y recursos provienen de `csynth` (Vitis HLS) y de la
  síntesis lógica de Vivado (`report_power.tcl`), **no** de una implementación
  post-P&R. La estimación de **potencia es *vectorless*** (tasa de conmutación
  por defecto, nivel de confianza *Medium*).
- **Ejecución en simulación/emulación.** La verificación se hace en C-sim (HLS)
  y SystemC (TLM); no incluye la ejecución del *bitstream* sobre la placa.

## Lo que NO es una limitación (aclaraciones)

- Las **excepciones e interrupciones son precisas y reanudables** (traps con
  `mepc`/`mcause`, `MRET`, interrupción de *timer*), gracias al retiro en orden
  del ROB.
- El **coprocesamiento es real**: el escalar avanza fuera de orden mientras el
  coprocesador ejecuta instrucciones vectoriales *in-flight*.
- La **corrección** está verificada por comparación **ciclo a ciclo** entre las
  pistas HLS y TLM (`make check-axpy-parity`).

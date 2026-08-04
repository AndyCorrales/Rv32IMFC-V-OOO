# ============================================================
# run_vivado.tcl — Flujo COMPLETO de Vivado a partir del RTL de
# Vitis HLS: síntesis + place & route (out-of-context), con reportes
# post-implementación de utilización (área), potencia y timing.
#
# Requisito: haber corrido antes csynth (vitis_hls -f run_hls.tcl),
# que deja el RTL en riscv_soc_proj/solution1/syn/verilog/.
#
# Uso (desde RISCV-SoC/):
#   source <ruta-a-Vivado>/Vivado/2024.x/settings64.sh
#   vivado -mode batch -notrace -source run_vivado.tcl
#
# Reportes en reports/:
#   util_impl.rpt   · utilización / ÁREA (LUT, FF, BRAM, DSP) post-P&R
#   power_impl.rpt  · potencia post-P&R
#   timing_impl.rpt · timing (WNS, Fmax) post-P&R
#   util_hier.rpt   · área por jerarquía de módulos
# ============================================================

set part     xck26-sfvc784-2LV-c                          ;# Kria KV260 (ZU5EV)
set top      riscv_soc_tick
set clk_port ap_clk
set clk_ns   10.0                                         ;# 100 MHz objetivo (7.407 -> 135 MHz)
set toggle   12.5                                         ;# % conmutacion (vectorless)
set rtl_dir  [pwd]/riscv_soc_proj/solution1/syn/verilog
set outdir   [pwd]/reports
file mkdir $outdir

# ---- proyecto en memoria ----
create_project -in_memory -part $part
set_property target_language Verilog [current_project]

# ---- .dat de init de memoria (para $readmemh, relativo al cwd) ----
foreach f [glob -nocomplain $rtl_dir/*.dat] { file copy -force $f [pwd] }

# ---- 1. generar los IP de punto flotante que instancia HLS ----
foreach iptcl [glob -nocomplain $rtl_dir/*_ip.tcl] { source $iptcl }
if {[llength [get_ips -quiet]] > 0} { generate_target synthesis [get_ips] }

# ---- 2. leer el RTL (wrappers HLS) ----
read_verilog [glob $rtl_dir/*.v]

# ============================================================
#   SÍNTESIS
# ============================================================
synth_design -top $top -part $part -mode out_of_context
create_clock -name $clk_port -period $clk_ns [get_ports $clk_port]
write_checkpoint -force $outdir/post_synth.dcp
report_utilization -file $outdir/util_synth.rpt

# ============================================================
#   IMPLEMENTACIÓN (place & route)
# ============================================================
opt_design
place_design
phys_opt_design
route_design

# ============================================================
#   REPORTES post-P&R
# ============================================================
set_switching_activity -default_toggle_rate $toggle
report_utilization        -file $outdir/util_impl.rpt
report_utilization -hierarchical -file $outdir/util_hier.rpt
report_timing_summary     -file $outdir/timing_impl.rpt
report_power              -file $outdir/power_impl.rpt
write_checkpoint -force   $outdir/post_route.dcp

# ---- resumen a consola ----
set wns [get_property SLACK [get_timing_paths -delay_type max -max_paths 1]]
puts "============================================================"
puts " FLUJO COMPLETO OK. Reportes en: $outdir/"
puts "   Área/util  -> util_impl.rpt   (y util_hier.rpt por módulo)"
puts "   Potencia   -> power_impl.rpt"
puts "   Timing     -> timing_impl.rpt   (WNS = $wns ns)"
puts "============================================================"

# Ф0 sims: make -> iss (fast ISS) and obj_$(CPU)/tb_soc (Verilator, CPU=VexRiscv|VexRiscv_Lite)
CPU ?= VexRiscv
OBJ := obj_$(CPU)

all: iss $(OBJ)/tb_soc

iss: iss.cpp
	g++ -O2 -march=native -o $@ $<

$(OBJ)/tb_soc: tb_soc.cpp ../rtl/vexriscv/$(CPU).v
	verilator --cc --exe --build -j 8 -O3 -Wno-fatal -Wno-lint -Wno-style \
	  --top-module VexRiscv --prefix Vcpu -Mdir $(OBJ) -o tb_soc \
	  -CFLAGS "-O2" -MAKEFLAGS "OPT_FAST=-O3 OPT_SLOW=-O1" ../rtl/vexriscv/$(CPU).v tb_soc.cpp >$(OBJ).log 2>&1 || { tail -20 $(OBJ).log; exit 1; }

clean:
	rm -rf iss obj_* obj_*.log
.PHONY: all clean

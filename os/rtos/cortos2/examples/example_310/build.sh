#!/usr/bin/env bash

# build the project

root_dir="$(pwd)"

cortos2 build "$@";

init_s="cortos_build/cortos_src/init.s"
if [[ -f "$init_s" ]]; then
	if ! rg -q "ensure PIL=0" "$init_s"; then
		awk '
		{print}
		$0 ~ /wr %l0, %psr/ && !done {
			print "  ! ensure PIL=0 (allow all interrupt levels)"
			print "  rd %psr, %l0"
			print "  andn %l0, 0xF00, %l0"
			print "  wr %l0, 0, %psr"
			print "  nop"
			print "  nop"
			print "  nop"
			done=1
		}
		' "$init_s" > "${init_s}.tmp" && mv "${init_s}.tmp" "$init_s"
	fi

	# Ensure all trap windows have a valid stack pointer (use thread 0 stack top).
	stack_addr="$(awk '/set 0x[0-9a-fA-F]+, %sp/ {print $2; exit}' "$init_s")"
	stack_addr="${stack_addr%,}"
	if [[ -n "$stack_addr" ]]; then
		# Use the local clear-stack routine (so our SP initialization is used).
		sed -i 's/call clear_stack_pointers/call __cortos_clear_stack_pointers/' "$init_s"
		awk -v stack_addr="$stack_addr" '
		$1 == "__cortos_clear_stack_pointers:" {in=1}
		in && $1 == "mov" && $2 == "%g0," && $3 == "%sp" {
			print "\tset " stack_addr ", %sp"
			next
		}
		{print}
		' "$init_s" > "${init_s}.tmp" && mv "${init_s}.tmp" "$init_s"
	fi

	# Force WIM=0 (all windows valid) to avoid invalid-window traps during ISR save.
	# Replace the WIM init value under COROTS_WIMSET.
	sed -i 's/set 0x1, %l0   ! window 0 is marked invalid. We start at window 0./set 0x0, %l0   ! all windows valid (debug)/' "$init_s"
fi

# Prepare a no-debug libgcc to avoid DWARF v5 (GDB 7.6.2 can't read it)
if [[ -z "${AJIT_LIBGCC_INSTALL_DIR:-}" ]]; then
	ajit_env_path="$root_dir/../../../../../ajit_env"
	if [[ -f "$ajit_env_path" ]]; then
		# shellcheck disable=SC1090
		source "$ajit_env_path"
	fi
fi
if [[ -z "${AJIT_LIBGCC_INSTALL_DIR:-}" && -f /usr/lib/gcc/sparc64-linux-gnu/12/32/libgcc.a ]]; then
	AJIT_LIBGCC_INSTALL_DIR="/usr/lib/gcc/sparc64-linux-gnu/12/32"
fi

libgcc_src="${AJIT_LIBGCC_INSTALL_DIR}/libgcc.a"
libgcc_dst_dir="${root_dir}/cortos_build/libgcc_nodbg"
objcopy_cmd="${AJIT_PROJECT_CROSS_COMPILER:-sparc64-linux-gnu}-objcopy"
ar_cmd="${AJIT_PROJECT_CROSS_COMPILER:-sparc64-linux-gnu}-ar"
ranlib_cmd="${AJIT_PROJECT_CROSS_COMPILER:-sparc64-linux-gnu}-ranlib"
if [[ -f "$libgcc_src" ]]; then
	mkdir -p "$libgcc_dst_dir"
	ln -sfn "${AJIT_LIBGCC_INSTALL_DIR}/include" "$libgcc_dst_dir/include"
	# Strip debug info from each object in libgcc.a to avoid DWARF5.
	tmp_dir="$(mktemp -d)"
	(
		cd "$tmp_dir"
		"$ar_cmd" x "$libgcc_src"
		for obj in *.o; do
			"$objcopy_cmd" --strip-debug "$obj"
		done
		"$ar_cmd" rcs "$libgcc_dst_dir/libgcc.a" *.o
		"$ranlib_cmd" "$libgcc_dst_dir/libgcc.a" || true
	)
	rm -rf "$tmp_dir"
fi

# Ensure GDB server is enabled in run script only if ENABLE_GDB=1
run_cmodel="cortos_build/run_cmodel.sh"
if [[ -f "$run_cmodel" ]]; then
	chmod +x "$run_cmodel"
	if [[ "${ENABLE_GDB:-0}" == "1" ]]; then
		if ! rg -q "gpb_init| -g " "$run_cmodel"; then
			awk '
			{print}
			$0 ~ /ajit_C_system_model/ && !done {
				print "  -g \\"
				print "  -p 9999 \\"
				done=1
			}
			' "$run_cmodel" > "${run_cmodel}.tmp" && mv "${run_cmodel}.tmp" "$run_cmodel"
		fi
	else
		# Remove any existing -g/-p flags if present
		sed -i 's/ -g -p [0-9][0-9]* //g' "$run_cmodel"
	fi
fi

# Ensure serial init doesn't clear RX interrupt enable
dev_c="cortos_build/cortos_src/cortos_devices.c"
if [[ -f "$dev_c" ]]; then
	sed -i 's/TX_ENABLE | RX_ENABLE)/TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE)/' "$dev_c"
fi

# Patch trap table locally so HW_trap_0x07, HW_trap_0x09, and SW_trap_0x80 go to local trap entries (avoid ta 0).
trap_s="cortos_build/cortos_src/trap_handlers_for_rtos.s"
if [[ -f "$trap_s" ]]; then
	awk '
	$1 == "HW_trap_0x07:" {
		print "HW_trap_0x07:"
		print "\tba local_trap_entry_hw; nop; nop; nop;"
		next
	}
	$1 == "HW_trap_0x09:" {
		print "HW_trap_0x09:"
		print "\tba local_trap_entry_hw; nop; nop; nop;"
		next
	}
	$1 == "SW_trap_0x80:" {
		print "SW_trap_0x80:"
		print "\tba local_trap_entry_sw; nop; nop; nop;"
		next
	}
	{print}
	' "$trap_s" > "${trap_s}.tmp" && mv "${trap_s}.tmp" "$trap_s"
fi

# Local trap entry wrappers to set a valid stack, print TT, then jump into ISR.
local_trap_s="cortos_build/cortos_src/local_trap_entry.s"
stack_addr="$(awk '/set 0x[0-9a-fA-F]+, %sp/ {print $2; exit}' "$init_s")"
stack_addr="${stack_addr%,}"
if [[ -n "$stack_addr" ]]; then
	cat > "$local_trap_s" <<EOF
	.section .rodata
hex_digits:
	.ascii "0123456789ABCDEF"

	.section .text
	.align 4
	.global local_trap_entry_hw
	.global local_trap_entry_sw
local_trap_entry_hw:
	set $stack_addr, %g1
	mov %g1, %sp
	clr %fp
	rd %tbr, %l0
	srl %l0, 4, %l1
	and %l1, 0xff, %l1
	mov 'T', %o0
	call ajit_shim_uart_putc
	nop
	mov '=', %o0
	call ajit_shim_uart_putc
	nop
	set hex_digits, %l3
	srl %l1, 4, %l2
	and %l2, 0xf, %l2
	ldub [%l3 + %l2], %o0
	call ajit_shim_uart_putc
	nop
	and %l1, 0xf, %l2
	ldub [%l3 + %l2], %o0
	call ajit_shim_uart_putc
	nop
	mov '\\n', %o0
	call ajit_shim_uart_putc
	nop
	rd %psr, %l0
	rd %tbr, %l3
	ba generic_vectored_isr
	nop

local_trap_entry_sw:
	set $stack_addr, %g1
	mov %g1, %sp
	clr %fp
	rd %tbr, %l0
	srl %l0, 4, %l1
	and %l1, 0xff, %l1
	mov 'T', %o0
	call ajit_shim_uart_putc
	nop
	mov '=', %o0
	call ajit_shim_uart_putc
	nop
	set hex_digits, %l3
	srl %l1, 4, %l2
	and %l2, 0xf, %l2
	ldub [%l3 + %l2], %o0
	call ajit_shim_uart_putc
	nop
	and %l1, 0xf, %l2
	ldub [%l3 + %l2], %o0
	call ajit_shim_uart_putc
	nop
	mov '\\n', %o0
	call ajit_shim_uart_putc
	nop
	rd %psr, %l0
	rd %tbr, %l3
	ba generic_vectored_sw_trap
	nop
EOF
fi

# rebuild using patched sources (init.s, trap table, devices, and stripped libgcc)
if [[ -x cortos_build/build.sh ]]; then
	if [[ -f "$libgcc_dst_dir/libgcc.a" ]]; then
		(
			export AJIT_LIBGCC_INSTALL_DIR="${libgcc_dst_dir}"
			cd cortos_build && ./build.sh
		)
	else
		( cd cortos_build && ./build.sh )
	fi
fi

# Recreate GDB helper files (cortos2 build regenerates cortos_build/)
gdb_cmds="cortos_build/gdb_commands.txt"
cat > "$gdb_cmds" <<'EOF'
file main.elf
target remote :9999
set pagination off

# Breakpoints
b my_serial_interrupt_handler
b ajit_generic_interrupt_handler

# Show PSR and key MMIO regs at start
printf "=== initial regs ===\n"
info reg psr
p/x $psr
printf "IRC_CTRL @0xFFFF3000 = "
x/wx 0xFFFF3000
printf "SERIAL_CTRL @0xFFFF3200 = "
x/wx 0xFFFF3200
printf "SERIAL_RX @0xFFFF3208 = "
x/wx 0xFFFF3208

continue
EOF

gdb_sh="cortos_build/gdb.sh"
cat > "$gdb_sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

GDB_BIN="/home/Manoj/Documents/ajit-toolchain-marshal/buildroot_src_64/src/development/amv-ajit-code/ajit-gdb-7.6.2/build/gdb/gdb"
GDB_DATA="/home/Manoj/Documents/ajit-toolchain-marshal/buildroot_src_64/src/development/amv-ajit-code/ajit-gdb-7.6.2/build/gdb/data-directory"

exec "$GDB_BIN" --data-directory="$GDB_DATA" -x gdb_commands.txt
EOF
chmod +x "$gdb_sh"

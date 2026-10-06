#!/usr/bin/env python3
"""Reuse the simulator's compiler/include configuration; write only locally."""
from pathlib import Path
import shlex,subprocess
root=Path(__file__).resolve().parents[1]
logs=sorted((root/'sitar/logs').glob('compile_*.log'),key=lambda p:p.stat().st_mtime,reverse=True)
command=None
for p in logs:
 for line in p.read_text().splitlines():
  if line.startswith('g++ ') and ' -c ' in line and line.endswith('/shim/modular_async_cache.cpp'):
   command=shlex.split(line);break
 if command:break
assert command,'Build simulator first'
args=[];i=1
while i<len(command)-1:
 if command[i]=='-o':i+=2;continue
 if command[i]!='-c':args.append(command[i])
 i+=1
# The first line records the installation used by the simulator build.
config=root/'sitar/sitar_scons_config.txt'
sitar_root=Path(config.read_text().splitlines()[0])
out=root/'sitar/logs/async_protocol_test'
subprocess.run([command[0],*args,str(root/'tests_async/cache_protocol.cpp'),
 str(root/'shim/modular_async_cache.cpp'),str(root/'shim/modular_packet_helpers.cpp'),
 str(root/'shim/modular_dcache_adapter.o'),str(root/'shim/modular_instance_id.o'),
 str(sitar_root/'core/sitar_module.o'),
 str(sitar_root/'core/sitar_simulation.o'),
 str(sitar_root/'core/sitar_logger.o'),
 str(root/'shim/sitar_port_wrapper.o'),str(root/'ajit_thread_deps/src/CacheInterface.o'),
 '-o',str(out)],cwd=root/'sitar',check=True)
subprocess.run([str(out)],cwd=root,check=True)

#!/usr/bin/env python3
"""Run existing images read-only, with all artifacts confined to the modular tree.

Build the requested topology first. This snapshots the executable and hashes
inputs, then runs both modes with otherwise identical environments.
"""
import argparse,datetime,hashlib,json,os,re,shutil,subprocess,sys,time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
repo=root.parents[3]
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def console_errors(expectation, actual, env):
 if not actual.is_file():return ['Console output missing']
 if expectation.suffix!='.markers':
  return [] if expectation.read_bytes()==actual.read_bytes() else ['Console bytes differ']
 console=actual.read_text(errors='replace');errors=[]
 for line in expectation.read_text().splitlines():
  if not line or line.startswith('#'):continue
  marker,sep,count=line.partition('|count=')
  marker=re.sub(r'\$\{([A-Z_][A-Z_0-9]*)(?::-([^}]*))?\}',
      lambda m:env.get(m[1],m[2] or ''),marker)
  # Marker files use integer addition for benchmark expected values. Do not
  # evaluate shell code from an expectation file.
  marker=re.sub(r'\$\(\(\s*(\d+)\s*\+\s*(\d+)\s*\)\)',
      lambda m:str(int(m[1])+int(m[2])),marker)
  actual_count=sum(marker in x for x in console.splitlines())
  if (actual_count!=int(count)) if sep else (actual_count==0):
   errors.append(repr(marker)+': found '+str(actual_count)+', expected '+(count if sep else 'at least 1'))
 return errors
def pick_case(row):
 p=repo/row[1]
 if p.suffix=='.vprj':
  folder=p.parent/'compiled'/p.stem
  return folder/(p.stem+'.mmap'),folder/(p.stem+'.results')
 choices=[p/'main.mmap.remapped',p/'main.mmap',p/'cortos_build/main.mmap.remapped',p/'cortos_build/main.mmap']
 choices += sorted(p.glob('*.mmap.remapped'))+sorted(p.glob('*.mmap'))
 mmap=next((x for x in choices if x.is_file()),None)
 expected=p/(row[2] or 'main.results')
 if not expected.is_file():
  expected=p/'cortos_build'/(row[2] or 'main.results')
 return mmap,expected

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument('--ids',required=True,help='comma-separated registry IDs, or short-existing')
 ap.add_argument('--modes',default='0,1')
 ap.add_argument('--cores',default='1')
 ap.add_argument('--workers',type=int,choices=range(1,65),default=8)
 ap.add_argument('--cycles',type=int,default=400000)
 args=ap.parse_args()
 rows={}
 for line in (root/'testcase_registry_v2.txt').read_text().splitlines():
  if line and not line.startswith('#'):
   r=line.split('|');r+=['']*(14-len(r));rows[r[0]]=r
 if args.ids=='short-existing':
  ids=[k for k,r in rows.items() if int(r[7] or 400000)<5000000 and
       (not r[3] or r[3]==args.cores) and pick_case(r)[0] and pick_case(r)[0].is_file()]
 else:ids=args.ids.split(',')
 if not ids:raise RuntimeError('No images selected; build the testcase images first')
 out=root/'sitar/logs'/('async_images_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S'))
 out.mkdir();binary=out/'sitar_sim';shutil.copy2(root/'sitar/sitar_sim',binary)
 (out/'source.diff').write_bytes(subprocess.check_output(['git','diff','--','.'],cwd=root))
 manifest={str(p.relative_to(root)):sha(p) for d in ['shim','sitar/modules','tests_async']
           for p in (root/d).iterdir() if p.is_file() and p.suffix in ['.cpp','.h','.sitar','.py']}
 manifest['sitar/cop.sitar']=sha(root/'sitar/cop.sitar');manifest['executable']=sha(binary)
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True))
 print(out,flush=True);failed=False;summary=[]
 for case in ids:
  r=rows[case];mmap,expected=pick_case(r)
  if not mmap or not mmap.is_file():raise RuntimeError('Missing prebuilt image: '+case)
  cores=r[3] or args.cores
  if cores!=args.cores:raise RuntimeError('Build topology {} required for {}'.format(cores,case))
  expected_text=expected.read_text() if expected.is_file() else ''
  required_threads=1+max([int(t) for t in re.findall(r'\bthread\s+(\d+)',expected_text)] or [0])
  threads=r[4] or str(required_threads)
  active=r[5] or str(required_threads)
  count_file=mmap.parent/'thread_count.txt'
  if not r[5] and count_file.is_file():active=count_file.read_text().strip()
  if int(threads)<required_threads:raise RuntimeError('Expected results require more threads: '+case)
  linux=case.startswith('linux_')
  cycles=max(500000000,args.cycles) if linux else max(args.cycles,int(r[7] or args.cycles))
  for mode in args.modes.split(','):
   d=out/(case+'_mode'+mode);work=d/'sitar';work.mkdir(parents=True)
   (d/'test_memmap_input_0.txt').symlink_to(mmap)
   env={k:v for k,v in os.environ.items() if not k.startswith(('AJIT_','OMP_'))}
   env.update(OMP_NUM_THREADS=str(args.workers),AJIT_MODULAR_ASYNC=mode,AJIT_MODULAR_REAL_THREAD='1',
       AJIT_NUM_CORES=cores,AJIT_THREADS_PER_CORE=threads,
       AJIT_ACTIVE_THREADS=active,AJIT_STOP_ON_TA0=r[13] or 'off',
       AJIT_INIT_PC=r[10] or '0',AJIT_MEMORY_DELAY=r[12] or '0',
       AJIT_RDASR_USES_SITAR_TIME='1',AJIT_CONSOLE_INPUT_FILE=str(repo/r[8]) if r[8] else '/dev/null',
       AJIT_CONSOLE_OUTPUT_FILE=str(work/'console.out'),AJIT_IRQ_DIAGNOSTICS='0',
       AJIT_MODULAR_TRACE_ADAPTER='0',AJIT_WT_FAULT_DIAGNOSTICS='0',
       AJIT_DUMP_REGS_ON_TA0='1',AJIT_DUMP_REGS_ON_SUMMARY='1')
   if r[6]:env['AJIT_ACTIVE_THREAD_MASK']=r[6]
   if r[11]:env['AJIT_TIMER_TICK_DIV']=r[11]
   if r[8] and r[9]:
    expected_console=(repo/r[9]).read_text()
    env['AJIT_CONSOLE_INPUT_PACED']='prompt' if '?' in expected_console.splitlines() else '1'
   if linux:env.update(AJIT_INIT_PC='0xf0000000',AJIT_TIMER_TICK_DIV='10000',AJIT_THREADS_PER_CORE='1')
   if expected.is_file():
    addresses=re.findall(r'm\[\s*(0x[0-9a-fA-F]+)\s*\]',expected.read_text())
    env['AJIT_EXPECT_MEM_ADDRS']=','.join(dict.fromkeys(addresses))
    env['AJIT_DUMP_MEMORY_ON_TA0']='1' if addresses else '0'
   cfg={k:v for k,v in env.items() if k.startswith(('AJIT_','OMP_'))}
   cfg.update(executable_sha256=sha(binary),mmap_sha256=sha(mmap),cycles=cycles)
   (d/'config.json').write_text(json.dumps(cfg,sort_keys=True,indent=2))
   print('START {} mode={} cycles={}'.format(case,mode,cycles),flush=True)
   start=time.time()
   with (work/'run.out').open('w') as stdout,(work/'run.err').open('w') as stderr,open(os.devnull) as stdin:
    proc=subprocess.run([str(binary),str(cycles)],cwd=work,env=env,stdin=stdin,stdout=stdout,stderr=stderr)
   ok=proc.returncode==0
   console=(work/'console.out').read_text(errors='replace') if (work/'console.out').exists() else ''
   if linux:
    ok=ok and 'Welcome to Buildroot' in console and 'buildroot login:' in console
   elif expected.is_file():
    checkenv=env.copy();checkenv.update(AJIT_EXPECT_RESULTS_FILE=str(expected),
        AJIT_RESULTS_RUN_ERR=str(work/'run.err'),AJIT_ACTUAL_RESULTS_FILE=str(work/'main.results.sitar'))
    with (work/'check.log').open('w') as f:
     result=subprocess.run([str(root/'check_sitar_results.sh')],env=checkenv,cwd=work,stdout=f,stderr=f)
    ok=ok and result.returncode==0
   elif not r[9]:raise RuntimeError('No register/memory or console expectations for '+case)
   if r[9]:
    expectation=repo/r[9]
    errors=console_errors(expectation,work/'console.out',env)
    (work/'console_check.log').write_text('\n'.join(errors) if errors else 'PASS\n')
    ok=ok and not errors
   assert sha(binary)==manifest['executable']
   entry=dict(case=case,mode=int(mode),passed=ok,returncode=proc.returncode,seconds=round(time.time()-start,2),cycles=cycles)
   summary.append(entry);(out/'summary.json').write_text(json.dumps(summary,indent=2))
   print(('PASS' if ok else 'FAIL')+' '+json.dumps(entry),flush=True);failed=failed or not ok
 print('SUMMARY '+str(out/'summary.json'),flush=True)
 return 1 if failed else 0
if __name__=='__main__':sys.exit(main())

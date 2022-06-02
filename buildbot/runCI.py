import sys
import subprocess
import os
import shutil
import argparse
import json
import re
from enum import Enum
from dataclasses import dataclass
import gbench
from gbench import util, report
from gbench.util import *

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class ScoreResult(Enum):
    SKIPPED = READY = 0
    BUILD_FAILED = 1
    BENCH_FAILED = 2
    PASSED = 3

parser = argparse.ArgumentParser(description='test results')
parser.add_argument("-workdir", type=str, help="working directory", default="")
parser.add_argument("-v", help="verbose", action="store_true", default=False)

args = parser.parse_args()
workdir = args.workdir
verbose = args.v

def buildAndValidate(buildDir, cmakeCxxFlags):
  try:
    subprocess.check_call("cmake -E make_directory " + buildDir, shell=True)
    print("Prepare build directory - OK")
  except:
    print(bcolors.FAIL + "Prepare build directory - Failed" + bcolors.ENDC)
    return False

  os.chdir(buildDir)

  try:
    if sys.platform != 'win32':
      subprocess.check_call("cmake -DCMAKE_BUILD_TYPE=Release " + cmakeCxxFlags + " " + os.path.join(buildDir, ".."), shell=True)
    else:
      subprocess.check_call("cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release " + cmakeCxxFlags + " " + os.path.join(buildDir, ".."), shell=True)
    print("CMake - OK")
  except:
    print(bcolors.FAIL + "CMake - Failed" + bcolors.ENDC)
    return False

  try:
    subprocess.check_call("cmake --build . --config Release --target clean", shell=True)
    subprocess.check_call("cmake --build . --config Release --parallel 8", shell=True)
    print("Build - OK")
  except:
    print(bcolors.FAIL + "Build - Failed" + bcolors.ENDC)
    return False

  try:
    subprocess.check_call("cmake --build . --config Release --target validate", shell=True)
    print("Validation - OK")
  except:
    print(bcolors.FAIL + "Validation - Failed" + bcolors.ENDC)
    return False

  return True

def build(workdir, solutionOrBaseline, cmakeCxxFlags):
  os.chdir(workdir)
  buildDir = os.path.join(workdir, "build_" + solutionOrBaseline)
  print("Build and Validate the " + solutionOrBaseline)
  if not buildAndValidate(buildDir, cmakeCxxFlags):
    return False

  return True

def noChangesToTheBaseline(workdir):
  solutionDir = os.path.join(workdir, "build_solution")
  baselineDir = os.path.join(workdir, "build_baseline")
  if sys.platform != 'win32':
    solutionExe = os.path.join(solutionDir, "wordcount")
    baselineExe = os.path.join(baselineDir, "wordcount")
    exit_code = subprocess.call("cmp " + solutionExe + " " + baselineExe, shell=True)
  else:
    # This is the ugly way of comparing whether binaries are similar on Windows.
    # We disassemble both binaries and compare textual output.
    solutionExe = os.path.join(solutionDir, "wordcount.exe")
    solutionDisasm = os.path.join(solutionDir, "wordcount.disasm")
    baselineExe = os.path.join(baselineDir, "wordcount.exe")
    baselineDisasm = os.path.join(baselineDir, "wordcount.disasm")
    subprocess.call("llvm-objdump.exe -d " + solutionExe + " >" + solutionDisasm, shell=True)
    subprocess.call("llvm-objdump.exe -d " + baselineExe + " >" + baselineDisasm, shell=True)
    subprocess.call("powershell -Command \"((Get-Content -path " + baselineDisasm + " -Raw) -replace 'baseline','') | Set-Content -Path " + baselineDisasm + "\"", shell=True)
    subprocess.call("powershell -Command \"((Get-Content -path " + solutionDisasm + " -Raw) -replace 'solution','') | Set-Content -Path " + solutionDisasm + "\"", shell=True)
    exit_code = subprocess.call("fc >NUL " + solutionDisasm + " " + baselineDisasm, shell=True)
  return exit_code == 0

def getSpeedUp(diff_report):
  old = diff_report[0]['measurements'][0]['real_time']
  new = diff_report[0]['measurements'][0]['real_time_other']
  diff = old - new
  speedup = (diff / old ) * 100
  return speedup

def benchmarkSolutionOrBaseline(buildDir, solutionOrBaseline):
  os.chdir(buildDir)
  try:
    subprocess.check_call("cmake --build " + buildDir + " --config Release --target benchmark", shell=True)
    print("Benchmarking " + solutionOrBaseline + " - OK")
  except:
    print(bcolors.FAIL + "Benchmarking " + solutionOrBaseline + " - Failed" + bcolors.ENDC)
    return False

  return True

def benchmark(workdir):

  print("Benchmark solution against the baseline")

  solutionDir = os.path.join(workdir, "build_solution")
  baselineDir = os.path.join(workdir, "build_baseline")

  benchmarkSolutionOrBaseline(solutionDir, "solution")
  benchmarkSolutionOrBaseline(baselineDir, "baseline")

  outJsonSolution = gbench.util.load_benchmark_results(os.path.join(solutionDir, "result.json"))
  outJsonBaseline = gbench.util.load_benchmark_results(os.path.join(baselineDir, "result.json"))

  # Parse two report files and compare them
  diff_report = gbench.report.get_difference_report(
    outJsonBaseline, outJsonSolution, True)
  output_lines = gbench.report.print_difference_report(
    diff_report,
    False, True, 0.05, True)
  for ln in output_lines:
    print(ln)

  speedup = getSpeedUp(diff_report)
  if abs(speedup) < 2.0:
    print (bcolors.FAIL + "New version has performance similar to the baseline (<2% difference). Submission failed." + bcolors.ENDC)
    return False
  if speedup < 0:
    print (bcolors.FAIL + "New version is slower. Submission failed." + bcolors.ENDC)
    return False

  print (bcolors.OKGREEN + "Measured speedup:", "{:.2f}".format(speedup), "%" + bcolors.ENDC)
  return True

# Entry point
if not workdir:
  print ("Error: working directory is not provided.")
  sys.exit(1)

os.chdir(workdir)

result = True

if not build(workdir, "solution", "-DCMAKE_CXX_FLAGS=\" -DSOLUTION  \""):
  sys.exit(1)
if not build(workdir, "baseline", "-DCMAKE_CXX_FLAGS=\"  \""):
  sys.exit(1)

if noChangesToTheBaseline(workdir):
  print(bcolors.OKCYAN + "The solution and the baseline are identical. Skipped." + bcolors.ENDC) 
else:
  result = benchmark(workdir)

if not result:
  sys.exit(1)
else:
  sys.exit(0)

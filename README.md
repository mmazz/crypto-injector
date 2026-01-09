# CKKS Fault Injection Framework

Dynamic binary instrumentation tool for architectural fault injection in CKKS (OpenFHE).

We compile OpenFHE with CKKS using the default dynamically linked build (i.e., without static linking).

Using a statically linked binary significantly reduces instrumentation granularity, since system libraries (e.g., libc) are merged into the same executable.

By relying on dynamic linking, we can use Pin’s -l 0 option to exclude shared system libraries from instrumentation, allowing us to focus exclusively on OpenFHE’s code and reduce noise and overhead.

## Quick Start

### 1. Setup Intel Pin
```bash
./setup.sh  # Downloads Pin 3.30 automatically
```

### 2. Build pintools
```bash
make profilers  # Build profiling tools
make injectors  # Build fault injector
# or just: make
```

### 3. Run profiling
```bash
# Basic instruction mix
./scripts/run_profile.sh basic /path/to/openfhe_test

# Per-function analysis
./scripts/run_profile.sh function /path/to/openfhe_test

# Targeted arithmetic profiling
./scripts/run_profile.sh targeted /path/to/openfhe_test
```

### 4. Run fault injection
```bash
./scripts/run_campaign.sh \
  --binary /path/to/openfhe_test \
  --function Encrypt \
  --opcode VADDPD \
  --num-faults 1000
```

## Notes

To use Openfhe first export:

```bash
export LD_LIBRARY_PATH=$HOME/openfhe-PRNG-Control/install/lib:$LD_LIBRARY_PATH
```

To compile the campaings:

```bash
cmake -DCMAKE_PREFIX_PATH=$HOME/openfhe-PRNG-Control/install -DBUILD_STATIC=OFF -DCMAKE_BUILD_TYPE=Release ..
make -j16
```
To compile the library:

```bash
cmake -DCMAKE_INSTALL_PREFIX=$HOME/openfhe-PRNG-Control/install -DBUILD_STATIC=OFF -DBUILD_SHARED=ON -DCMAKE_BUILD_TYPE=Release -DWITH_OPENMP=OFF -DBUILD_UNITTESTS=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_EXTRAS=OFF ..
make -j16
sudo make install
```


## Project Structure

- `src/profiler/` - Profiling pintools
- `src/injector/` - Fault injection pintool
- `src/common/` - Shared utilities
- `scripts/` - Automation scripts
- `tests/` - Simple test programs

## Requirements

- Linux x86-64
- GCC 14+ or Clang 19+
- Intel Pin 4.0 (auto-downloaded by setup.sh)

## Citation

[Your paper info]

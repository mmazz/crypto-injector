# CKKS Fault Injection Framework

Dynamic binary instrumentation tool for architectural fault injection in CKKS (OpenFHE).

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

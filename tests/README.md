# Pin Tool Project Setup

This guide explains how to set up a minimal Pin tool project outside of Pin's source tree.

## Problem Overview

Pin's build system expects a specific structure. When building tools **outside** Pin's source tree, you need to properly configure `PIN_ROOT` and split your configuration into two files following Pin's conventions.

## Directory Structure

```
crypto-injector/
├── pin/                    # Pin installation
│   ├── pin                 # Pin executable
│   └── source/
│       └── tools/
│           └── Config/
│               ├── makefile.config
│               └── makefile.default.rules
└── tests/                  # Your tool directory
    ├── makefile            # Main makefile (includes configuration)
    ├── makefile.rules      # Tool definitions (your tools go here)
    └── hello.cpp           # Your tool source code
```

**Important:** `hello.cpp` must be in the **same directory** as the makefiles, not in a subdirectory.

## Setup Instructions

### 1. Create your project structure

```bash
mkdir tests
cd tests
```

### 2. Create `makefile`

This file handles the Pin build system integration:

```makefile
##############################################################
#
# Makefile for tools outside Pin's source tree
#
##############################################################

# CRITICAL: Define PIN_ROOT before the ifdef
PIN_ROOT := $(shell pwd)/../pin

ifdef PIN_ROOT
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
else
CONFIG_ROOT := ../Config
endif

include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules
```

**Key point:** `PIN_ROOT := $(shell pwd)/../pin` must be defined **before** the `ifdef` check, otherwise it evaluates to false and tries to use `../Config` (which doesn't exist outside Pin's tree).

### 3. Create `makefile.rules`

This file defines which tools to build:

```makefile
##############################################################
#
# makefile.rules - Tool definitions
#
##############################################################

# Tools to compile
TEST_TOOL_ROOTS := hello

# Leave these empty
TEST_ROOTS :=
TOOL_ROOTS :=
SA_TOOL_ROOTS :=
APP_ROOTS :=
OBJECT_ROOTS :=
DLL_ROOTS :=
LIB_ROOTS :=

# Sanity subset
SANITY_SUBSET := $(TEST_TOOL_ROOTS) $(TEST_ROOTS)

##############################################################
#
# Test recipes (optional)
#
##############################################################

# Add custom test recipes here if needed

##############################################################
#
# Build rules (optional)
#
##############################################################

# Add custom build rules here if needed
```

### 4. Create your tool source code

Create `hello.cpp` in the same directory:

```cpp
#include <iostream>
#include "pin.H"

using namespace std;

VOID Fini(INT32 code, VOID *v) {
    cout << "=== Pin tool finished ===" << endl;
}

INT32 Usage() {
    cerr << "This is a basic Pin tool example" << endl;
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc, char *argv[]) {
    if (PIN_Init(argc, argv)) return Usage();

    cout << "=== Pin tool started ===" << endl;

    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();

    return 0;
}
```

### 5. Build

```bash
make tools
```

This will create `obj-intel64/hello.so` and others tools you add

or

```bash
make hello.test
```

Will create `obj-intel64/hello.so` and execute it.

### 6. Run

```bash
../pin/pin -t obj-intel64/hello.so -- /bin/ls
```

## Why This Structure?

Pin's official examples use this two-file approach:

- **`makefile`**: Contains the infrastructure (includes, Pin integration)
- **`makefile.rules`**: Contains your project-specific configuration (which tools to build)

This separation makes it easy to:
- Add new tools (just update `TEST_TOOL_ROOTS` in `makefile.rules`)
- Share the same `makefile` across multiple projects
- Follow Pin's conventions for compatibility

## Adding More Tools

To add a new tool (e.g., `bitflip`):

1. Create `bitflip.cpp` in the same directory
2. Update `makefile.rules`:
   ```makefile
   TEST_TOOL_ROOTS := hello bitflip
   ```
3. Run `make`

Both tools will be compiled to `obj-intel64/`.

## Extending for Bit-Flip Injection

For your bit-flip injection tool, you can:

1. Add custom build rules in the "Build rules" section of `makefile.rules`
2. Add custom test recipes in the "Test recipes" section
3. Use the same structure to compile multiple related tools

Example for a bit-flip injector with custom flags:

```makefile
# In makefile.rules, add:

TEST_TOOL_ROOTS := hello bitflip

# Custom build rule for bitflip
$(OBJDIR)bitflip$(OBJ_SUFFIX): bitflip.cpp
	$(CXX) $(TOOL_CXXFLAGS) -DVERBOSE_MODE $(COMP_OBJ)$@ $<

# Custom test
bitflip.test: $(OBJDIR)bitflip$(PINTOOL_SUFFIX) $(TESTAPP)
	$(PIN) -t $(OBJDIR)bitflip$(PINTOOL_SUFFIX) -o results.log -- $(TESTAPP)
```

## Reference

This setup is based on Pin's official examples in:
- `pin/source/tools/ManualExamples/`
- `pin/source/tools/SimpleExamples/`

For more details, see Pin's documentation at: https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html

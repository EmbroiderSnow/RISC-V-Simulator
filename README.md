# RISC-V ISA Simulator

This project is a course assignment for the **Computer Architecture and Organization Lab** at **Peking University** in the **Fall semester of 2025**, developed as an extension of the framework provided by the course.

## ✨ Features

* Implements a majority of the instructions from the RV64IM instruction set.
* Implements the **`exit`** and **`write`** system calls (with `write` currently supporting output only to **stdout**), and enables the use of **`printf`** within the virtual RISC-V environment.
* Includes a simple built-in debugger with support for single-stepping, printing registers, and scanning memory.
* Supports function call tracing (**ftrace**).
* Supports instruction execution history logging (**itrace**).

```
.
├── .github/workflows/    # GitHub CI/CD workflow configurations
├── build/                # Directory for compiled artifacts
├── doc/                  # Directory for docs about the implement of some features
├── sim/                  # Simulator core source code
│   ├── include/          # Header files
│   └── src/              # Source files
├── test/                 # Test cases
│   ├── include/
│   ├── lib/        
│   ├── src/              # Test program source code
│   └── trm/              # Trap and Run Machine environment
├── trace/                # Reference trace files for instruction execution
├── Dockerfile            # Dockerfile for building the development environment
├── docker-compose.yml    # Docker Compose configuration file
└── Makefile              # Main project Makefile
```

## 🚀 Getting Started

### Prerequisites

This project is best built and run using Docker to avoid the complexities of local environment setup.

* [Docker](https://www.docker.com/)
* [Docker Compose](https://docs.docker.com/compose/)

If you prefer to build locally, you will need to install the RISC-V GNU toolchain and other build tools.
* `riscv64-unknown-elf-gcc`
* `make`
* `gcc`/`g++`
* lib `llvm` and `libelf`

### Build and Run (Docker)

1.  **Start the Docker container:**
    This command will build the image and create a container named `simulator_dev` based on `docker-compose.yml`.

    ```bash
    docker-compose up --build -d
    ```

2. **Build the project and run all test:**

   ```bash
   docker-compose exec dev make test-all MODEL=[iss|mc|pl]
   ```

### Build and Run (Local)

1.  **Install dependencies:**
    Ensure you have installed all the tools listed in the [Prerequisites](#prerequisites) section.

2.  **Build the project and run all test**
    Run the `make` command in the project root directory.

    ```bash
    make test-all 
    ```

## 🧩 Simulator Usage Guide

This document explains how to build and run the RISC-V simulator and test programs using the provided **Makefile**.
It describes all available commands, parameters, and typical usage examples.

---

### ⚙️ Build Commands

| Command           | Description                                                    |
| ----------------- | -------------------------------------------------------------- |
| `make build`      | Build both the simulator (`sim`) and the test program (`test`) |
| `make build-sim`  | Build the simulator only                                       |
| `make build-test` | Build the test program only                                    |

---

### 🚀 Running the Simulator

#### Basic Command Format

```bash
make run T=<test_name> MODEL=<model_name> [ARGS="optional arguments"]
```

#### Parameters

| Parameter | Description                                        | Example                               |
| --------- | -------------------------------------------------- | ------------------------------------- |
| `T`       | The test program name (without `.bin` extension)   | `T=dummy`                             |
| `MODEL`   | The simulator model to run                         | `MODEL=iss` or `MODEL=mc` or `MODEL=pl`        |
| `ARGS`    | Optional runtime arguments passed to the simulator | `ARGS="--itrace"` or `ARGS="--debug"` |

#### Execution Process

When you run `make run`, the following happens automatically:

1. The simulator and test program are built (via `build-sim` and `build-test`).
2. The command below is executed:

   ```
   sim/build/Simulator <MODEL> <T> <ARGS>
   ```

   * `<MODEL>` specifies which simulator model to run.
   * `<T>` is the test program name.
   * `<ARGS>` are additional runtime options.

---

### 🧠 Predefined Shortcuts

To simplify common use cases, the Makefile defines several shortcut targets:

| Command               | Equivalent to                                | Description                                           |
| --------------------- | -------------------------------------------- | ----------------------------------------------------- |
| `make iss T=dummy`    | `make run MODEL=iss T=dummy ARGS="--batch"`  | Run the ISS (instruction set simulator) in batch mode |
| `make mc T=dummy`     | `make run MODEL=mc T=dummy ARGS=""`          | Run the multi-cycle CPU model                         |
| `make debug T=dummy`  | `make run MODEL=iss T=dummy ARGS="--debug"`  | Run the ISS in debug mode                             |
| `make itrace T=dummy` | `make run MODEL=iss T=dummy ARGS="--itrace"` | Enable instruction tracing                            |
| `make ftrace T=dummy` | `make run MODEL=iss T=dummy ARGS="--ftrace"` | Enable function call tracing                          |

---

### 🧪 Examples

#### Example 1 — Run the Multi-Cycle Model

```bash
make run T=dummy MODEL=mc
```

➡️ Builds and runs `test/build/dummy.bin` using the multi-cycle CPU model.

---

#### Example 2 — Run the ISS Model

```bash
make run T=dummy MODEL=iss
```

➡️ Builds and runs the instruction set simulator.

---

#### Example 3 — Enable Instruction Tracing

```bash
make run T=dummy MODEL=iss ARGS="--itrace"
```

or simply:

```bash
make itrace T=dummy
```

➡️ Runs the simulator with instruction-level trace output.

---

#### Example 4 — Debug Mode

```bash
make debug T=dummy
```

➡️ Runs the ISS with detailed debug output enabled.

---

#### Example 5 — Clean the Build

```bash
make clean
```

➡️ Removes all generated files under `sim/build/` and `test/build/`.

---

### 📘 Summary

| Use Case          | Recommended Command          |
| ----------------- | ---------------------------- |
| First run         | `make run T=dummy MODEL=iss` |
| Multi-cycle model | `make run T=dummy MODEL=mc`  |
| Batch mode        | `make iss T=dummy`           |
| Instruction trace | `make itrace T=dummy`        |
| Debug mode        | `make debug T=dummy`         |
| Clean build files | `make clean`                 |


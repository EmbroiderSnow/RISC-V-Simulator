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
   docker-compose exec dev make test_all
   ```

### Build and Run (Local)

1.  **Install dependencies:**
    Ensure you have installed all the tools listed in the [Prerequisites](#prerequisites) section.

2.  **Build the project and run all test**
    Run the `make` command in the project root directory.

    ```bash
    make test_all
    ```

## 🎮 Usage

### Running Test Programs

You can run any test program located in the project's root directory. All test programs are compiled into the `test/build` directory.

For example, to run the `quicksort` test:
```bash
docker-compose exec dev make T=quicksort
```

### Debug Mode

Start the simulator in debug mode by the make command below:
```bash
docker-compose exec dev make debug T=XXX
```

In debug mode, you can use the following commands:

- `help`: Print the help message of debug commands

* `si [N]`: Execute a single instruction if `arg N` not provided, or execute N instructions.
* `c`: Continue execution until the program finishes.
* `info r`: Print the status of all general-purpose registers (include PC).
* `x <N> <EXPR>`: Scan `N` 4-byte words of memory starting at address `EXPR`.

### Other Commands

To run the Simulator in `itrace`/`ftrace` mode:

```sh
docker-compose exec dev make itrace T=XXX
docker-compose exec dev make ftrace T=XXX
```

## ✅ Testing

If you want to add your own test cases, place your test source file (`.c` file) in the **`/test/src`** directory.
 For example, if your test is named **`sample`**, you can run it with the following command:

```bash
docker-compose exec dev make T=sample
```

FROM docker.io/harsonlau/riscv-tools:latest

RUN apt-get update && apt-get install -y llvm-dev

RUN rm -rf /var/lib/apt/lists/*
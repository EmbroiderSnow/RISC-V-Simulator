FROM docker.io/harsonlau/riscv-tools:latest

RUN apt-get update && apt-get install -y llvm-dev && apt-get install libelf-dev -y

RUN rm -rf /var/lib/apt/lists/*
# Building

## Building on Linux (and maybe others)

1. Clone the repo.
2. `cd simple_harness`
3. `mkdir build`
4. `cd build`
5. `cmake ..`
6. `cmake --build .`
7. `cd ..`
8. `./build/simple_harness`

## Building on MacOS
1. Clone the repo.
2. `cd simple_harness`
3. `gcc -o compile compile.c`
4. `./compile`
5. `./simple_harness`

## Usage

1. `./simple_harness --make-template` generates a template file.
2. `./simple_harness ./template_harness.txt` launches the GUI viewer with the template file.

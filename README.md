The first part (Modifications) of texts is from @yc518suray. The second part of texts is from the original README.

This repository is forked from @tylerlumsden. The codes are used to search for periodic Golay pairs (PGPs or PCPs) with lengths up to 90, 106 and 130, as an midterm project in the class **Sequence Design for Communications** in NCKU CCE, 2025 fall.

## Modifications

### Script

The script argument list is modified:

```bash
./driver.sh [Length] [Compression Factor] --start [Stage1] --stop [Stage2]
```

The `--start` option is used to start the program from some specific stage. `[Stage1]`is the stage from which the program starts. If this option is not provided, the program starts from the beginning.

- `M` or `m` $\rightarrow$ matching
- `U` or `u` $\rightarrow$ uncompression
- `E` or `e` $\rightarrow$ equivalence filtering
- `P` or `p` $\rightarrow$ calculate PAPR of PCPs

The `--stop` option is used to stop the program after some specific stage. `[Stage2]`is the stage after which the program stops. If this option is not provided, the program finishes as usual.

- `G` or `g` $\rightarrow$ candidate generation
- `M` or `m` $\rightarrow$ matching
- `U` or `u` $\rightarrow$ uncompression
- `E` or `e` $\rightarrow$ equivalence filtering

A **PAPR calculation** stage is added. Therefore, if you don't want to calculate the PAPR of each found unique PCP, you can stop the program after equivalence filtering.

If both options (along with the `Stage` parameters) are ignored, the script runs as usual.

### Parallelization

To accelerate the searching process, program parallelization is adopted, mainly through OpenMP and splitting files.

### Trick

In order to find PCP of length 106 and 130, within limited computation time, a random-selection approach is adopted. It's basically a random selection of the compressed pairs. To this end, first compile the source code `src/generate_selection.cpp`. Then use this program as follows:

```bash
./generate_selection.exe [total_lines] [lines_per_execution] [number_of_files]
```

For example, in the case of L = 130, if the total number of lines in **130-pairs-found** is 10000, and the uncompression stage splits file **130-pairs-found** into 12 files (for parallelization). You might want to uncompress 100 lines for each uncompression.exe execution, then the command goes like:

```bash
./generate_selection.exe 10000 100 12
```

Note that the parameter `number_of_files` should be the same as the parameter `NUM_PROCS` in the driver script `driver.sh`. Otherwise, the result is unexpected.

Afterwards, a file `rd_select.out` is generated. Put this file in the `results/130` directory, and specify the specific selection you would like to uncompress in the uncompress.sh script, by setting the parameter `SELECT`. `SELECT=1` means that the first line (selection #1) of `rd_select.out` is used, `SELECT=69` means that the 69th line (selection #69) of `rd_select.out` is used, etc.

This trick can be used to accelerate uncompression. For example, suppose there are 1001 selections in the file `rd_select.out`. We can have 1001 computers work on each selection. So this trick can also be used to parallelized the uncompression process.

### Verifaction

We provide a program to verify that the found pairs are actually PCPs.

Compile:

```bash
g++ -Wall -g -O3 verify_pcp.cpp -o verofy_pcp
```

Usage:

```bash
./verify_pcp.exe [path_to_file] [Length]
```

Pull requests with explanations are welcome.

---

This repository contains a set of procedures for the generation and post-processing of Periodic Golay Pairs.

## Prerequisites

To compile and run our code, you are required to have installed the following:

```A valid C++ compiler``` (our code is compiled using the ```g++``` command)

```FFTW``` (version 3.3.10)

The FFTW instructions for installing the FFTW library are located at https://fftw.org/.

Due to the FFTW library, our code can only be compiled in a unix environment.

## Usage

Compilation of our code is handled by a Makefile, so to compile our procedures you should enter the directory where the Makefile is located, and enter the bash command ```make```, ensuring no errors are produced.

To run our code, we have a driver script that handles the interactions between our different procedures.

## Driver Script

Our driver script is usable with the following commands:

```bash
./driver.sh [Length] [Compression Factor]
```

Valid length values for searching with our code include: 2, 4, 8, 10, 16, 18, 20, 26, 32, 34, 36, 40, 50, 52, 58, 64, 68, 72, 74, 80, 82, and 90.

Valid compression factor values include any number which divides the length value. To search without compression, use a compression factor of 1.

The output of our procedures will be located in the ```results``` directory, stored in the file [Length]-pairs-found

## Procedures

The ```src``` directory contains all source files containing runnable procedures which compile to a ```main``` function.

The ```lib``` directory contains all source files containing helper functions for our main procedures.




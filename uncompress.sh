#!/bin/bash
#SBATCH --account=def-cbright
#SBATCH --time=7-00:00
#SBATCH --mem-per-cpu=4G
#SBATCH --cpus-per-task=1
#SBATCH -o ./Report/output.%a.out # STDOUT 
    
    order=$1
    compress=$2
    newcompress=$3
    proc=$4

    start=`date +%s`

    echo "--- Proc $proc starting uncompression ---"
    INPUT_FILE="results/$order/uncomp_input_$proc"

    if [ ! -f "$INPUT_FILE" ]; then
        echo "Error: Worker input file $INPUT_FILE not found. Exiting Proc $proc."
        exit 1
    fi

    ./bin/uncompression $order $compress $newcompress $proc $INPUT_FILE

    end=`date +%s`
    runtime3=$((end-start))
    echo "Proc $proc finished in $runtime3 seconds."
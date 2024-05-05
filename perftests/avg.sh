#!/bin/bash

ITERATIONS=100

total_runtime=0

for ((i=1; i<=$ITERATIONS; i++)); do
    start=$(date +%s.%N)

    ./ast

    end=$(date +%s.%N)

    runtime=$(echo "$end - $start" | bc)
    total_runtime=$(echo "$total_runtime + $runtime" | bc)
done

average_runtime=$(echo "scale=2; $total_runtime / $ITERATIONS" | bc)

echo "Average runtime of ./dce over $ITERATIONS iterations: $average_runtime seconds"


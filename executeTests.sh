# This will run our compiler on a C file, export its LLVM ASM to a .ll file and output how it went to the log.
# The LLVM ASM is than ran and its outut sent to the execution log.
function runTest {
    ./run.sh -i tests/$1.c -fAsm -o tests/$1.ll &> tests/$1.log
    ./run.sh -i tests/$1.c -fAsm -optimize -o tests/$1Optimized.ll &> tests/$1Optimized.log
    # lli new-tests/$1.ll &> new-tests/$1Execution.log
    llvm-as tests/$1.ll -o tests/$1.bc &> tests/$1Bytecode.log
    llvm-as tests/$1Optimized.ll -o tests/$1Optimized.bc &> tests/$1OptimizedBytecode.log
}

# Run all the C file tests.
for file in "tests"/*.c; do
    file=$(basename $file .c)
    echo "Testing: $file"
    runTest $file
done
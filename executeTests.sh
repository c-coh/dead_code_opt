# This will run our compiler on a C file, export its LLVM ASM to a .ll file and output how it went to the log.
# The LLVM ASM is than ran and its outut sent to the execution log.
function runTest {
    ./run.sh -i new-tests/$1.c -fAsm -o new-tests/$1.ll &> new-tests/$1.log
    # lli new-tests/$1.ll &> new-tests/$1Execution.log
    llvm-as new-tests/$1.ll -o new-tests/$1.bc &> new-tests/$1Bytecode.log
}

# Run all the C file tests.
for file in "new-tests"/*.c; do
    file=$(basename $file .c)
    echo "Testing: $file"
    runTest $file
done
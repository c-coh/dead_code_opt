# Dead Code Elimination Pass for LLVM AST
Ceci Cohen, clc169

Christopher Danner, cld99

Joey Li, xxl1021

[Copy project description from report]

## Running the Code
To easily run new test cases or other inputs, add the appropriate C files to the `tests` folder, navigate to the base directory for the project (either open the project folder in the terminal from file explorer or navigate within the terminal using `cd`), and run `./execute tests.sh`. Note that if the `./build.sh` scipt does not work on your computer, you will need to build the files manually by following a process similar to that outlined in `./build.sh` or `PG4_README.md`.

To run a file independently, use `./run.sh -i filename.c -fAsm -o filename.ll &> filename.log` to obtain the compiled LLVM IR file for the C program (`filename.ll`) and a log file showing the console output - including the AST - for the program (`filename.log`). You can omit `&> filename.log` to instead print the output to the console.

You can run the compiled LLVM file with `lli filename.ll &> filenameExecution.log` to obtain a log file showing the console output of running the program (`filenameExecution.log`). You can omit `&> filename.log` to instead print the output to the console.

You can convert the LLVM IR to LLVM bytecode with `llvm-as filename.ll -o filename.bc &> filenameBytecode.log` to obtain the compiled LLVM bytecode for the LLVM IR and a log file showing the console output. You can omit `&> filenameBytecode.log` to instead print the output to the console.

Note that the above commands assume that your file to run are in the base directory of the project. If this is not the case, use file paths to route to the correct file for compilation and execution.

### Required Packages
Required Packages: **CLANG**, **Make**, **CMake**, **LLVM**, **Flex**, **Bison**, **GraphViz**

If you are missing any of these required packages, refer to 'PG4_README.md' for installation instructions (Linux).

## Test Cases
test1:
    Tested variable uses in all major classes of nodes without dataflow complexities
    Classes of Nodes Tested:
        Statement blocks
        Return statements
        Binary expressions (addition, subtraction, multiplication, division, and, or, comparison)
        Unary expressions (int to float, float to int, int to bool, bool to int, negation)
        Function calls
        Assignment statements
    Relevant Lines:
        Line 15: Variable use in unary expression (int to float)
        Line 16: Variable use in assignment
        Line 18: Variable use in binary expression (comparison)
        Line 19: Variable use in function call
        Line 24: Variable use in return statement
test2:
    Tested dead variables in all major classes of nodes without dataflow complexities
    Classes of Nodes Tested:
        Statement blocks
        Return statements
        Binary expressions (addition, subtraction, multiplication, division, and, or, comparison)
        Unary expressions (int to float, float to int, int to bool, bool to int, negation)
        Function calls
        Assignment statements
    Relevant Lines:
        Line 13: Dead assignment in statement block
        Line 14: Dead assignment in binary expression (addition)
        Line 15: Dead assignment in assignment statement
        Line 16: Dead assignment in unary operator (int to float)
        Line 17: Dead assignment in function call
        Line 18: Dead assignment in return statement
test3:
    Tested variables dead through one and through all dataflow paths of if statements
    Tested variables dead through for loop body execution only and through all dataflow paths
    Tested variables dead through for loop body execution only and through all dataflow paths
    Relevant Lines:
        Line 13: Variable dead through one dataflow path only (then)
        Line 18: Variable dead through all dataflow paths (reassigned after if statement)
        Line 20: Dead assignment in if statement
        Line 24: Variable dead through one dataflow path only (else)
        Line 30: Variable dead through all dataflow paths (then and else)
        Line 38: Variable dead through for loop body only
        Line 43: Variable dead on all paths through for loop
        Line 45: Dead assignment in for loop
        Line 49: Variable dead through while loop body only
        Line 56: Variable dead on all paths through while loop
        Line 59: Dead assignment in while loop
test4:
    Tested unreachable code in if, for, and while statements
    Relevant Lines:
        Line 7: Reachable code with always-true if statement condition
        Line 10: Unreachable code with always-true if statement condition
        Line 14: Unreachable code with always-false if statement condition
        Line 17: Reachable code with always-false if statement condition
        Line 21: Reachable code with indeterminate if statement condition
        Line 24: Reachable code with indeterminate if statement condition
        Line 27: True or expression in conditional
        Line 28: Reachable code with always-true while loop condition
        Line 31: False and expression in conditional
        Line 32: Unreachable code with always-false while loop condition
        Line 36: Indeterminate or expression in conditional
        Line 36: Reachable code with indeterminate while loop condition
        Line 39: True and expression in conditional
        Line 40: Reachable code with always-true for loop condition
        Line 43: False or expression in conditional
        Line 44: Unreachable code with always-false for loop condition
        Line 47: Indeterminate and expression in conditional
        Line 48: Reachable code with indeterminate for loop condition
    Note: Due to checking for an always-true loop condition, test case should be compiled but not run
test5:
    Tested corner cases such as dead assignments with function calls or live assignments as their right-hand side
    Relevant Lines:
        Line 14: Dead assignment with essential function call as right-hand side
        Line 15: Dead assignment with live assignment as right-hand side
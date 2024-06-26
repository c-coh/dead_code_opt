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
The primary test cases used for this project were as follows:
var1: Variable declared and assigned but never used in function body
var2: Variable declared, assigned, and used outside if-else statment but code finished inside if statment and the only usage of the variable cannot be reach
var3: Variable declared and assigned inside if-else statment but never used
var4: Variable declared and assigned inside for loop but never used
var5: Multiple variables declared and assigned but never used in function body
var6: Variable declared outside if-else statment and assigned inside if-else statment but never used
var7: Variable declared and assigned inside for-loop header but never used
var8: Multiple variables declared and assigned outside and inside for-loop body and header but not used
var9: Variable declared and assigned in different for-loop body but neither used
var10: Multiple variables with different types declared and assigned but never used in function body
var11: Variable assigned inside if-else statment but never used in one branch
var12: Variable assigned inside while loop but never used

Variables declared and assigned but not used in for loop declaration.
Variables declared and assigned but not used in function which was invoked by another function.
Function invoked but return value ignored.
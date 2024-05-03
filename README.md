# Dead Code Elimination Pass for LLVM AST
Ceci Cohen, clc169

Christopher Danner, cld99

Joey Li, xxl1021

[Copy project description from report]

## Running the Code
To easily run new test cases or other inputs, add the appropriate C files to the `tests` folder, navigate to the base directory for the project (either open the project folder in the terminal from file explorer or navigate within the terminal using `cd`), and run `./execute tests.sh`.

To run a file independently, use `./run.sh -i filename.c -fAsm -o filename.ll &> filename.log` to obtain the compiled LLVM IR file for the C program (`filename.ll`) and a log file showing the console output - including the AST - for the program (`filename.log`). You can omit `&> filename.log` to instead print the output to the console.

You can run the compiled LLVM file with `lli filename.ll &> filenameExecution.log` to obtain a log file showing the console output of running the program (`filenameExecution.log`). You can omit `&> filename.log` to instead print the output to the console.

Note that the above commands assume that your file to run are in the base directory of the project. If this is not the case, use file paths to route to the correct file for compilation and execution.

### Required Packages
Required Packages: **CLANG**, **Make**, **CMake**, **LLVM**, **Flex**, **Bison**, **GraphViz**

If you are missing any of these required packages, refer to 'PG4_README.md' for installation instructions (Linux).

## Test Cases
The primary test cases used for this project were as follows:

[Copy description of test cases from report]
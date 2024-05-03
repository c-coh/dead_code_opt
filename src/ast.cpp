#include "ast.h"
#include "function.h"
#include "statements/block.h"
#include "statements/for.h"
#include "statements/if.h"
#include "statements/return.h"
#include "statements/while.h"
#include "expressions/addition.h"
#include "expressions/and.h"
#include "expressions/assignment.h"
#include "expressions/bool2Int.h"
#include "expressions/call.h"
#include "expressions/comparison.h"
#include "expressions/division.h"
#include "expressions/float2Int.h"
#include "expressions/int2Bool.h"
#include "expressions/int2Float.h"
#include "expressions/multiplication.h"
#include "expressions/negative.h"
#include "expressions/or.h"
#include "expressions/subtraction.h"
#include "expressions/variable.h"

#include <iostream>
#include <typeinfo>
#include <llvm/Bitcode/BitcodeWriter.h>

//#include <llvm/Transforms/InstCombine/InstCombine.h> // This causes an error on my machine.
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

AST::AST(const std::string modName) : module(modName, context), builder(context), fpm(&module)
{

    // This requires the above includes that don't work on my machine, so I can't really add these default optimizations.

    // Promote allocas to registers.
    fpm.add(llvm::createPromoteMemoryToRegisterPass());

    // Do simple "peephole" optimizations and bit-twiddling optzns.
    //fpm.add(llvm::createInstructionCombiningPass());

    // Reassociate expressions.
    fpm.add(llvm::createReassociatePass());

    // Do simple "peephole" optimizations and bit-twiddling optzns.
    //fpm.add(llvm::createInstructionCombiningPass());

    // Reassociate expressions.
    fpm.add(llvm::createReassociatePass());

    // Eliminate Common SubExpressions.
    fpm.add(llvm::createGVNPass());

    // Simplify the control flow graph (deleting unreachable blocks, etc).
    fpm.add(llvm::createCFGSimplificationPass());

    // Finally initialize.
    fpm.doInitialization();

}

ASTFunction* AST::AddFunction(const std::string& name, std::unique_ptr<VarType> returnType, ASTFunctionParameters parameters, bool variadic)
{

    // Add to our function list.
    auto func = std::make_unique<ASTFunction>(*this, name, std::move(returnType), std::move(parameters), variadic);
    functionList.push_back(name);
    functions[name] = std::move(func);
    return functions[name].get();

}

ASTFunction* AST::GetFunction(const std::string& name)
{

    // Get function if exists.
    auto found = functions.find(name);
    if (found != functions.end()) return found->second.get();
    else throw std::runtime_error("ERROR: Function " + name + " can not be found in the ast!");

}

void AST::Compile()
{

    // All we need to do is compile each function.
    for (auto& func : functionList)
    {
        std::cout << "INFO: Compiling function " + func + "." << std::endl;
        functions[func]->Compile(module, builder);
    }
    compiled = true;

}

std::string AST::ToString()
{
    std::string output = module.getModuleIdentifier() + "\n";
    for (int i = 0; i < functionList.size() - 1; i++)
        output += "├──" + functions[functionList.at(i)]->ToString("│  ");
    output += "└──" + functions[functionList.back()]->ToString("   ");
    return output;
}

void AST::WriteLLVMAssemblyToFile(const std::string& outFile)
{
    if (!compiled) throw std::runtime_error("ERROR: Module " + std::string(module.getName().data()) + " not compiled!");
    if (outFile == "") throw std::runtime_error("ERROR: Writing assembly to standard out is not supported!");
    std::error_code err;
    llvm::raw_fd_ostream outLl(outFile, err);
    module.print(outLl, nullptr);
    outLl.close();
}

void AST::WriteLLVMBitcodeToFile(const std::string& outFile)
{
    if (!compiled) throw std::runtime_error("ERROR: Module " + std::string(module.getName().data()) + " not compiled!");
    if (outFile == "") throw std::runtime_error("ERROR: Writing bitcode to standard out is not supported!");
    std::error_code err;
    llvm::raw_fd_ostream outBc(outFile, err);
    llvm::WriteBitcodeToFile(module, outBc);
    outBc.close();
}

void AST::DeadCodeEliminationPass()
{
    // Keep track of function live status.
    std::map<std::string, bool> funcLive;

    for (auto& [name, func] : functions)
    {
        // Keep track of variable live status.
        std::map<std::string, bool> varLive;
        // For each defined function, perform dead code elimination on its body
        if(func->definition) {
            // Get body of function and call EliminateDeadCode on it
            //std::unique_ptr<ASTStatement> node = func->definition;
            EliminateDeadCode(std::move(func->definition), varLive, funcLive);
        }
    }
}

    bool AST::EliminateDeadCode(std::unique_ptr<ASTStatement> node, std::map<std::string, bool>& variables, std::map<std::string, bool>& functions)
    {
        // Use dynamic_cast to check node type?
        auto funcType = dynamic_cast<ASTStatementBlock*>(node.get());
        // Recursively call dead code elimination for complex nodes, or update/retrieve live status for simple variable uses/assignments
            if(dynamic_cast<ASTStatementBlock*>(node.get()) != NULL) {
                printf("Reading Statement Block\n");
                ASTStatementBlock* nodePtr = dynamic_cast<ASTStatementBlock*>(node.get());
                // Iterate through children in reverse order, removing dead assignments
                for(int i = nodePtr->statements.size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(std::move(nodePtr->statements[i]), variables, functions)) nodePtr->statements.erase(nodePtr->statements.begin()+i);
                }
            }
            else if(dynamic_cast<ASTStatementIf*>(node.get()) != NULL) {
                printf("Reading If Statement\n");
                ASTStatementIf* nodePtr = dynamic_cast<ASTStatementIf*>(node.get());
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> elseVars(variables);
                if(EliminateDeadCode(std::move(nodePtr->elseStatement), elseVars, functions)) nodePtr->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
                if(EliminateDeadCode(std::move(nodePtr->thenStatement), variables, functions)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, elseVars);
                if(EliminateDeadCode(std::move(nodePtr->condition), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            }
            else if(dynamic_cast<ASTStatementWhile*>(node.get()) != NULL) {
                printf("Reading While Statement\n");
                ASTStatementWhile* nodePtr = dynamic_cast<ASTStatementWhile*>(node.get());
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> loopVars(variables);
                if(EliminateDeadCode(std::move(nodePtr->thenStatement), loopVars, functions)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, loopVars);
                if(EliminateDeadCode(std::move(nodePtr->condition), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            }
            else if(dynamic_cast<ASTStatementFor*>(node.get()) != NULL) {
                printf("Reading For Statement\n");
                ASTStatementFor* nodePtr = dynamic_cast<ASTStatementFor*>(node.get());
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> loopVars(variables);
                if(EliminateDeadCode(std::move(nodePtr->increment), loopVars, functions)) nodePtr->increment = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->increment.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->body), loopVars, functions)) nodePtr->body = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, loopVars);
                if(EliminateDeadCode(std::move(nodePtr->condition), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->init), variables, functions)) nodePtr->init = std::unique_ptr<ASTStatement>(nullptr);
            }
            else if(dynamic_cast<ASTStatementReturn*>(node.get()) != NULL) {
                printf("Reading Return Statement\n");
                ASTStatementReturn* nodePtr = dynamic_cast<ASTStatementReturn*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->returnExpression), variables, functions)) nodePtr->returnExpression = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->returnExpression.get())->right);
            }
            else if(dynamic_cast<ASTExpressionAddition*>(node.get()) != NULL) {
                printf("Reading Addition Expression\n");
                ASTExpressionAddition* nodePtr = dynamic_cast<ASTExpressionAddition*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionSubtraction*>(node.get()) != NULL) {
                printf("Reading Subtraction Expression\n");
                ASTExpressionSubtraction* nodePtr = dynamic_cast<ASTExpressionSubtraction*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionMultiplication*>(node.get()) != NULL) {
                printf("Reading Multiplication Expression\n");
                ASTExpressionMultiplication* nodePtr = dynamic_cast<ASTExpressionMultiplication*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionDivision*>(node.get()) != NULL) {
                printf("Reading Division Expression\n");
                ASTExpressionDivision* nodePtr = dynamic_cast<ASTExpressionDivision*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionAnd*>(node.get()) != NULL) {
                printf("Reading And Expression\n");
                ASTExpressionAnd* nodePtr = dynamic_cast<ASTExpressionAnd*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionOr*>(node.get()) != NULL) {
                printf("Reading Or Expression\n");
                ASTExpressionOr* nodePtr = dynamic_cast<ASTExpressionOr*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionComparison*>(node.get()) != NULL) {
                printf("Reading Addition Expression\n");
                ASTExpressionComparison* nodePtr = dynamic_cast<ASTExpressionComparison*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->a1), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
                if(EliminateDeadCode(std::move(nodePtr->a2), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            }
            else if(dynamic_cast<ASTExpressionFloat2Int*>(node.get()) != NULL) {
                printf("Reading Float to Int Expression\n");
                ASTExpressionFloat2Int* nodePtr = dynamic_cast<ASTExpressionFloat2Int*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->operand), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            }
            else if(dynamic_cast<ASTExpressionInt2Float*>(node.get()) != NULL) {
                printf("Reading Int to Float Expression\n");
                ASTExpressionInt2Float* nodePtr = dynamic_cast<ASTExpressionInt2Float*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->operand), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            }
            else if(dynamic_cast<ASTExpressionInt2Bool*>(node.get()) != NULL) {
                printf("Reading Int to Bool Expression\n");
                ASTExpressionInt2Bool* nodePtr = dynamic_cast<ASTExpressionInt2Bool*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->operand), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            }
            else if(dynamic_cast<ASTExpressionBool2Int*>(node.get()) != NULL) {
                printf("Reading Bool to Int Expression\n");
                ASTExpressionBool2Int* nodePtr = dynamic_cast<ASTExpressionBool2Int*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->operand), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            }
            else if(dynamic_cast<ASTExpressionNegation*>(node.get()) != NULL) {
                printf("Reading Negation Expression\n");
                ASTExpressionNegation* nodePtr = dynamic_cast<ASTExpressionNegation*>(node.get());
                if(EliminateDeadCode(std::move(nodePtr->operand), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            }
            else if(dynamic_cast<ASTExpressionCall*>(node.get()) != NULL) {
                printf("Reading Call Expression\n");
                ASTExpressionCall* nodePtr = dynamic_cast<ASTExpressionCall*>(node.get());
                functions.emplace(nodePtr->callee->ToString(""), true);
                // Iterate through children in reverse order, removing dead assignments
                for(int i = nodePtr->arguments.size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(std::move(nodePtr->arguments[i]), variables, functions)) nodePtr->arguments.erase(nodePtr->arguments.begin()+i);
                }
            }
            else if(dynamic_cast<ASTExpressionVariable*>(node.get()) != NULL) {
                printf("Reading Variable\n");
                ASTExpressionVariable* nodePtr = dynamic_cast<ASTExpressionVariable*>(node.get());
                // If variable is already in map, update live status to true, otherwise add it to map
                if(variables.find(nodePtr->var) != variables.end()) variables[nodePtr->var] = true;
                else variables.emplace(nodePtr->var, true);
            }
            else if(dynamic_cast<ASTExpressionAssignment*>(node.get()) != NULL) {
                printf("Reading Assignment Expression\n");
                ASTExpressionAssignment* nodePtr = dynamic_cast<ASTExpressionAssignment*>(node.get());
                // If variable is in map and live, set live status to false but do not remove assignment, otherwise, mark assignment for removal
                if(variables.find(nodePtr->left->ToString("")) != variables.end() && variables.at(nodePtr->left->ToString(""))) {
                    variables[nodePtr->left->ToString("")] = false;
                    if(EliminateDeadCode(std::move(nodePtr->right), variables, functions)) nodePtr->right = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->right.get())->right);
                    return false;
                }
                return true;
            }
        return false;
    }

    void AST::mergeVarMaps(std::map<std::string, bool>& map1, std::map<std::string, bool>& map2)
    {
        // Add each variable from map 2 to map 1 if not already included
        for(auto& [key, value] : map2) {
            // If variable is not in map 1, add it
            if(map1.find(key) == map1.end()) map1.emplace(key, value);
            // If variable is in map 1 and it is live in map 2, set it to live
            else if(value) map1[key] = value;
        }
    }
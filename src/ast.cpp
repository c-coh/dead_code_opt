#include "ast.h"
#include "function.h"
#include "expression.h"
#include "statements/block.h"
#include "statements/for.h"
#include "statements/if.h"
#include "statements/return.h"
#include "statements/while.h"
#include "expressions/addition.h"
#include "expressions/and.h"
#include "expressions/assignment.h"
#include "expressions/bool2Int.h"
#include "expressions/bool.h"
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
        std::cout << "INFO: Finished compiling function " + func + "." << std::endl;
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
            EliminateDeadCode(func->definition.get(), varLive, funcLive, true);
        }
    }
}

    bool AST::EliminateDeadCode(ASTStatement* node, std::map<std::string, bool>& variables, std::map<std::string, bool>& functions, bool eliminate)
    {
        // Recursively call dead code elimination for complex nodes, or update/retrieve live status for simple variable uses/assignments
        if(dynamic_cast<ASTStatementBlock*>(node) != NULL) {
            ASTStatementBlock* nodePtr = dynamic_cast<ASTStatementBlock*>(node);
            // Iterate through children in reverse order, removing dead assignments
            for(int i = nodePtr->statements.size() - 1; i >= 0; i--) {
                if(EliminateDeadCode(nodePtr->statements[i].get(), variables, functions, eliminate)) nodePtr->statements.erase(nodePtr->statements.begin()+i);
            }
        }
        else if(dynamic_cast<ASTStatementIf*>(node) != NULL) {
            EliminateUnreachableCode(node);
            ASTStatementIf* nodePtr = dynamic_cast<ASTStatementIf*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> elseVars(variables);
            if(EliminateDeadCode(nodePtr->elseStatement.get(), elseVars, functions, eliminate)) nodePtr->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
            if(EliminateDeadCode(nodePtr->thenStatement.get(), variables, functions, eliminate)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, elseVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions, eliminate)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
        }
        else if(dynamic_cast<ASTStatementWhile*>(node) != NULL) {
            EliminateUnreachableCode(node);
            ASTStatementWhile* nodePtr = dynamic_cast<ASTStatementWhile*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> loopVars(variables);
            // Traverse loop body and condition once to obtain accurate live variables going into loop body
            EliminateDeadCode(nodePtr->thenStatement.get(), loopVars, functions, false);
            EliminateDeadCode(nodePtr->condition.get(), loopVars, functions, false);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(loopVars, variables);
            if(EliminateDeadCode(nodePtr->thenStatement.get(), loopVars, functions, false)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, loopVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions, eliminate)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
        }
        else if(dynamic_cast<ASTStatementFor*>(node) != NULL) {
            EliminateUnreachableCode(node);
            ASTStatementFor* nodePtr = dynamic_cast<ASTStatementFor*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> loopVars(variables);
            EliminateDeadCode(nodePtr->increment.get(), loopVars, functions, false);
            EliminateDeadCode(nodePtr->body.get(), loopVars, functions, false);
            EliminateDeadCode(nodePtr->condition.get(), loopVars, functions, false);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(loopVars, variables);
            if(EliminateDeadCode(nodePtr->increment.get(), loopVars, functions, false)) nodePtr->increment = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->increment.get())->right);
            if(EliminateDeadCode(nodePtr->body.get(), loopVars, functions, eliminate)) nodePtr->body = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, loopVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions, false)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            if(EliminateDeadCode(nodePtr->init.get(), variables, functions, false)) nodePtr->init = std::unique_ptr<ASTStatement>(nullptr);
        }
        else if(dynamic_cast<ASTStatementReturn*>(node) != NULL) {
            ASTStatementReturn* nodePtr = dynamic_cast<ASTStatementReturn*>(node);
            if(EliminateDeadCode(nodePtr->returnExpression.get(), variables, functions, eliminate)) nodePtr->returnExpression = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->returnExpression.get())->right);
        }
        else if(dynamic_cast<ASTExpressionAddition*>(node) != NULL) {
            ASTExpressionAddition* nodePtr = dynamic_cast<ASTExpressionAddition*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionSubtraction*>(node) != NULL) {
            ASTExpressionSubtraction* nodePtr = dynamic_cast<ASTExpressionSubtraction*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionMultiplication*>(node) != NULL) {
            ASTExpressionMultiplication* nodePtr = dynamic_cast<ASTExpressionMultiplication*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionDivision*>(node) != NULL) {
            ASTExpressionDivision* nodePtr = dynamic_cast<ASTExpressionDivision*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionAnd*>(node) != NULL) {
            ASTExpressionAnd* nodePtr = dynamic_cast<ASTExpressionAnd*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionOr*>(node) != NULL) {
            ASTExpressionOr* nodePtr = dynamic_cast<ASTExpressionOr*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionComparison*>(node) != NULL) {
            ASTExpressionComparison* nodePtr = dynamic_cast<ASTExpressionComparison*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions, eliminate)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions, eliminate)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
        }
        else if(dynamic_cast<ASTExpressionFloat2Int*>(node) != NULL) {
            ASTExpressionFloat2Int* nodePtr = dynamic_cast<ASTExpressionFloat2Int*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions, eliminate)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
        }
        else if(dynamic_cast<ASTExpressionInt2Float*>(node) != NULL) {
            ASTExpressionInt2Float* nodePtr = dynamic_cast<ASTExpressionInt2Float*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions, eliminate)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
        }
        else if(dynamic_cast<ASTExpressionInt2Bool*>(node) != NULL) {
            ASTExpressionInt2Bool* nodePtr = dynamic_cast<ASTExpressionInt2Bool*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions, eliminate)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
        }
        else if(dynamic_cast<ASTExpressionBool2Int*>(node) != NULL) {
            ASTExpressionBool2Int* nodePtr = dynamic_cast<ASTExpressionBool2Int*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions, eliminate)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
        }
        else if(dynamic_cast<ASTExpressionNegation*>(node) != NULL) {
            ASTExpressionNegation* nodePtr = dynamic_cast<ASTExpressionNegation*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions, eliminate)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
        }
        else if(dynamic_cast<ASTExpressionCall*>(node) != NULL) {
            ASTExpressionCall* nodePtr = dynamic_cast<ASTExpressionCall*>(node);
            functions.emplace(nodePtr->callee->ToString(""), true);
            // Iterate through children in reverse order, removing dead assignments
            for(int i = nodePtr->arguments.size() - 1; i >= 0; i--) {
                if(EliminateDeadCode(nodePtr->arguments[i].get(), variables, functions, eliminate)) nodePtr->arguments.erase(nodePtr->arguments.begin()+i);
            }
        }
        else if(dynamic_cast<ASTExpressionVariable*>(node) != NULL) {
            ASTExpressionVariable* nodePtr = dynamic_cast<ASTExpressionVariable*>(node);
            // If variable is already in map, update live status to true, otherwise add it to map
            if(variables.find(nodePtr->var) != variables.end()) variables[nodePtr->var] = true;
            else variables.emplace(nodePtr->var, true);
        }
        else if(dynamic_cast<ASTExpressionAssignment*>(node) != NULL) {
            ASTExpressionAssignment* nodePtr = dynamic_cast<ASTExpressionAssignment*>(node);
            // If variable is in map and live, set live status to false but do not remove assignment, otherwise, mark assignment for removal
            if(variables.find(nodePtr->left->ToString("")) != variables.end() && variables.at(nodePtr->left->ToString(""))) {
                variables[nodePtr->left->ToString("")] = false;
                if(EliminateDeadCode(nodePtr->right.get(), variables, functions, eliminate)) nodePtr->right = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->right.get())->right);
                return false;
            }
            return eliminate;
        }
        return false;
}

void AST::EliminateUnreachableCode(ASTStatement* node) {
    
    //IF STATEMENT
    if (dynamic_cast<ASTStatementIf*>(node) != nullptr) {
        ASTStatementIf* nodePtr = dynamic_cast<ASTStatementIf*>(node);

        int condVal = EvaluateExpression(nodePtr->condition.get());

        //check for always-true or always-false conditionals
        if(condVal == 1) {
            //expression is always true; 'else' is unreachable
            nodePtr->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
        }
        else if(condVal == 0) {
            //expression is always false; 'then' is unreachable
            nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
        }
    }
    //FOR STATEMENT
    else if (dynamic_cast<ASTStatementFor*>(node) != nullptr) {
        ASTStatementFor* nodePtr = dynamic_cast<ASTStatementFor*>(node);

        //check for always-true or always-false conditionals
        int condVal = EvaluateExpression(nodePtr->condition.get());

        if (condVal == 0) {
            // Loop condition is false or not determinable, loop body is unreachable
            nodePtr->body = std::unique_ptr<ASTStatement>(nullptr);
        }
    }
    //WHILE STATEMENT
    else if (dynamic_cast<ASTStatementWhile*>(node) != nullptr) {

        ASTStatementWhile* nodePtr = dynamic_cast<ASTStatementWhile*>(node);

        // Evaluate the condition expression
        int condVal = EvaluateExpression(nodePtr->condition.get());

        if (condVal == 0) {
            // Loop condition is false or not determinable, loop body is unreachable
            nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
        }
    }
}

// Helper function to evaluate an expression
int AST::EvaluateExpression(ASTExpression* expr) {
    
    //VARIABLE EXPRESSION
    if (dynamic_cast<ASTExpressionVariable*>(expr) != nullptr) {

        //no procedure to handle var expressions at the moment
        return 2;

    //BOOLEAN EXPRESSION
    } else if (dynamic_cast<ASTExpressionBool*>(expr) != nullptr) {

        ASTExpressionBool* boolExpr = dynamic_cast<ASTExpressionBool*>(expr);
        return static_cast<int>((*boolExpr).value);

    //NEGATION EXPRESSION
    } else if (dynamic_cast<ASTExpressionNegation*>(expr) != nullptr) {

        ASTExpressionNegation* negExpr = dynamic_cast<ASTExpressionNegation*>(expr);
        auto result = EvaluateExpression(negExpr->operand.get());

        if(result == 2){
            return 2;
        }

        if (result == 1) {
            return 0;
        } else {
            return 1;
        }

    //AND EXPRESSION
    } else if (dynamic_cast<ASTExpressionAnd*>(expr) != nullptr) {

        ASTExpressionAnd* andExpr = dynamic_cast<ASTExpressionAnd*>(expr);
        auto left = EvaluateExpression(andExpr->a1.get());
        auto right = EvaluateExpression(andExpr->a2.get());
    
        if(!(left && right) || left == 2 || right == 2){
            return 2;
        }

        auto result = left + right;
        if(result == 2){
            return 1;
        }
        else{
            return 0;
        }

    //OR EXPRESSION
    } else if (dynamic_cast<ASTExpressionOr*>(expr) != nullptr) {

        ASTExpressionOr* orExpr = dynamic_cast<ASTExpressionOr*>(expr);
        auto left = EvaluateExpression(orExpr->a1.get());
        auto right = EvaluateExpression(orExpr->a2.get());

        if(!(left && right) || left == 2 || right == 2){
            return 2;
        }

        auto result = left + right;
        if (result >= 1) {
            return 1;
        } else {
            return 0;
        }
    } 
    else if (dynamic_cast<ASTExpressionComparison*>(expr) != nullptr) {
        ASTExpressionComparison* compExpr = dynamic_cast<ASTExpressionComparison*>(expr);
        auto left = compExpr->a1.get();
        auto right = compExpr->a2.get();

        // if (left && right) {
        //     switch (compExpr->op) {
        //         case ASTExpressionComparison::LESS_THAN:
        //             return static_cast<int>(left < right);

        //         case ASTExpressionComparison::GREATER_THAN:
        //             return static_cast<int>(left > right);

        //         case ASTExpressionComparison::LESS_THAN_OR_EQUAL:
        //             return static_cast<int>(left <= right);

        //         case ASTExpressionComparison::GREATER_THAN_OR_EQUAL:
        //             return static_cast<int>(left >= right);

        //         case ASTExpressionComparison::EQUAL:
        //             return static_cast<int>(left == right);

        //         case ASTExpressionComparison::NOT_EQUAL:
        //             return static_cast<int>(left >= right);
        //     }
        // } else {
        //     return 2;
        // }
        return 2;
    }

    //otherwise return unkown
    return 2;
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
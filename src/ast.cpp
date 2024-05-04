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
            EliminateDeadCode(func->definition.get(), varLive, funcLive);
        }
    }
    printf("Dead Code Elimination Successful\n");
}

    bool AST::EliminateDeadCode(ASTStatement* node, std::map<std::string, bool>& variables, std::map<std::string, bool>& functions)
    {
        // Recursively call dead code elimination for complex nodes, or update/retrieve live status for simple variable uses/assignments
        if(dynamic_cast<ASTStatementBlock*>(node) != NULL) {
            printf("Reading Statement Block\n");
            ASTStatementBlock* nodePtr = dynamic_cast<ASTStatementBlock*>(node);
            // Iterate through children in reverse order, removing dead assignments
            for(int i = nodePtr->statements.size() - 1; i >= 0; i--) {
                if(EliminateDeadCode(nodePtr->statements[i].get(), variables, functions)) nodePtr->statements.erase(nodePtr->statements.begin()+i);
            }
            printf("Statement Block Recursion Successful\n");
        }
        else if(dynamic_cast<ASTStatementIf*>(node) != NULL) {
            printf("Reading If Statement\n");
            ASTStatementIf* nodePtr = dynamic_cast<ASTStatementIf*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> elseVars(variables);
            if(EliminateDeadCode(nodePtr->elseStatement.get(), elseVars, functions)) nodePtr->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
            if(EliminateDeadCode(nodePtr->thenStatement.get(), variables, functions)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, elseVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            printf("If Statement Recursion Successful\n");
        }
        else if(dynamic_cast<ASTStatementWhile*>(node) != NULL) {
            printf("Reading While Statement\n");
            ASTStatementWhile* nodePtr = dynamic_cast<ASTStatementWhile*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> loopVars(variables);
            if(EliminateDeadCode(nodePtr->thenStatement.get(), loopVars, functions)) nodePtr->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, loopVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            printf("While Statement Recursion Successful\n");
        }
        else if(dynamic_cast<ASTStatementFor*>(node) != NULL) {
            printf("Reading For Statement\n");
            ASTStatementFor* nodePtr = dynamic_cast<ASTStatementFor*>(node);
            // Set up duplicate variable status map to account for branching paths
            std::map<std::string, bool> loopVars(variables);
            if(EliminateDeadCode(nodePtr->increment.get(), loopVars, functions)) nodePtr->increment = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->increment.get())->right);
            if(EliminateDeadCode(nodePtr->body.get(), loopVars, functions)) nodePtr->body = std::unique_ptr<ASTStatement>(nullptr);
            // Merge maps, assigning live status to variables that are live in either branch
            mergeVarMaps(variables, loopVars);
            if(EliminateDeadCode(nodePtr->condition.get(), variables, functions)) nodePtr->condition = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->condition.get())->right);
            if(EliminateDeadCode(nodePtr->init.get(), variables, functions)) nodePtr->init = std::unique_ptr<ASTStatement>(nullptr);
            printf("For Statement Recursion Successful\n");
        }
        else if(dynamic_cast<ASTStatementReturn*>(node) != NULL) {
            printf("Reading Return Statement\n");
            ASTStatementReturn* nodePtr = dynamic_cast<ASTStatementReturn*>(node);
            if(EliminateDeadCode(nodePtr->returnExpression.get(), variables, functions)) nodePtr->returnExpression = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->returnExpression.get())->right);
            printf("Return Statement Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionAddition*>(node) != NULL) {
            printf("Reading Addition Expression\n");
            ASTExpressionAddition* nodePtr = dynamic_cast<ASTExpressionAddition*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Addition Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionSubtraction*>(node) != NULL) {
            printf("Reading Subtraction Expression\n");
            ASTExpressionSubtraction* nodePtr = dynamic_cast<ASTExpressionSubtraction*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Subtraction Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionMultiplication*>(node) != NULL) {
            printf("Reading Multiplication Expression\n");
            ASTExpressionMultiplication* nodePtr = dynamic_cast<ASTExpressionMultiplication*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Multiplication Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionDivision*>(node) != NULL) {
            printf("Reading Division Expression\n");
            ASTExpressionDivision* nodePtr = dynamic_cast<ASTExpressionDivision*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Division Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionAnd*>(node) != NULL) {
            printf("Reading And Expression\n");
            ASTExpressionAnd* nodePtr = dynamic_cast<ASTExpressionAnd*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("And Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionOr*>(node) != NULL) {
            printf("Reading Or Expression\n");
            ASTExpressionOr* nodePtr = dynamic_cast<ASTExpressionOr*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Or Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionComparison*>(node) != NULL) {
            printf("Reading Comparison Expression\n");
            ASTExpressionComparison* nodePtr = dynamic_cast<ASTExpressionComparison*>(node);
            if(EliminateDeadCode(nodePtr->a1.get(), variables, functions)) nodePtr->a1 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a1.get())->right);
            if(EliminateDeadCode(nodePtr->a2.get(), variables, functions)) nodePtr->a2 = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->a2.get())->right);
            printf("Comparison Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionFloat2Int*>(node) != NULL) {
            printf("Reading Float to Int Expression\n");
            ASTExpressionFloat2Int* nodePtr = dynamic_cast<ASTExpressionFloat2Int*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            printf("Float to Int Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionInt2Float*>(node) != NULL) {
            printf("Reading Int to Float Expression\n");
            ASTExpressionInt2Float* nodePtr = dynamic_cast<ASTExpressionInt2Float*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            printf("Int to Float Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionInt2Bool*>(node) != NULL) {
            printf("Reading Int to Bool Expression\n");
            ASTExpressionInt2Bool* nodePtr = dynamic_cast<ASTExpressionInt2Bool*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            printf("Int to Bool Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionBool2Int*>(node) != NULL) {
            printf("Reading Bool to Int Expression\n");
            ASTExpressionBool2Int* nodePtr = dynamic_cast<ASTExpressionBool2Int*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            printf("Bool to Int Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionNegation*>(node) != NULL) {
            printf("Reading Negation Expression\n");
            ASTExpressionNegation* nodePtr = dynamic_cast<ASTExpressionNegation*>(node);
            if(EliminateDeadCode(nodePtr->operand.get(), variables, functions)) nodePtr->operand = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->operand.get())->right);
            printf("Negation Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionCall*>(node) != NULL) {
            printf("Reading Call Expression\n");
            ASTExpressionCall* nodePtr = dynamic_cast<ASTExpressionCall*>(node);
            functions.emplace(nodePtr->callee->ToString(""), true);
            // Iterate through children in reverse order, removing dead assignments
            for(int i = nodePtr->arguments.size() - 1; i >= 0; i--) {
                if(EliminateDeadCode(nodePtr->arguments[i].get(), variables, functions)) nodePtr->arguments.erase(nodePtr->arguments.begin()+i);
            }
            printf("Call Expression Recursion Successful\n");
        }
        else if(dynamic_cast<ASTExpressionVariable*>(node) != NULL) {
            printf("Reading Variable\n");
            ASTExpressionVariable* nodePtr = dynamic_cast<ASTExpressionVariable*>(node);
            // If variable is already in map, update live status to true, otherwise add it to map
            if(variables.find(nodePtr->var) != variables.end()) variables[nodePtr->var] = true;
            else variables.emplace(nodePtr->var, true);
        }
        else if(dynamic_cast<ASTExpressionAssignment*>(node) != NULL) {
            printf("Reading Assignment Expression\n");
            ASTExpressionAssignment* nodePtr = dynamic_cast<ASTExpressionAssignment*>(node);
            // If variable is in map and live, set live status to false but do not remove assignment, otherwise, mark assignment for removal
            if(variables.find(nodePtr->left->ToString("")) != variables.end() && variables.at(nodePtr->left->ToString(""))) {
                variables[nodePtr->left->ToString("")] = false;
                if(EliminateDeadCode(nodePtr->right.get(), variables, functions)) nodePtr->right = std::move(dynamic_cast<ASTExpressionAssignment*>(nodePtr->right.get())->right);
                return false;
            }
            return true;
        }
        return false;
}

bool EliminateUnreachableCode(std::unique_ptr<ASTStatement>& node) {
    
    //IF STATEMENT
    if (dynamic_cast<ASTStatementIf*>(node.get()) != nullptr) {
        ASTStatementIf* nodePtr = dynamic_cast<ASTStatementIf*>(node.get());

        std::unique_ptr<ASTExpression> condition = std::move(nodePtr->condition);
        int condVal = EvaluateExpression(std::move(condition));

        //check for always-true or always-false conditionals
        if (condVal == 2) {
            //expression is not guaranteed true or false
            EliminateUnreachableCode(nodePtr->thenStatement);
            EliminateUnreachableCode(nodePtr->elseStatement);

        } else if(condVal == 1) {
            //expression is always true; 'else' is unreachable
            EliminateUnreachableCode(nodePtr->thenStatement);
            nodePtr->elseStatement = nullptr;
        }
        else{
            //expression is always false; 'then' is unreachable
            nodePtr->thenStatement = nullptr;
            EliminateUnreachableCode(nodePtr->elseStatement);
        }
    //FOR STATEMENT
    } else if (dynamic_cast<ASTStatementFor*>(node.get()) != nullptr) {
        ASTStatementFor* nodePtr = dynamic_cast<ASTStatementFor*>(node.get());

        //check for always-true or always-false conditionals
        std::unique_ptr<ASTExpression> condition = std::move(nodePtr->condition);
        int condVal = EvaluateExpression(std::move(condition));

        if (condVal == 0) {
            // Loop condition is false or not determinable, loop body is unreachable
            nodePtr->body = nullptr;
        } else {
            // Loop body is reachable
            EliminateUnreachableCode(nodePtr->body);
        }
    //WHILE STATEMENT
    } else if (dynamic_cast<ASTStatementWhile*>(node.get()) != nullptr) {

        ASTStatementWhile* nodePtr = dynamic_cast<ASTStatementWhile*>(node.get());

        // Evaluate the condition expression
        std::unique_ptr<ASTExpression> condition = std::move(nodePtr->condition);
        int condVal = EvaluateExpression(std::move(condition));

        if (condVal == 0) {
            // Loop condition is false or not determinable, loop body is unreachable
            nodePtr->thenStatement = nullptr;
        } else {
            // Loop body is reachable
            EliminateUnreachableCode(nodePtr->thenStatement);
        }

    //OTHER STATEMENTS
    } else if (dynamic_cast<ASTStatementBlock*>(node.get()) != nullptr) {
        ASTStatementBlock* nodePtr = dynamic_cast<ASTStatementBlock*>(node.get());
        for (auto& stmt : nodePtr->statements) {
            EliminateUnreachableCode(stmt);
        }
    }

    return true;
}

// Helper function to evaluate an expression
int EvaluateExpression(std::unique_ptr<ASTExpression>& expr) {
    
    //VARIABLE EXPRESSION
    if (dynamic_cast<ASTExpressionVariable*>(expr.get()) != nullptr) {

        //no procedure to handle var expressions at the moment
        return 2;

    //BOOLEAN EXPRESSION
    } else if (dynamic_cast<ASTExpressionBool*>(expr.get()) != nullptr) {

        ASTExpressionBool* boolExpr = dynamic_cast<ASTExpressionBool*>(expr.get());
        return static_cast<int>boolExpr*.value

    //NEGATION EXPRESSION
    } else if (dynamic_cast<ASTExpressionNegation*>(expr.get()) != nullptr) {

        ASTExpressionNegation* negExpr = dynamic_cast<ASTExpressionNegation*>(expr.get());
        auto result = EvaluateExpression(negExpr->operand);

        if(result == 2){
            return 2;
        }

        if (result == 1) {
            return 0;
        } else {
            return 1;
        }

    //AND EXPRESSION
    } else if (dynamic_cast<ASTExpressionAnd*>(expr.get()) != nullptr) {

        ASTExpressionAnd* andExpr = dynamic_cast<ASTExpressionAnd*>(expr.get());
        auto left = EvaluateExpression(andExpr->a1);
        auto right = EvaluateExpression(andExpr->a2);
    
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
    } else if (dynamic_cast<ASTExpressionOr*>(expr.get()) != nullptr) {

        ASTExpressionOr* orExpr = dynamic_cast<ASTExpressionOr*>(expr.get());
        auto left = EvaluateExpression(orExpr->a1);
        auto right = EvaluateExpression(orExpr->a2);

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
    else if (dynamic_cast<ASTExpressionComparison*>(expr.get()) != nullptr) {
        ASTExpressionComparison* compExpr = dynamic_cast<ASTExpressionComparison*>(expr.get());
        auto left = compExpr->a1;
        auto right = compExpr->a2;

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
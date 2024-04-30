#include "ast.h"

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
    for (auto& [name, func] : functions)
    {
        // Keep track of variable live status.
        std::map<std::string, bool> varLive;
        // Keep track of function live status.
        std::map<std::string, bool> funcLive;
        // For each defined function, perform dead code elimination on its body
        if(func->definition) {
            // Get body of function and call EliminateDeadCode on it
            std::unique_ptr<ASTStatement> node = func->definition;
            EliminateDeadCode(node, varLive, funcLive);
        }
    }
}

private:

    bool AST::EliminateDeadCode(std::unique_ptr<ASTStatement> node, std::map<std::string, bool>& variables, std::map<std::string, bool>& functions)
    {
        // Use dynamic_cast to check node type?
        auto type = typeid(*node);
        // Recursively call dead code elimination for complex nodes, or update/retrieve live status for simple variable uses/assignments
        switch(type) {
            case typeid(ASTStatementBlock):
                // Iterate through children in reverse order, removing dead assignments
                for(int i = node->statements->size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(node->*statements[i], variables, functions)) node->statements->erase(i);
                }
                break;
            case typeid(AstStatementIf):
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> elseVars(variables);
                if(EliminateDeadCode(node->elseStatement, elseVars, functions)) node->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
                if(EliminateDeadCode(node->thenStatement, variables, functions)) node->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, elseVars);
                if(EliminateDeadCode(node->condition, variables, functions)) node->condition = node->condition->right;
                break;
            case typeid(ASTStatementWhile):
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> loopVars(variables);
                if(EliminateDeadCode(node->thenStatement, loopVars, functions)) node->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, loopVars);
                if(EliminateDeadCode(node->condition, variables, functions)) node->condition = node->condition->right;
                break;
            case typeid(ASTStatementFor):
                // Set up duplicate variable status map to account for branching paths
                std::map<std::string, bool> loopVars(variables);
                EliminateDeadCode(node->increment, loopVars, functions) node->increment = node->increment->right;
                if(EliminateDeadCode(node->body, loopVars, functions)) node->body = std::unique_ptr<ASTStatement>(nullptr);
                // Merge maps, assigning live status to variables that are live in either branch
                mergeVarMaps(variables, loopVars);
                if(EliminateDeadCode(node->condition, variables, functions)) node->condition = node->condition->right;
                if(EliminateDeadCode(node->init, variables, functions)) node->init = std::unique_ptr<ASTStatement>(nullptr);
                break;
            case typeid(ASTStatementReturn):
                if(EliminateDeadCode(node->returnExpression, variables, functions)) node->returnExpression = node->returnExpression->right;
                break;
            case typeid(ASTExpressionAddition):
            case typeid(ASTExpressionSubtraction):
            case typeid(ASTExpressionMultiplication):
            case typeid(ASTExpressionDivision):
            case typeid(ASTExpressionAnd):
            case typeid(ASTExpressionOr):
            case typeid(ASTExpressionComparison):
                if(EliminateDeadCode(node->a1, variables, functions)) node->a1 = node->a1->right;
                if(EliminateDeadCode(node->a2, variables, functions)) node->a2 = node->a2->right;
                break;
            case typeid(ASTExpressionFloat2Int):
            case typeid(ASTExpressionInt2Float):
            case typeid(ASTExpressionInt2Bool):
            case typeid(ASTExpressionBool2Int):
            case typeid(ASTExpressionNegation):
                if(EliminateDeadCode(node->operand, variables, functions)) node->operand = node->operand->right;
                break;
            case typeid(ASTExpressionCall):
                // Mark function as called
                functions.emplace(node->callee->toString(), true);
                for(int i = node->arguments->size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(node->*arguments[i], variables, functions)) node->statements->erase(i);
                }
                break;
            case typeid(ASTExpressionVar):
                // If variable is already in map, update live status to true, otherwise add it to map
                if(variables.find(node->var) != variables->end()) {
                    variables[node->var] = true;
                }
                else {
                    variables.emplace(node->var->toString(), true);
                }
                break;
            case typeid(ASTExpressionAssignment):
                // If variable is in map and live, set live status to false but do not remove assignment, otherwise, mark assignment for removal
                if(variables.find(node->left->toString()) != variables->end() && variables->at(node->left->toString())) {
                    variables[node->left->toString()] = false;
                    if(EliminateDeadCode(node->right, variables, functions)) node->right = node->right->right;
                    return false;
                }
                return true;
                break;
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
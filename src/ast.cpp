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
    // Keep track of variable live status.
    std::map<std::string, bool> variables;

    // Keep track of function live status.
    std::map<std::string, llvm::Value*> functions;

    for (auto& func : functionList)
    {
        //Get body of function and call EliminateDeadCode on it
        std::unique_ptr<ASTStatement> node = func->GetDef();
        EliminateDeadCode(node, variables, functions);
    }
}

private:

    bool AST::EliminateDeadCode(std::unique_ptr<ASTStatement> node, std::map<std::string, bool>& variables, std::map<std::string, llvm::Value*>& functions)
    {
        //Add case statements for all statement/expression types, with updates to live status and recursive calls on children as needed
        auto type = typeid(*node);
        switch(type) {
            case typeid(ASTStatementBlock):
                for(int i = node->statements->size(); i > 0; i--) {
                    EliminateDeadCode(node->*statements[i], variables, functions);
                }
                break;
            case typeid(AstStatementIf):
                EliminateDeadCode(node->elseStatement, variables, functions);
                EliminateDeadCode(node->thenStatement, variables, functions);
                EliminateDeadCode(node->condition, variables, functions);
                break;
            case typeid(ASTStatementWhile):
                EliminateDeadCode(node->thenStatement, variables, functions);
                EliminateDeadCode(node->condition, variables, functions);
                break;
            case typeid(ASTStatementFor):
                EliminateDeadCode(node->increment, variables, functions);
                EliminateDeadCode(node->body, variables, functions);
                EliminateDeadCode(node->condition, variables, functions);
                EliminateDeadCode(node->init, variables, functions);
                break;
            case typeid(ASTStatementReturn):
                EliminateDeadCode(node->returnExpression, variables, functions);
                break;
            case typeid(ASTExpressionAddition):
            case typeid(ASTExpressionSubtraction):
            case typeid(ASTExpressionMultiplication):
            case typeid(ASTExpressionDivision):
            case typeid(ASTExpressionAnd):
            case typeid(ASTExpressionOr):
            case typeid(ASTExpressionComparison):
                EliminateDeadCode(node->a1, variables, functions);
                EliminateDeadCode(node->a2, variables, functions);
                break;
            case typeid(ASTExpressionFloat2Int):
            case typeid(ASTExpressionInt2Float):
            case typeid(ASTExpressionInt2Bool):
            case typeid(ASTExpressionBool2Int):
            case typeid(ASTExpressionNegation):
                EliminateDeadCode(node->operand, variables, functions);
                break;
            case typeid(ASTExpressionCall):
                functions->emplace(node->callee->toString(), true);
                for(int i = node->arguments->size(); i > 0; i--) {
                    EliminateDeadCode(node->*arguments[i], variables, functions);
                }
                break;
            case typeid(ASTExpressionVar):
                // Finish implementation
                break;
            case typeid(ASTExpressionAssignment):
                if(variables->find(node->left->toString()) != variables->end() && variables->at(node->left->toString())) {
                    *variables[node->left->toString()] = false;
                    EliminateDeadCode(node->right, variables, functions);
                    return false;
                }
                return true;
                break;
            // Recursively process complex statements/expressions until reaching the lowest-level nodes (basic expressions and statements)
            // When you reach an expression, update live use information
            // When you reach an assignment, reference live use information and remove if necessary (possibly switch to boolean return type to facilitate removal?)
            // Use dynamic_cast to check node type?
        }
        return false;
    }
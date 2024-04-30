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

    for (auto& func : functionList)
    {
        // Keep track of variable live status.
        std::map<std::string, bool> variables;
        // Keep track of function live status.
        std::map<std::string, llvm::Value*> functions;
        if(func->GetDef()) {
            //Get body of function and call EliminateDeadCode on it
            std::unique_ptr<ASTStatement> node = func->GetDef();
            EliminateDeadCode(node, variables, functions);
            for(int i = node->stackVariables->size() - 1; i >= 0; i--) {
                if(node->*stackVariables[i], variables, functions) node->stackVariables->erase(i);
            }
        }
    }
}

private:

    bool AST::EliminateDeadCode(std::unique_ptr<ASTStatement> node, std::map<std::string, bool>& variables, std::map<std::string, llvm::Value*>& functions)
    {
        auto type = typeid(*node);
        switch(type) {
            case typeid(ASTStatementBlock):
                for(int i = node->statements->size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(node->*statements[i], variables, functions)) node->statements->erase(i);
                }
                break;
            case typeid(AstStatementIf):
                //Create new variable live tables, then merge at end
                if(EliminateDeadCode(node->elseStatement, variables, functions)) node->elseStatement = std::unique_ptr<ASTStatement>(nullptr);
                if(EliminateDeadCode(node->thenStatement, variables, functions)) node->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                if(EliminateDeadCode(node->condition, variables, functions)) node->condition = node->condition->right;
                break;
            case typeid(ASTStatementWhile):
                //Create new variable live table, then merge at end
                if(EliminateDeadCode(node->thenStatement, variables, functions)) node->thenStatement = std::unique_ptr<ASTStatement>(nullptr);
                if(EliminateDeadCode(node->condition, variables, functions)) node->condition = node->condition->right;
                break;
            case typeid(ASTStatementFor):
                //Create new variable live table, then merge at end
                EliminateDeadCode(node->increment, variables, functions) //Perform some measure of simplification on increment?
                if(EliminateDeadCode(node->body, variables, functions)) node->body = std::unique_ptr<ASTStatement>(nullptr);
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
                functions->emplace(node->callee->toString(), true);
                for(int i = node->arguments->size() - 1; i >= 0; i--) {
                    if(EliminateDeadCode(node->*arguments[i], variables, functions)) node->statements->erase(i);
                }
                break;
            case typeid(ASTExpressionVar):
                if(variables->find(node->var) != variables->end()) {
                    *variables[node->var] = true;
                }
                break;
            case typeid(ASTExpressionAssignment):
                if(variables->find(node->left->toString()) != variables->end() && variables->at(node->left->toString())) {
                    *variables[node->left->toString()] = false;
                    if(EliminateDeadCode(node->right, variables, functions)) node->right = node->right->right;
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
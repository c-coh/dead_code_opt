#include "bool.h"

std::unique_ptr<VarType> ASTExpressionBool::ReturnType(ASTFunction& func)
{
    return VarTypeSimple::BoolType.Copy(); // Of course we are returning a bool, what else would it be.
}

bool ASTExpressionBool::IsLValue(ASTFunction& func)
{
    return false; // It's a constant, of course it's not an L-Value.
}

llvm::Value* ASTExpressionBool::Compile(llvm::IRBuilder<>& builder, ASTFunction& func)
{
    return llvm::ConstantInt::get(VarTypeSimple::BoolType.GetLLVMType(builder.getContext()), value); // Simply just create a bool constant to return.
}

std::string ASTExpressionBool::ToString(const std::string& prefix)
{
    return std::to_string(value) + "\n";
}
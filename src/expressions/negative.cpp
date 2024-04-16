#include "negative.h"

std::unique_ptr<VarType> ASTExpressionNegation::ReturnType(ASTFunction& func)
{
    return operand->ReturnType(func);
}

bool ASTExpressionNegation::IsLValue(ASTFunction& func)
{
    return false; // If we are negating a value, it must be a usable R-Value. Taking the negation just results in an R-Value.
}

llvm::Value* ASTExpressionNegation::Compile(llvm::IRBuilder<>& builder, ASTFunction& func)
{
    // Compile the values as needed. Remember, we can only do operations on R-Values.
    auto retType = ReturnType(func);
    if (retType->Equals(&VarTypeSimple::IntType)) // Do negation on integer operand since we return an int.
        return builder.CreateSub(llvm::ConstantInt::get(VarTypeSimple::IntType.GetLLVMType(builder.getContext()), 0), operand->CompileRValue(builder, func));
    else if (retType->Equals(&VarTypeSimple::FloatType)) // Do negation on floating point operand since we return a float.
        return builder.CreateFNeg(operand->CompileRValue(builder, func));
    else // Call to return type should make this impossible, but best to keep it here just in case of a bug.
        throw std::runtime_error("ERROR: Can not perform negation! Is the input either an int or a float?");
}

std::string ASTExpressionNegation::ToString(const std::string& prefix)
{
    std::string ret = "(-)\n";
    ret += prefix + "└──" + operand->ToString(prefix + "   ");
    return ret;
}
#pragma once

#include "../expression.h"
#include "../statement.h"

// For a for loop statement.
class ASTStatementFor : public ASTStatement
{

public:
    // Loop body to execute.
    std::unique_ptr<ASTStatement> body;
    
    // Initial statement to execute.
    std::unique_ptr<ASTStatement> init;

    // Condition to check.
    std::unique_ptr<ASTExpression> condition;

    // Increment statement to execute.
    std::unique_ptr<ASTStatement> increment;
    
    // Create a new for statement.
    // body: Statement to execute while the condition is true.
    // init: Statement to execute on loop start.
    // condition: Condition to check.
    // increment: Statement to execute after each iteration.
    ASTStatementFor(std::unique_ptr<ASTStatement> body, std::unique_ptr<ASTStatement> init, std::unique_ptr<ASTExpression> condition, std::unique_ptr<ASTStatement> increment) : body(std::move(body)), init(std::move(init)), condition(std::move(condition)), increment(std::move(increment)) {}

    // Create a new for statement.
    // body: Statement to execute while the condition is true.
    // init: Statement to execute on loop start.
    // condition: Condition to check.
    // increment: Statement to execute after each iteration.
    static auto Create(std::unique_ptr<ASTStatement> body, std::unique_ptr<ASTStatement> init, std::unique_ptr<ASTExpression> condition, std::unique_ptr<ASTStatement> increment)
    {
        return std::make_unique<ASTStatementFor>(std::move(body), std::move(init), std::move(condition), std::move(increment));
    }

    // Virtual functions. See base class for details.
    virtual std::unique_ptr<VarType> StatementReturnType(ASTFunction& func) override;
    virtual void Compile(llvm::Module& mod, llvm::IRBuilder<>& builder, ASTFunction& func) override;
    virtual std::string ToString(const std::string& prefix) override;

};
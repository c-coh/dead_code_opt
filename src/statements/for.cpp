#include "for.h"

#include "../function.h"

std::unique_ptr<VarType> ASTStatementFor::StatementReturnType(ASTFunction& func)
{

    if(init && init->StatementReturnType(func)) {
        return init->StatementReturnType(func);
    }
    else {
        // It is completely possible for a for loop's condition to never be true, so even if does return something it's not confirmed.
        return nullptr;
    }

}

void ASTStatementFor::Compile(llvm::Module& mod, llvm::IRBuilder<>& builder, ASTFunction& func)
{

    /*

        A for loop can be desugared to basic blocks. Take the following example:

            for (init; condition; increment)
            {
                doThing();
            }

        This is really just another way of saying:

            entry:
                init;
            
            forLoop:
                if (condition) goto forLoopBody else goto forLoopEnd;

            forLoopBody:
                doThing();
                goto forLoopContinue;
            
            forLoopContinue:
                increment;
                goto forLoop;

            forLoopEnd:
                ...

    */

    // Create the basic blocks.
    auto* funcVal = (llvm::Function*)func.GetVariableValue(func.name);
    auto forLoopEntry = llvm::BasicBlock::Create(builder.getContext(), "forLoopEntry", funcVal);
    auto forLoop = llvm::BasicBlock::Create(builder.getContext(), "forLoop", funcVal);
    auto forLoopBody = llvm::BasicBlock::Create(builder.getContext(), "forLoopBody", funcVal);
    auto forLoopContinue = llvm::BasicBlock::Create(builder.getContext(), "forLoopContinue", funcVal);
    auto forLoopEnd = llvm::BasicBlock::Create(builder.getContext(), "forLoopEnd", funcVal);

    // Jump to the init statement.
    builder.CreateBr(forLoopEntry);

    // Compile init statement and jump to the for loop.
    builder.SetInsertPoint(forLoopEntry);
    // If init statement is not null, then add it to entry block.
    if(init) {
        init->Compile(mod, builder, func);
        // If init statement does not return, continue creating loop.
        if (!init->StatementReturnType(func)) builder.CreateBr(forLoop);
    }
    // Otherwise, skip init statement.
    else {
        builder.CreateBr(forLoop);
    }

    // Compile condition and jump to the right block.
    builder.SetInsertPoint(forLoop);
    // If condition is not null, then add conditional jump into loop.
    if(condition) {
        auto conditionVal = condition->CompileRValue(builder, func);
        builder.CreateCondBr(conditionVal, forLoopBody, forLoopEnd);
    }
    // Otherwise, create unconditional jump.
    else {
        builder.CreateBr(forLoopBody);
    }

    // Compile the body. Note that we need to not create a jump if there is a return.
    builder.SetInsertPoint(forLoopBody);
    body->Compile(mod, builder, func);
    // If body does not return, continue creating loop.
    if (!body->StatementReturnType(func)) builder.CreateBr(forLoopContinue);

    // Compile inc statement and jump to the for loop.
    builder.SetInsertPoint(forLoopContinue);
    // If increment statement is not null, then add it to continue block.
    if(increment) {
        increment->Compile(mod, builder, func);
        // If increment statement does not return, continue creating loop.
        if (!increment->StatementReturnType(func)) builder.CreateBr(forLoop);
    }
    // Otherwise, skip increment statement.
    else {
        builder.CreateBr(forLoop);
    }

    // Continue from the end of the created for loop.
    builder.SetInsertPoint(forLoopEnd);

}

std::string ASTStatementFor::ToString(const std::string& prefix)
{
    std::string output = "for\n";
    output += prefix + "├──" + body->ToString(prefix + "│  ");
    output += prefix + "├──" + init->ToString(prefix + "│  ");
    output += prefix + "├──" + condition->ToString(prefix + "│  ");
    output += prefix + "└──" + increment->ToString(prefix + "   ");
    return output;
}
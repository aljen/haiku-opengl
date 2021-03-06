/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <cstdio>
#include "instructionssoa.h"

#include "storagesoa.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_memory.h"

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Attributes.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>


#include <iostream>


/* disable some warnings. this file is autogenerated */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
using namespace llvm;
#include "gallivmsoabuiltins.cpp"
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wunused-variable"
#endif

InstructionsSoa::InstructionsSoa(llvm::Module *mod, llvm::Function *func,
                                 llvm::BasicBlock *block, StorageSoa *storage)
   : m_builder(block),
     m_storage(storage),
     m_idx(0)
{
   createFunctionMap();
   createBuiltins();
}

const char * InstructionsSoa::name(const char *prefix) const
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

llvm::Value * InstructionsSoa::vectorFromVals(llvm::Value *x, llvm::Value *y,
                                              llvm::Value *z, llvm::Value *w)
{
   VectorType  *vectorType = VectorType::get(Type::FloatTy, 4);
   Constant *constVector = Constant::getNullValue(vectorType);
   Value *res = m_builder.CreateInsertElement(constVector, x,
                                              m_storage->constantInt(0),
                                              name("vecx"));
   res = m_builder.CreateInsertElement(res, y, m_storage->constantInt(1),
                               name("vecxy"));
   res = m_builder.CreateInsertElement(res, z, m_storage->constantInt(2),
                               name("vecxyz"));
   if (w)
      res = m_builder.CreateInsertElement(res, w, m_storage->constantInt(3),
                                          name("vecxyzw"));
   return res;
}

void InstructionsSoa::end()
{
   m_builder.CreateRetVoid();
}

std::vector<llvm::Value*> InstructionsSoa::extractVector(llvm::Value *vector)
{
   std::vector<llvm::Value*> res(4);
   res[0] = m_builder.CreateExtractElement(vector,
                                           m_storage->constantInt(0),
                                           name("extract1X"));
   res[1] = m_builder.CreateExtractElement(vector,
                                           m_storage->constantInt(1),
                                           name("extract2X"));
   res[2] = m_builder.CreateExtractElement(vector,
                                           m_storage->constantInt(2),
                                           name("extract3X"));
   res[3] = m_builder.CreateExtractElement(vector,
                                           m_storage->constantInt(3),
                                           name("extract4X"));

   return res;
}

llvm::IRBuilder<>* InstructionsSoa::getIRBuilder()
{
   return &m_builder;
}

void InstructionsSoa::createFunctionMap()
{
   m_functionsMap[TGSI_OPCODE_ABS]   = "abs";
   m_functionsMap[TGSI_OPCODE_DP3]   = "dp3";
   m_functionsMap[TGSI_OPCODE_DP4]   = "dp4";
   m_functionsMap[TGSI_OPCODE_MIN]   = "min";
   m_functionsMap[TGSI_OPCODE_MAX]   = "max";
   m_functionsMap[TGSI_OPCODE_POWER] = "pow";
   m_functionsMap[TGSI_OPCODE_LIT]   = "lit";
   m_functionsMap[TGSI_OPCODE_RSQ]   = "rsq";
   m_functionsMap[TGSI_OPCODE_SLT]   = "slt";
}

void InstructionsSoa::createDependencies()
{
   {
      std::vector<std::string> powDeps(2);
      powDeps[0] = "powf";
      powDeps[1] = "powvec";
      m_builtinDependencies["pow"] = powDeps;
   }
   {
      std::vector<std::string> absDeps(2);
      absDeps[0] = "fabsf";
      absDeps[1] = "absvec";
      m_builtinDependencies["abs"] = absDeps;
   }
   {
      std::vector<std::string> maxDeps(1);
      maxDeps[0] = "maxvec";
      m_builtinDependencies["max"] = maxDeps;
   }
   {
      std::vector<std::string> minDeps(1);
      minDeps[0] = "minvec";
      m_builtinDependencies["min"] = minDeps;
   }
   {
      std::vector<std::string> litDeps(4);
      litDeps[0] = "minvec";
      litDeps[1] = "maxvec";
      litDeps[2] = "powf";
      litDeps[3] = "powvec";
      m_builtinDependencies["lit"] = litDeps;
   }
   {
      std::vector<std::string> rsqDeps(4);
      rsqDeps[0] = "sqrtf";
      rsqDeps[1] = "sqrtvec";
      rsqDeps[2] = "fabsf";
      rsqDeps[3] = "absvec";
      m_builtinDependencies["rsq"] = rsqDeps;
   }
}

llvm::Function * InstructionsSoa::function(int op)
{
    if (m_functions.find(op) != m_functions.end())
       return m_functions[op];

    std::string name = m_functionsMap[op];

    std::cout <<"For op = "<<op<<", func is '"<<name<<"'"<<std::endl;

    std::vector<std::string> deps = m_builtinDependencies[name];
    for (unsigned int i = 0; i < deps.size(); ++i) {
       llvm::Function *func = m_builtins->getFunction(deps[i]);
       std::cout <<"\tinjecting dep = '"<<func->getName()<<"'"<<std::endl;
       injectFunction(func);
    }

    llvm::Function *originalFunc = m_builtins->getFunction(name);
    injectFunction(originalFunc, op);
    return m_functions[op];
}

llvm::Module * InstructionsSoa::currentModule() const
{
   BasicBlock *block = m_builder.GetInsertBlock();
   if (!block || !block->getParent())
      return 0;

   return block->getParent()->getParent();
}

void InstructionsSoa::createBuiltins()
{
   std::string ErrMsg;
   MemoryBuffer *buffer = MemoryBuffer::getMemBuffer(
      (const char*)&soabuiltins_data[0],
      (const char*)&soabuiltins_data[Elements(soabuiltins_data) - 1]);
   m_builtins = ParseBitcodeFile(buffer, &ErrMsg);
   std::cout<<"Builtins created at "<<m_builtins<<" ("<<ErrMsg<<")"<<std::endl;
   assert(m_builtins);
   createDependencies();
}


std::vector<llvm::Value*> InstructionsSoa::abs(const std::vector<llvm::Value*> in1)
{
   llvm::Function *func = function(TGSI_OPCODE_ABS);
   return callBuiltin(func, in1);
}

std::vector<llvm::Value*> InstructionsSoa::add(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   res[0] = m_builder.CreateAdd(in1[0], in2[0], name("addx"));
   res[1] = m_builder.CreateAdd(in1[1], in2[1], name("addy"));
   res[2] = m_builder.CreateAdd(in1[2], in2[2], name("addz"));
   res[3] = m_builder.CreateAdd(in1[3], in2[3], name("addw"));

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::arl(const std::vector<llvm::Value*> in)
{
   std::vector<llvm::Value*> res(4);

   //Extract x's
   llvm::Value *x1 = m_builder.CreateExtractElement(in[0],
                                                    m_storage->constantInt(0),
                                                    name("extractX"));
   //cast it to an unsigned int
   x1 = m_builder.CreateFPToUI(x1, IntegerType::get(32), name("x1IntCast"));

   res[0] = x1;//vectorFromVals(x1, x2, x3, x4);
   //only x is valid. the others shouldn't be necessary
   /*
   res[1] = Constant::getNullValue(m_floatVecType);
   res[2] = Constant::getNullValue(m_floatVecType);
   res[3] = Constant::getNullValue(m_floatVecType);
   */

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::dp3(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_DP3);
   return callBuiltin(func, in1, in2);
}

std::vector<llvm::Value*> InstructionsSoa::lit(const std::vector<llvm::Value*> in)
{
   llvm::Function *func = function(TGSI_OPCODE_LIT);
   return callBuiltin(func, in);
}

std::vector<llvm::Value*> InstructionsSoa::madd(const std::vector<llvm::Value*> in1,
                                                const std::vector<llvm::Value*> in2,
                                                const std::vector<llvm::Value*> in3)
{
   std::vector<llvm::Value*> res = mul(in1, in2);
   return add(res, in3);
}

std::vector<llvm::Value*> InstructionsSoa::max(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_MAX);
   return callBuiltin(func, in1, in2);
}

std::vector<llvm::Value*> InstructionsSoa::min(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_MIN);
   return callBuiltin(func, in1, in2);
}

std::vector<llvm::Value*> InstructionsSoa::mul(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   res[0] = m_builder.CreateMul(in1[0], in2[0], name("mulx"));
   res[1] = m_builder.CreateMul(in1[1], in2[1], name("muly"));
   res[2] = m_builder.CreateMul(in1[2], in2[2], name("mulz"));
   res[3] = m_builder.CreateMul(in1[3], in2[3], name("mulw"));

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::pow(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_POWER);
   return callBuiltin(func, in1, in2);
}

std::vector<llvm::Value*> InstructionsSoa::rsq(const std::vector<llvm::Value*> in)
{
   llvm::Function *func = function(TGSI_OPCODE_RSQ);
   return callBuiltin(func, in);
}

std::vector<llvm::Value*> InstructionsSoa::slt(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_SLT);
   return callBuiltin(func, in1, in2);
}

std::vector<llvm::Value*> InstructionsSoa::sub(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   std::vector<llvm::Value*> res(4);

   res[0] = m_builder.CreateSub(in1[0], in2[0], name("subx"));
   res[1] = m_builder.CreateSub(in1[1], in2[1], name("suby"));
   res[2] = m_builder.CreateSub(in1[2], in2[2], name("subz"));
   res[3] = m_builder.CreateSub(in1[3], in2[3], name("subw"));

   return res;
}

void checkFunction(Function *func)
{
   for (Function::const_iterator BI = func->begin(), BE = func->end();
        BI != BE; ++BI) {
      const BasicBlock &BB = *BI;
      for (BasicBlock::const_iterator II = BB.begin(), IE = BB.end();
           II != IE; ++II) {
         const Instruction &I = *II;
         std::cout<< "Instr = "<<I;
         for (unsigned op = 0, E = I.getNumOperands(); op != E; ++op) {
            const Value *Op = I.getOperand(op);
            std::cout<< "\top = "<<Op<<"("<<op<<")"<<std::endl;
            //I->setOperand(op, V);
  }
      }
   }
}

llvm::Value * InstructionsSoa::allocaTemp()
{
   VectorType *vector   = VectorType::get(Type::FloatTy, 4);
   ArrayType  *vecArray = ArrayType::get(vector, 4);
   AllocaInst *alloca = new AllocaInst(vecArray, name("tmpRes"),
                                       m_builder.GetInsertBlock());

   std::vector<Value*> indices;
   indices.push_back(m_storage->constantInt(0));
   indices.push_back(m_storage->constantInt(0));
   GetElementPtrInst *getElem = GetElementPtrInst::Create(alloca,
                                                          indices.begin(),
                                                          indices.end(),
                                                          name("allocaPtr"),
                                                          m_builder.GetInsertBlock());
   return getElem;
}

std::vector<llvm::Value*> InstructionsSoa::allocaToResult(llvm::Value *allocaPtr)
{
   GetElementPtrInst *xElemPtr =  GetElementPtrInst::Create(allocaPtr,
                                                            m_storage->constantInt(0),
                                                            name("xPtr"),
                                                            m_builder.GetInsertBlock());
   GetElementPtrInst *yElemPtr =  GetElementPtrInst::Create(allocaPtr,
                                                            m_storage->constantInt(1),
                                                            name("yPtr"),
                                                            m_builder.GetInsertBlock());
   GetElementPtrInst *zElemPtr =  GetElementPtrInst::Create(allocaPtr,
                                                            m_storage->constantInt(2),
                                                            name("zPtr"),
                                                            m_builder.GetInsertBlock());
   GetElementPtrInst *wElemPtr =  GetElementPtrInst::Create(allocaPtr,
                                                            m_storage->constantInt(3),
                                                            name("wPtr"),
                                                            m_builder.GetInsertBlock());

   std::vector<llvm::Value*> res(4);
   res[0] = new LoadInst(xElemPtr, name("xRes"), false, m_builder.GetInsertBlock());
   res[1] = new LoadInst(yElemPtr, name("yRes"), false, m_builder.GetInsertBlock());
   res[2] = new LoadInst(zElemPtr, name("zRes"), false, m_builder.GetInsertBlock());
   res[3] = new LoadInst(wElemPtr, name("wRes"), false, m_builder.GetInsertBlock());

   return res;
}

std::vector<llvm::Value*> InstructionsSoa::dp4(const std::vector<llvm::Value*> in1,
                                               const std::vector<llvm::Value*> in2)
{
   llvm::Function *func = function(TGSI_OPCODE_DP4);
   return callBuiltin(func, in1, in2);
}

std::vector<Value*> InstructionsSoa::callBuiltin(llvm::Function *func, const std::vector<llvm::Value*> in1)
{
   std::vector<Value*> params;

   llvm::Value *allocaPtr = allocaTemp();
   params.push_back(allocaPtr);
   params.push_back(in1[0]);
   params.push_back(in1[1]);
   params.push_back(in1[2]);
   params.push_back(in1[3]);
   CallInst *call = m_builder.CreateCall(func, params.begin(), params.end());
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);

   return allocaToResult(allocaPtr);
}

std::vector<Value*> InstructionsSoa::callBuiltin(llvm::Function *func, const std::vector<llvm::Value*> in1,
                                                 const std::vector<llvm::Value*> in2)
{
   std::vector<Value*> params;

   llvm::Value *allocaPtr = allocaTemp();
   params.push_back(allocaPtr);
   params.push_back(in1[0]);
   params.push_back(in1[1]);
   params.push_back(in1[2]);
   params.push_back(in1[3]);
   params.push_back(in2[0]);
   params.push_back(in2[1]);
   params.push_back(in2[2]);
   params.push_back(in2[3]);
   CallInst *call = m_builder.CreateCall(func, params.begin(), params.end());
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);

   return allocaToResult(allocaPtr);
}

std::vector<Value*> InstructionsSoa::callBuiltin(llvm::Function *func, const std::vector<llvm::Value*> in1,
                                                 const std::vector<llvm::Value*> in2,
                                                 const std::vector<llvm::Value*> in3)
{
   std::vector<Value*> params;

   llvm::Value *allocaPtr = allocaTemp();
   params.push_back(allocaPtr);
   params.push_back(in1[0]);
   params.push_back(in1[1]);
   params.push_back(in1[2]);
   params.push_back(in1[3]);
   params.push_back(in2[0]);
   params.push_back(in2[1]);
   params.push_back(in2[2]);
   params.push_back(in2[3]);
   params.push_back(in3[0]);
   params.push_back(in3[1]);
   params.push_back(in3[2]);
   params.push_back(in3[3]);
   CallInst *call = m_builder.CreateCall(func, params.begin(), params.end());
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);

   return allocaToResult(allocaPtr);
}

void InstructionsSoa::injectFunction(llvm::Function *originalFunc, int op)
{
   assert(originalFunc);
   std::cout << "injecting function originalFunc " <<originalFunc->getName() <<std::endl;
   if (op != TGSI_OPCODE_LAST) {
      /* in this case it's possible the function has been already
       * injected as part of the dependency chain, which gets
       * injected below */
      llvm::Function *func = currentModule()->getFunction(originalFunc->getName());
      if (func) {
         m_functions[op] = func;
         return;
      }
   }
   llvm::Function *func = 0;
   if (originalFunc->isDeclaration()) {
      func = Function::Create(originalFunc->getFunctionType(), GlobalValue::ExternalLinkage,
                              originalFunc->getName(), currentModule());
      func->setCallingConv(CallingConv::C);
      const AttrListPtr pal;
      func->setAttributes(pal);
      currentModule()->dump();
   } else {
      DenseMap<const Value*, Value *> val;
      val[m_builtins->getFunction("fabsf")] = currentModule()->getFunction("fabsf");
      val[m_builtins->getFunction("powf")] = currentModule()->getFunction("powf");
      val[m_builtins->getFunction("sqrtf")] = currentModule()->getFunction("sqrtf");
      func = CloneFunction(originalFunc, val);
#if 0
      std::cout <<" replacing "<<m_builtins->getFunction("powf")
                <<", with " <<currentModule()->getFunction("powf")<<std::endl;
      std::cout<<"1111-------------------------------"<<std::endl;
      checkFunction(originalFunc);
      std::cout<<"2222-------------------------------"<<std::endl;
      checkFunction(func);
      std::cout <<"XXXX = " <<val[m_builtins->getFunction("powf")]<<std::endl;
#endif
      currentModule()->getFunctionList().push_back(func);
   }
   if (op != TGSI_OPCODE_LAST) {
      m_functions[op] = func;
   }
}



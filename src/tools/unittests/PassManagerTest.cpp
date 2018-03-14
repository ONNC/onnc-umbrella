//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <skypat/skypat.h>
#include <onnc/Core/Pass.h>
#include <onnc/Core/ModulePass.h>
#include <onnc/Core/PassRegistry.h>
#include <onnc/Core/PassSupport.h>
#include <onnc/Core/AnalysisUsage.h>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>

using namespace skypat;
using namespace onnc;

namespace {

/** \class A
 */
class A : public ModulePass
{
public:
  static char ID;

  A() : ModulePass(ID) { }

  bool runOnModule(Module &pModule) { return true; }

  StringRef getPassName() const { return "A"; }
};

char A::ID = 0;

/** \class B
 */
class B : public ModulePass
{
public:
  static char ID;

public:
  B() : ModulePass(ID) { }

  bool runOnModule(Module &pModule) { return true; }

  void getAnalysisUsage(AnalysisUsage& pUsage) const {
    pUsage.addRequiredID(A::ID);
  }

  StringRef getPassName() const { return "B"; }
};

char B::ID = 0;

/** \class C
 */
class C : public ModulePass
{
public:
  static char ID;
  C() : ModulePass(ID), data(0x12) { }
  bool runOnModule(Module &pModule) { return true; }

  void getAnalysisUsage(AnalysisUsage& pUsage) const {
    pUsage.addRequiredID(A::ID);
    pUsage.addRequiredID(B::ID);
  }

  StringRef getPassName() const { return "C"; }

  int data;
};

char C::ID = 0;

} // anonymous namespace

INITIALIZE_PASS(A, "A")
INITIALIZE_PASS(B, "B")
INITIALIZE_PASS(C, "C")

//===----------------------------------------------------------------------===//
// PassManagerTest
//===----------------------------------------------------------------------===//
SKYPAT_F(PassManagerTest, add_passes)
{
  PassRegistry registry;
  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());

  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  EXPECT_EQ(registry.numOfPasses(), 3);
  EXPECT_FALSE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, add_duplicated_passes)
{
  PassRegistry registry;
  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());

  InitializeAPass(registry);
  InitializeAPass(registry);
  InitializeAPass(registry);

  EXPECT_EQ(registry.numOfPasses(), 1);
  EXPECT_FALSE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, delete_passes)
{
  PassRegistry registry;
  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  ASSERT_EQ(registry.numOfPasses(), 3);
  ASSERT_FALSE(registry.isEmpty());

  registry.clear();

  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, query_passes)
{
  PassRegistry registry;
  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  ASSERT_EQ(registry.numOfPasses(), 3);
  ASSERT_FALSE(registry.isEmpty());

  const PassInfo* info = registry.getPassInfo(&C::ID);
  EXPECT_TRUE(info->getPassName().equals("C"));
  Pass* pass = info->createPass();
  EXPECT_EQ(((C*)pass)->data, 0x12);

  PassManager pm(registry);
  pm.add(pass);
  pm.add(pass);
  Module module;
  pm.run(module);
}

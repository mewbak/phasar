/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <initializer_list>
#include <set>
#include <string>
#include <vector>

#include <llvm/IR/IntrinsicInst.h>

#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/BranchSwitchInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/CallToRetFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/CheckOperandsFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/GEPInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/GenerateFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/IdentityFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/MapTaintedValuesToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/MapTaintedValuesToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/MemSetInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/MemTransferInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/PHINodeFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/ReturnInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/StoreInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/VAEndInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/VAStartInstFlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/LcovRetValWriter.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/LcovWriter.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/LineNumberWriter.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/TraceStats.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/TraceStatsWriter.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Utils/DataFlowUtils.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSEnvironmentVariableTracing.h>

namespace psr {

IFDSEnvironmentVariableTracing::IFDSEnvironmentVariableTracing(
    LLVMBasedICFG &ICFG, std::vector<std::string> EntryPoints = {"main"})
    : DefaultIFDSTabulationProblem<const llvm::Instruction *, ExtendedValue,
                                   const llvm::Function *, LLVMBasedICFG &>(
          ICFG),
      EntryPoints(EntryPoints),
      taintConfig(std::initializer_list<
                      TaintConfiguration<ExtendedValue>::SourceFunction>(),
                  std::initializer_list<
                      TaintConfiguration<ExtendedValue>::SinkFunction>()) {
  for (auto i : DataFlowUtils::getTaintedFunctions()) {
    taintConfig.addSource(
        TaintConfiguration<ExtendedValue>::SourceFunction(i, false));
  }
  for (auto i : DataFlowUtils::getBlacklistedFunctions()) {
    taintConfig.addSink(TaintConfiguration<ExtendedValue>::SinkFunction(
        i, TaintConfiguration<ExtendedValue>::None()));
  }
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
  this->solver_config.computeValues = true; // do not touch
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSEnvironmentVariableTracing::getNormalFlowFunction(
    const llvm::Instruction *currentInst,
    const llvm::Instruction *successorInst) {
  if (taintConfig.isSource(currentInst)) {
    // TODO: generate current inst wrapped in an ExtendedValue
  }

  if (taintConfig.isSink(currentInst)) {
    // TODO: report leak as done for the functions
  }

  if (DataFlowUtils::isReturnValue(currentInst, successorInst))
    return std::make_shared<ReturnInstFlowFunction>(successorInst, traceStats,
                                                    zeroValue());

  if (llvm::isa<llvm::StoreInst>(currentInst))
    return std::make_shared<StoreInstFlowFunction>(currentInst, traceStats,
                                                   zeroValue());

  if (llvm::isa<llvm::BranchInst>(currentInst) ||
      llvm::isa<llvm::SwitchInst>(currentInst))
    return std::make_shared<BranchSwitchInstFlowFunction>(
        currentInst, traceStats, zeroValue());

  if (llvm::isa<llvm::GetElementPtrInst>(currentInst))
    return std::make_shared<GEPInstFlowFunction>(currentInst, traceStats,
                                                 zeroValue());

  if (llvm::isa<llvm::PHINode>(currentInst))
    return std::make_shared<PHINodeFlowFunction>(currentInst, traceStats,
                                                 zeroValue());

  if (DataFlowUtils::isCheckOperandsInst(currentInst))
    return std::make_shared<CheckOperandsFlowFunction>(currentInst, traceStats,
                                                       zeroValue());

  return std::make_shared<IdentityFlowFunction>(currentInst, traceStats,
                                                zeroValue());
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSEnvironmentVariableTracing::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  return std::make_shared<MapTaintedValuesToCallee>(
      llvm::cast<llvm::CallInst>(callStmt), destMthd, traceStats, zeroValue());
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSEnvironmentVariableTracing::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  return std::make_shared<MapTaintedValuesToCaller>(
      llvm::cast<llvm::CallInst>(callSite),
      llvm::cast<llvm::ReturnInst>(exitStmt), traceStats, zeroValue());
}

/*
 * Every fact that was valid before call to function will be handled here
 * right after the function call has returned... We would like to keep all
 * previously generated facts. Facts from the returning functions are
 * handled in getRetFlowFunction.
 */
std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSEnvironmentVariableTracing::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    std::set<const llvm::Function *> callees) {
  /*
   * It is important to wrap the identity call here. Consider the following
   * example:
   *
   * br i1 %cmp, label %cond.true, label %cond.false
   * cond.true:
   *   %call1 = call i32 (...) @foo()
   *   br label %cond.end
   * ...
   * cond.end:
   * %cond = phi i32 [ %call1, %cond.true ], [ 1, %cond.false ]
   *
   * Because we are in a tainted branch we must push %call1 to facts. We cannot
   * do that in the getSummaryFlowFunction() because if we return a flow
   * function we never follow the function. If we intercept here the call
   * instruction will be pushed when the flow function is called with the branch
   * instruction fact.
   */
  return std::make_shared<CallToRetFlowFunction>(callSite, traceStats,
                                                 zeroValue());
}

/*
 * If we return sth. different than a nullptr the callee will not be traversed.
 * Instead facts according to the defined flow function will be taken into
 * account.
 */
std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSEnvironmentVariableTracing::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  const auto destMthdName = destMthd->getName();

  /*
   * We exclude function ptr calls as they will be applied to every
   * function matching its signature (@see LLVMBasedICFG.cpp:217).
   */
  const auto callInst = llvm::cast<llvm::CallInst>(callStmt);
  bool isStaticCallSite = callInst->getCalledFunction();
  if (!isStaticCallSite)
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  zeroValue());

  /*
   * Exclude blacklisted functions here.
   */

  if (taintConfig.isSink(destMthdName))
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  zeroValue());

  /*
   * Intrinsics.
   */
  if (llvm::isa<llvm::MemTransferInst>(callStmt))
    return std::make_shared<MemTransferInstFlowFunction>(callStmt, traceStats,
                                                         zeroValue());

  if (llvm::isa<llvm::MemSetInst>(callStmt))
    return std::make_shared<MemSetInstFlowFunction>(callStmt, traceStats,
                                                    zeroValue());

  if (llvm::isa<llvm::VAStartInst>(callStmt))
    return std::make_shared<VAStartInstFlowFunction>(callStmt, traceStats,
                                                     zeroValue());

  if (llvm::isa<llvm::VAEndInst>(callStmt))
    return std::make_shared<VAEndInstFlowFunction>(callStmt, traceStats,
                                                   zeroValue());

  /*
   * Provide summary for tainted functions.
   */
  if (taintConfig.isSource(destMthdName))
    return std::make_shared<GenerateFlowFunction>(callStmt, traceStats,
                                                  zeroValue());

  /*
   * Skip all (other) declarations.
   */
  bool isDeclaration = destMthd->isDeclaration();
  if (isDeclaration)
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  zeroValue());

  /*
   * Follow call -> getCallFlowFunction()
   */
  return nullptr;
}

std::map<const llvm::Instruction *, std::set<ExtendedValue>>
IFDSEnvironmentVariableTracing::initialSeeds() {
  std::map<const llvm::Instruction *, std::set<ExtendedValue>> seedMap;

  for (const auto &entryPoint : this->EntryPoints) {
    if (taintConfig.isSink(entryPoint))
      continue;

    seedMap.insert(std::make_pair(&icfg.getMethod(entryPoint)->front().front(),
                                  std::set<ExtendedValue>({zeroValue()})));
  }

  return seedMap;
}

void IFDSEnvironmentVariableTracing::printIFDSReport(
    std::ostream &os,
    SolverResults<const llvm::Instruction *, ExtendedValue, BinaryDomain>
        &solverResults) {
  const std::string lcovTraceFile =
      DataFlowUtils::getTraceFilenamePrefix(EntryPoints.front()) + "-trace.txt";
  const std::string lcovRetValTraceFile =
      DataFlowUtils::getTraceFilenamePrefix(EntryPoints.front()) +
      "-return-value-trace.txt";

#ifdef DEBUG_BUILD
  // Write line number trace (for tests only)
  LineNumberWriter lineNumberWriter(traceStats, "line-numbers.txt");
  lineNumberWriter.write();
#endif

  // Write lcov trace
  LcovWriter lcovWriter(traceStats, lcovTraceFile);
  lcovWriter.write();

  // Write lcov return value trace
  LcovRetValWriter lcovRetValWriter(traceStats, lcovRetValTraceFile);
  lcovRetValWriter.write();
}

} // namespace psr

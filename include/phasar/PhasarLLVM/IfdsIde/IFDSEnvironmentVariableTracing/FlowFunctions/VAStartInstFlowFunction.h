/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef VASTARTINSTFLOWFUNCTION_H
#define VASTARTINSTFLOWFUNCTION_H

#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/FlowFunctions/FlowFunctionBase.h>

namespace psr {

class VAStartInstFlowFunction : public FlowFunctionBase {
public:
  VAStartInstFlowFunction(const llvm::Instruction *_currentInst,
                          TraceStats &_traceStats, ExtendedValue _zeroValue)
      : FlowFunctionBase(_currentInst, _traceStats, _zeroValue) {}
  ~VAStartInstFlowFunction() override = default;

  std::set<ExtendedValue> computeTargetsExt(ExtendedValue &fact) override;
};

} // namespace psr

#endif // VASTARTINSTFLOWFUNCTION_H

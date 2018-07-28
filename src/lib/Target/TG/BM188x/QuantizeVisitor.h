//===- QuantizeVisitor.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNC_TARGET_BM188X_QUANTIZE_VISITOR_H
#define ONNC_TARGET_BM188X_QUANTIZE_VISITOR_H
#include "BM188xVisitor.h"
#include <onnc/Target/TG/BM188x/common_calibration2.pb.h>

namespace onnc {
namespace BM188X {

class QuantizeVisitor : public BM188xVisitor
{
public:
  QuantizeVisitor(const tg::bm1880::LayerCalibrationParameter & pLCP);

  void visit(BM188X::AveragePool& pAveragePool) override;

private:
  const tg::bm1880::LayerCalibrationParameter &m_LayerCtable;
};

} // namespace BM188X
} // namespace onnc

#endif

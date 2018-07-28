//===- QuantizeVisitor.cpp ------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "QuantizeVisitor.h"
#include "Compute/AveragePool.h"

using namespace onnc;
using namespace onnc::BM188X;

//===----------------------------------------------------------------------===//
// QuantizeVisitor
//===----------------------------------------------------------------------===//
BM188X::QuantizeVisitor::QuantizeVisitor(
    const tg::bm1880::LayerCalibrationParameter & pLCP)
    : m_LayerCtable(pLCP)
{
}

void BM188X::QuantizeVisitor::visit(BM188X::AveragePool& pOperator)
{
  pOperator.setRShiftWidth(m_LayerCtable.right_shift_width());
  pOperator.setThresholdXQuantized(
      *m_LayerCtable.threshold_x_quantized().data());
}

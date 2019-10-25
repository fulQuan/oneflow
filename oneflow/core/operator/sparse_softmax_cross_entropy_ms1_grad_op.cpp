#include "oneflow/core/operator/sparse_softmax_cross_entropy_ms1_grad_op.h"
#include "oneflow/core/job/sbp_signature_builder.h"
#include "oneflow/core/common/balanced_splitter.h"

namespace oneflow {

void SparseSoftmaxCrossEntropyMs1GradOp::InitFromOpConf() {
  CHECK(op_conf().has_sparse_softmax_cross_entropy_ms1_grad_conf());
  EnrollInputBn("dy");
  EnrollInputBn("prob");
  EnrollInputBn("label", false);
  EnrollOutputBn("dx");
}

const PbMessage& SparseSoftmaxCrossEntropyMs1GradOp::GetCustomizedConf() const {
  return op_conf().sparse_softmax_cross_entropy_ms1_grad_conf();
}

Maybe<void> SparseSoftmaxCrossEntropyMs1GradOp::InferBlobDescs(
    std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx, const SbpSignature* sbp_signature,
    std::function<void(OpContext*)> EnrollOpCtx) const {
  const BlobDesc* dy_blob_desc = GetBlobDesc4BnInOp("dy");
  const BlobDesc* label_blob_desc = GetBlobDesc4BnInOp("label");
  CHECK_OR_RETURN(IsIntegralDataType(label_blob_desc->data_type()));
  CHECK_OR_RETURN(IsFloatingDataType(dy_blob_desc->data_type()));
  CHECK_EQ_OR_RETURN(dy_blob_desc->has_data_id_field(), label_blob_desc->has_data_id_field());
  CHECK_EQ_OR_RETURN(dy_blob_desc->has_dim0_valid_num_field(),
                     label_blob_desc->has_dim0_valid_num_field());
  CHECK_EQ_OR_RETURN(dy_blob_desc->has_dim0_inner_shape(), label_blob_desc->has_dim0_inner_shape());
  if (dy_blob_desc->has_dim0_inner_shape()) {
    CHECK_EQ_OR_RETURN(dy_blob_desc->dim0_inner_shape().At(0), 1);
    CHECK_EQ_OR_RETURN(dy_blob_desc->dim0_inner_shape(), label_blob_desc->dim0_inner_shape());
  }
  // prob
  const BlobDesc* prob_blob_desc = GetBlobDesc4BnInOp("prob");
  CHECK_GE_OR_RETURN(prob_blob_desc->shape().NumAxes(), 2);
  const int64_t num_out_axes = prob_blob_desc->shape().NumAxes() - 1;
  CHECK_GE_OR_RETURN(label_blob_desc->shape().NumAxes(), num_out_axes);
  CHECK_EQ_OR_RETURN(label_blob_desc->shape().Count(num_out_axes), 1);
  FOR_RANGE(int64_t, i, 0, num_out_axes) {
    CHECK_EQ_OR_RETURN(prob_blob_desc->shape().At(i), label_blob_desc->shape().At(i));
  }
  // out
  BlobDesc* dx_blob_desc = GetBlobDesc4BnInOp("dx");
  *dx_blob_desc = *prob_blob_desc;

  return Maybe<void>::Ok();
}

Maybe<void> SparseSoftmaxCrossEntropyMs1GradOp::InferBatchAxis(
    std::function<OptInt64*(const std::string&)> BatchAxis4BnInOp) const {
  *BatchAxis4BnInOp("dx") = *BatchAxis4BnInOp("dy");
  return Maybe<void>::Ok();
}

Maybe<void> SparseSoftmaxCrossEntropyMs1GradOp::GetSbpSignatures(
    SbpSignatureList* sbp_sig_list) const {
  SbpSignatureBuilder().Broadcast("dy").Split("prob", 1).Broadcast("label").Split("dx", 1).Build(
      sbp_sig_list->mutable_sbp_signature()->Add());
  return Maybe<void>::Ok();
}

void SparseSoftmaxCrossEntropyMs1GradOp::VirtualGenKernelConf(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx, KernelConf* kernel_conf) const {
  const int64_t dim = op_conf().sparse_softmax_cross_entropy_ms1_grad_conf().depth();
  CHECK_GE(dim, parallel_ctx->parallel_num());
  BalancedSplitter bs(dim, parallel_ctx->parallel_num());
  kernel_conf->mutable_sparse_softmax_cross_entropy_grad_conf()->set_lower_bound(
      bs.At(parallel_ctx->parallel_id()).begin());
}

REGISTER_OP(OperatorConf::kSparseSoftmaxCrossEntropyMs1GradConf,
            SparseSoftmaxCrossEntropyMs1GradOp);

}  // namespace oneflow
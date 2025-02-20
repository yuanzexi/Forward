/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <NvInferPlugin.h>

#include <memory>
#include <string>
#include <vector>

#include "trt_engine/trt_network_crt/plugins/common/bert_common.h"

namespace fwd {
namespace bert {

// Clip plugin specific constants
static const char* FWD_SKIP_LAYER_NORM_VERSION{"1"};
static const char* FWD_SKIP_LAYER_NORM_NAME{"ForwardSkipLayerNormPluginDynamic"};
static const char* FWD_SKIP_LAYER_NORM_VAR_SEQLEN_VERSION{"2"};

template <bool hasBias>
int computeSkipLayerNormDQQ(cudaStream_t stream, const int ld, const int n, const int8_t* input,
                            const int8_t* skip, const __half* beta, const __half* gamma,
                            int8_t* output, const __half* bias, const float dqScaleIn,
                            const float dqScaleSkip, const float qScale);

template <typename T, bool hasBias>
int computeSkipLayerNorm(cudaStream_t stream, const int ld, const int n, const T* input,
                         const T* skip, const T* beta, const T* gamma, T* output, const T* bias);

class SkipLayerNormPluginDynamic : public nvinfer1::IPluginV2DynamicExt {
 public:
  SkipLayerNormPluginDynamic(const std::string name, const nvinfer1::DataType type, const int ld,
                             const nvinfer1::Weights& beta, const nvinfer1::Weights& gamma,
                             const nvinfer1::Weights& bias);

  SkipLayerNormPluginDynamic(const std::string name, const void* data, size_t length);

  // It doesn't make sense to make SkipLayerNormPluginDynamic without arguments,
  // so we delete default constructor.
  SkipLayerNormPluginDynamic() = delete;

  // IPluginV2DynamicExt Methods
  nvinfer1::IPluginV2DynamicExt* clone() const override;
  nvinfer1::DimsExprs getOutputDimensions(int outputIndex, const nvinfer1::DimsExprs* inputs,
                                          int nbInputs,
                                          nvinfer1::IExprBuilder& exprBuilder) override;
  bool supportsFormatCombination(int pos, const nvinfer1::PluginTensorDesc* inOut, int nbInputs,
                                 int nbOutputs) override;
  void configurePlugin(const nvinfer1::DynamicPluginTensorDesc* in, int nbInputs,
                       const nvinfer1::DynamicPluginTensorDesc* out, int nbOutputs) override;
  size_t getWorkspaceSize(const nvinfer1::PluginTensorDesc* inputs, int nbInputs,
                          const nvinfer1::PluginTensorDesc* outputs, int nbOutputs) const override;
  int enqueue(const nvinfer1::PluginTensorDesc* inputDesc,
              const nvinfer1::PluginTensorDesc* outputDesc, const void* const* inputs,
              void* const* outputs, void* workspace, cudaStream_t stream) override;

  // IPluginV2Ext Methods
  nvinfer1::DataType getOutputDataType(int index, const nvinfer1::DataType* inputTypes,
                                       int nbInputs) const override;

  // IPluginV2 Methods
  const char* getPluginType() const override;
  const char* getPluginVersion() const override;
  int getNbOutputs() const override;
  int initialize() override;
  void terminate() override;
  size_t getSerializationSize() const override;
  void serialize(void* buffer) const override;
  void destroy() override;
  void setPluginNamespace(const char* pluginNamespace) override;
  const char* getPluginNamespace() const override;

 private:
  const std::string mLayerName;
  std::string mNamespace;

  cuda_unique_ptr<void> mGammaDev;
  fwd::bert::cuda_unique_ptr<void> mBetaDev;
  size_t mLd;  // leading dim
  fwd::bert::WeightsWithOwnership mGamma;
  fwd::bert::WeightsWithOwnership mBeta;
  nvinfer1::DataType mType;
  nvinfer1::DataType mCfgType;

  bool mHasBias;
  fwd::bert::cuda_unique_ptr<void> mBiasDev;
  fwd::bert::WeightsWithOwnership mBias;

  size_t mParamWordsize;

 protected:
  // To prevent compiler warnings.
  using nvinfer1::IPluginV2DynamicExt::canBroadcastInputAcrossBatch;
  using nvinfer1::IPluginV2DynamicExt::configurePlugin;
  using nvinfer1::IPluginV2DynamicExt::enqueue;
  using nvinfer1::IPluginV2DynamicExt::getOutputDimensions;
  using nvinfer1::IPluginV2DynamicExt::getWorkspaceSize;
  using nvinfer1::IPluginV2DynamicExt::isOutputBroadcastAcrossBatch;
  using nvinfer1::IPluginV2DynamicExt::supportsFormat;
};

class SkipLayerNormPluginDynamicCreator : public nvinfer1::IPluginCreator {
 public:
  SkipLayerNormPluginDynamicCreator();

  const char* getPluginName() const override;

  const char* getPluginVersion() const override;

  const nvinfer1::PluginFieldCollection* getFieldNames() override;

  nvinfer1::IPluginV2* createPlugin(const char* name,
                                    const nvinfer1::PluginFieldCollection* fc) override;

  nvinfer1::IPluginV2* deserializePlugin(const char* name, const void* serialData,
                                         size_t serialLength) override;

  void setPluginNamespace(const char* pluginNamespace) override;

  const char* getPluginNamespace() const override;

 private:
  nvinfer1::PluginFieldCollection mFC;
  std::vector<nvinfer1::PluginField> mPluginAttributes;
  std::string mNamespace;
};

class SkipLayerNormVarSeqlenPlugin : public nvinfer1::IPluginV2DynamicExt {
 public:
  SkipLayerNormVarSeqlenPlugin(const std::string name, const nvinfer1::DataType type,
                               const nvinfer1::Weights& beta, const nvinfer1::Weights& gamma,
                               const nvinfer1::Weights& bias);

  SkipLayerNormVarSeqlenPlugin(const std::string name, const void* data, size_t length);

  // It doesn't make sense to make SkipLayerNormVarSeqlenPlugin without
  // arguments, so we delete default constructor.
  SkipLayerNormVarSeqlenPlugin() = delete;

  // IPluginV2DynamicExt Methods
  nvinfer1::IPluginV2DynamicExt* clone() const override;
  nvinfer1::DimsExprs getOutputDimensions(int outputIndex, const nvinfer1::DimsExprs* inputs,
                                          int nbInputs,
                                          nvinfer1::IExprBuilder& exprBuilder) override;
  bool supportsFormatCombination(int pos, const nvinfer1::PluginTensorDesc* inOut, int nbInputs,
                                 int nbOutputs) override;
  void configurePlugin(const nvinfer1::DynamicPluginTensorDesc* in, int nbInputs,
                       const nvinfer1::DynamicPluginTensorDesc* out, int nbOutputs) override;
  size_t getWorkspaceSize(const nvinfer1::PluginTensorDesc* inputs, int nbInputs,
                          const nvinfer1::PluginTensorDesc* outputs, int nbOutputs) const override;
  int enqueue(const nvinfer1::PluginTensorDesc* inputDesc,
              const nvinfer1::PluginTensorDesc* outputDesc, const void* const* inputs,
              void* const* outputs, void* workspace, cudaStream_t stream) override;

  // IPluginV2Ext Methods
  nvinfer1::DataType getOutputDataType(int index, const nvinfer1::DataType* inputTypes,
                                       int nbInputs) const override;

  // IPluginV2 Methods
  const char* getPluginType() const override;
  const char* getPluginVersion() const override;
  int getNbOutputs() const override;
  int initialize() override;
  void terminate() override;
  size_t getSerializationSize() const override;
  void serialize(void* buffer) const override;
  void destroy() override;
  void setPluginNamespace(const char* pluginNamespace) override;
  const char* getPluginNamespace() const override;

 protected:
  void copyParamToDevice();

 private:
  const std::string mLayerName;
  std::string mNamespace;

  fwd::bert::cuda_unique_ptr<void> mGammaDev;
  fwd::bert::cuda_unique_ptr<void> mBetaDev;
  size_t mLd;  // leading dim
  fwd::bert::WeightsWithOwnership mGamma;
  fwd::bert::WeightsWithOwnership mBeta;
  nvinfer1::DataType mType;
  nvinfer1::DataType mCfgType;

  bool mHasBias;
  fwd::bert::cuda_unique_ptr<void> mBiasDev;
  fwd::bert::WeightsWithOwnership mBias;

  size_t mParamWordsize;
  bool mParamsOnDevice;

 protected:
  // To prevent compiler warnings.
  using nvinfer1::IPluginV2DynamicExt::canBroadcastInputAcrossBatch;
  using nvinfer1::IPluginV2DynamicExt::configurePlugin;
  using nvinfer1::IPluginV2DynamicExt::enqueue;
  using nvinfer1::IPluginV2DynamicExt::getOutputDimensions;
  using nvinfer1::IPluginV2DynamicExt::getWorkspaceSize;
  using nvinfer1::IPluginV2DynamicExt::isOutputBroadcastAcrossBatch;
  using nvinfer1::IPluginV2DynamicExt::supportsFormat;
};

class SkipLayerNormVarSeqlenPluginCreator : public nvinfer1::IPluginCreator {
 public:
  SkipLayerNormVarSeqlenPluginCreator();

  const char* getPluginName() const override;

  const char* getPluginVersion() const override;

  const nvinfer1::PluginFieldCollection* getFieldNames() override;

  nvinfer1::IPluginV2* createPlugin(const char* name,
                                    const nvinfer1::PluginFieldCollection* fc) override;

  nvinfer1::IPluginV2* deserializePlugin(const char* name, const void* serialData,
                                         size_t serialLength) override;

  void setPluginNamespace(const char* pluginNamespace) override;

  const char* getPluginNamespace() const override;

 private:
  nvinfer1::PluginFieldCollection mFC;
  std::vector<nvinfer1::PluginField> mPluginAttributes;
  std::string mNamespace;
};

}  // namespace bert
}  // namespace fwd

// Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef _QAIC_APPLICATION_H_
#define _QAIC_APPLICATION_H_

#include "ProgramConfig.h"
#include "AICMetadataWriter.h"
#include "networkdesc/qpc/inc/QAicQpc.h"

namespace qaic {

class Program {
public:
  virtual ~Program() = default;

  virtual std::unique_ptr<AICMetadataWriter> generateMetadata() const = 0;
  virtual std::unique_ptr<aicnwdesc::networkDescriptor>
  generateNetworkDescriptor() const = 0;
  virtual bool validateQPC(QAicQpcHandle *handle) const = 0;

  virtual uintptr_t getEntrypointAddr() const = 0;
  virtual void setEntrypointAddr(uintptr_t addr) = 0;
};

class ComputeProgram : public Program {
public:
  explicit ComputeProgram(ProgramConfig &&config);

  const ProgramConfig &getConfig() const { return config_; }
  std::unique_ptr<AICMetadataWriter> generateMetadata() const override;
  std::unique_ptr<aicnwdesc::networkDescriptor>
  generateNetworkDescriptor() const override;
  bool validateQPC(QAicQpcHandle *handle) const override;

  uintptr_t getEntrypointAddr() const override { return entryPoint_; }
  void setEntrypointAddr(uintptr_t addr) override { entryPoint_ = addr; }

private:
  ProgramConfig config_;
  uintptr_t entryPoint_;
};
} // namespace qaic

#endif

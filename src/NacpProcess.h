#pragma once
#include "types.h"

#include <pietendo/hac/ApplicationControlProperty.h>

namespace nstool
{

class NacpProcess
{
  public:
    NacpProcess();

    void process();

    void setInputFile(const std::shared_ptr<tc::io::IStream> &file);
    void setVerifyMode(bool verify);

    const pie::hac::ApplicationControlProperty &getApplicationControlProperty() const;

  private:
    std::string mModuleName;

    std::shared_ptr<tc::io::IStream> mFile;

    bool mVerify;

    pie::hac::ApplicationControlProperty mNacp;

    void importNacp();
    void displayNacp();
};

} // namespace nstool

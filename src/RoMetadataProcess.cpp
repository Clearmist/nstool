#include "RoMetadataProcess.h"
#include "Report.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

nstool::RoMetadataProcess::RoMetadataProcess()
    : mModuleName("nstool::RoMetadataProcess"), mIs64BitInstruction(true), mListApi(false), mListSymbols(false),
      mApiInfo(), mDynSym(), mDynStr(), mRoBlob(), mSdkVerApiList(), mPublicApiList(), mDebugApiList(),
      mPrivateApiList(), mSymbolList()
{
}

void nstool::RoMetadataProcess::process()
{
    importApiList();

    displayRoMetaData();
}

void nstool::RoMetadataProcess::setRoBinary(const tc::ByteData &bin)
{
    mRoBlob = bin;
}

void nstool::RoMetadataProcess::setApiInfo(size_t offset, size_t size)
{
    mApiInfo.offset = offset;
    mApiInfo.size = size;
}

void nstool::RoMetadataProcess::setDynSym(size_t offset, size_t size)
{
    mDynSym.offset = offset;
    mDynSym.size = size;
}

void nstool::RoMetadataProcess::setDynStr(size_t offset, size_t size)
{
    mDynStr.offset = offset;
    mDynStr.size = size;
}

void nstool::RoMetadataProcess::setIs64BitInstruction(bool flag)
{
    mIs64BitInstruction = flag;
}

void nstool::RoMetadataProcess::setListApi(bool listApi)
{
    mListApi = listApi;
}

void nstool::RoMetadataProcess::setListSymbols(bool listSymbols)
{
    mListSymbols = listSymbols;
}

const std::vector<nstool::SdkApiString> &nstool::RoMetadataProcess::getSdkVerApiList() const
{
    return mSdkVerApiList;
}

const std::vector<nstool::SdkApiString> &nstool::RoMetadataProcess::getPublicApiList() const
{
    return mPublicApiList;
}

const std::vector<nstool::SdkApiString> &nstool::RoMetadataProcess::getDebugApiList() const
{
    return mDebugApiList;
}

const std::vector<nstool::SdkApiString> &nstool::RoMetadataProcess::getPrivateApiList() const
{
    return mPrivateApiList;
}

const std::vector<nstool::SdkApiString> &nstool::RoMetadataProcess::getGuidelineApiList() const
{
    return mGuidelineApiList;
}

const std::vector<nstool::ElfSymbolParser::sElfSymbol> &nstool::RoMetadataProcess::getSymbolList() const
{
    return mSymbolList.getSymbolList();
}

void nstool::RoMetadataProcess::importApiList()
{
    if (mRoBlob.size() == 0)
    {
        throw tc::Exception(mModuleName, "No ro binary set.");
    }

    if (mApiInfo.size > 0)
    {
        std::stringstream list_stream(std::string((char *)mRoBlob.data() + mApiInfo.offset, mApiInfo.size));
        std::string api_str;

        while (std::getline(list_stream, api_str, (char)0x00))
        {
            SdkApiString api(api_str);

            switch (api.getApiType())
            {
            case SdkApiString::API_SDK_VERSION:
                mSdkVerApiList.push_back(api);
                break;
            case SdkApiString::API_MIDDLEWARE:
                mPublicApiList.push_back(api);
                break;
            case SdkApiString::API_DEBUG:
                mDebugApiList.push_back(api);
                break;
            case SdkApiString::API_PRIVATE:
                mPrivateApiList.push_back(api);
                break;
            case SdkApiString::API_GUIDELINE:
                mGuidelineApiList.push_back(api);
                break;
            default:
                break;
            }
        }
    }

    if (mDynSym.size > 0)
    {
        mSymbolList.parseData(
            mRoBlob.data() + mDynSym.offset, mDynSym.size, mRoBlob.data() + mDynStr.offset, mDynStr.size,
            mIs64BitInstruction);
    }
}

void nstool::RoMetadataProcess::displayRoMetaData()
{
    Report &r = get_report();

    size_t api_num = mSdkVerApiList.size() + mPublicApiList.size() + mDebugApiList.size() + mPrivateApiList.size();

    if (api_num > 0)
    {
        r.text("[SDK API List]", Report::TextType::Extended);

        if (mSdkVerApiList.size() > 0)
        {
            r.text(fmt::format("  Sdk Revision: {:s}", mSdkVerApiList[0].getModuleName()), Report::TextType::Extended);

            r.set("data.sdkRevision", mSdkVerApiList[0].getModuleName());
        }

        if (mPublicApiList.size() > 0)
        {
            r.text("  Public APIs:", Report::TextType::Extended);

            for (size_t i = 0; i < mPublicApiList.size(); i++)
            {
                r.text(
                    fmt::format(
                        "    {:s} (vendor: {:s})", mPublicApiList[i].getModuleName(),
                        mPublicApiList[i].getVenderName()),
                    Report::TextType::Extended);

                r.push(
                    "data.publicAPI",
                    nlohmann::json{
                        {"module", mPublicApiList[i].getModuleName()}, {"vendor", mPublicApiList[i].getVenderName()}});
            }
        }

        if (mDebugApiList.size() > 0)
        {
            r.text("  Debug APIs:", Report::TextType::Extended);

            for (size_t i = 0; i < mDebugApiList.size(); i++)
            {
                r.text(
                    fmt::format(
                        "    {:s} (vendor: {:s})", mDebugApiList[i].getModuleName(), mDebugApiList[i].getVenderName()),
                    Report::TextType::Extended);

                r.push(
                    "data.debugAPI",
                    nlohmann::json{
                        {"module", mDebugApiList[i].getModuleName()}, {"vendor", mDebugApiList[i].getVenderName()}});
            }
        }

        if (mPrivateApiList.size() > 0)
        {
            r.text("  Private APIs:", Report::TextType::Extended);

            for (size_t i = 0; i < mPrivateApiList.size(); i++)
            {
                r.text(
                    fmt::format(
                        "    {:s} (vendor: {:s})", mPrivateApiList[i].getModuleName(),
                        mPrivateApiList[i].getVenderName()),
                    Report::TextType::Extended);

                r.push(
                    "data.privateAPI", nlohmann::json{
                                           {"module", mPrivateApiList[i].getModuleName()},
                                           {"vendor", mPrivateApiList[i].getVenderName()}});
            }
        }

        if (mGuidelineApiList.size() > 0)
        {
            r.text("  Guideline APIs:", Report::TextType::Extended);

            for (size_t i = 0; i < mGuidelineApiList.size(); i++)
            {
                r.text(
                    fmt::format(
                        "    {:s} (vendor: {:s})", mGuidelineApiList[i].getModuleName(),
                        mGuidelineApiList[i].getVenderName()),
                    Report::TextType::Extended);

                r.push(
                    "data.guidelineAPI", nlohmann::json{
                                             {"module", mGuidelineApiList[i].getModuleName()},
                                             {"vendor", mGuidelineApiList[i].getVenderName()}});
            }
        }
    }

    if (mSymbolList.getSymbolList().size() > 0)
    {
        r.text("[Symbol List]", Report::TextType::Extended);

        for (size_t i = 0; i < mSymbolList.getSymbolList().size(); i++)
        {
            const ElfSymbolParser::sElfSymbol &symbol = mSymbolList.getSymbolList()[i];

            r.text(
                fmt::format(
                    "  {:s}  [SHN={:s} ({:04x})][STT={:s}][STB={:s}]", symbol.name,
                    getSectionIndexStr(symbol.shn_index), symbol.shn_index, getSymbolTypeStr(symbol.symbol_type),
                    getSymbolBindingStr(symbol.symbol_binding)),
                Report::TextType::Extended);

            r.push(
                "data.symbols", nlohmann::json{
                                    {"name", symbol.name},
                                    {"shn", getSectionIndexStr(symbol.shn_index)},
                                    {"shnIndex", symbol.shn_index},
                                    {"stt", getSymbolTypeStr(symbol.symbol_type)},
                                    {"stb", getSymbolBindingStr(symbol.symbol_binding)}});
        }
    }
}

std::string nstool::RoMetadataProcess::getSectionIndexStr(uint16_t shn_index) const
{
    std::string str;

    switch (shn_index)
    {
    case (elf::SHN_UNDEF):
        str = "UNDEF";
        break;
    case (elf::SHN_LOPROC):
        str = "LOPROC";
        break;
    case (elf::SHN_HIPROC):
        str = "HIPROC";
        break;
    case (elf::SHN_LOOS):
        str = "LOOS";
        break;
    case (elf::SHN_HIOS):
        str = "HIOS";
        break;
    case (elf::SHN_ABS):
        str = "ABS";
        break;
    case (elf::SHN_COMMON):
        str = "COMMON";
        break;
    default:
        str = "UNKNOWN";
        break;
    }

    return str;
}

std::string nstool::RoMetadataProcess::getSymbolTypeStr(byte_t symbol_type) const
{
    std::string str;

    switch (symbol_type)
    {
    case (elf::STT_NOTYPE):
        str = "NOTYPE";
        break;
    case (elf::STT_OBJECT):
        str = "OBJECT";
        break;
    case (elf::STT_FUNC):
        str = "FUNC";
        break;
    case (elf::STT_SECTION):
        str = "SECTION";
        break;
    case (elf::STT_FILE):
        str = "FILE";
        break;
    case (elf::STT_LOOS):
        str = "LOOS";
        break;
    case (elf::STT_HIOS):
        str = "HIOS";
        break;
    case (elf::STT_LOPROC):
        str = "LOPROC";
        break;
    case (elf::STT_HIPROC):
        str = "HIPROC";
        break;
    default:
        str = "UNKNOWN";
        break;
    }

    return str;
}

std::string nstool::RoMetadataProcess::getSymbolBindingStr(byte_t symbol_binding) const
{
    std::string str;

    switch (symbol_binding)
    {
    case (elf::STB_LOCAL):
        str = "LOCAL";
        break;
    case (elf::STB_GLOBAL):
        str = "GLOBAL";
        break;
    case (elf::STB_WEAK):
        str = "WEAK";
        break;
    case (elf::STB_LOOS):
        str = "LOOS";
        break;
    case (elf::STB_HIOS):
        str = "HIOS";
        break;
    case (elf::STB_LOPROC):
        str = "LOPROC";
        break;
    case (elf::STB_HIPROC):
        str = "HIPROC";
        break;
    default:
        str = "UNKNOWN";
        break;
    }

    return str;
}

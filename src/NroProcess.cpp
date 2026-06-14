#include "NroProcess.h"
#include "Report.hpp"

nstool::NroProcess::NroProcess() : mModuleName("nstool::NroProcess"), mFile(), mVerify(false) {}

void nstool::NroProcess::process()
{
    importHeader();
    importCodeSegments();

    displayHeader();

    processRoMeta();

    if (mIsHomebrewNro)
    {
        mAssetProc.process();
    }
}

void nstool::NroProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::NroProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

void nstool::NroProcess::setIs64BitInstruction(bool flag)
{
    mRoMeta.setIs64BitInstruction(flag);
}

void nstool::NroProcess::setListApi(bool listApi)
{
    mRoMeta.setListApi(listApi);
}

void nstool::NroProcess::setListSymbols(bool listSymbols)
{
    mRoMeta.setListSymbols(listSymbols);
}

void nstool::NroProcess::setAssetIconExtractPath(const tc::io::Path &path)
{
    mAssetProc.setIconExtractPath(path);
}

void nstool::NroProcess::setAssetNacpExtractPath(const tc::io::Path &path)
{
    mAssetProc.setNacpExtractPath(path);
}

void nstool::NroProcess::setAssetRomfsShowFsTree(bool show_fs_tree)
{
    mAssetProc.setRomfsShowFsTree(show_fs_tree);
}

void nstool::NroProcess::setAssetRomfsExtractJobs(const std::vector<nstool::ExtractJob> &extract_jobs)
{
    mAssetProc.setRomfsExtractJobs(extract_jobs);
}

const nstool::RoMetadataProcess &nstool::NroProcess::getRoMetadataProcess() const
{
    return mRoMeta;
}

void nstool::NroProcess::importHeader()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size is smaller than NRO header size
    if (tc::io::IOUtil::castInt64ToSize(mFile->length()) < sizeof(pie::hac::sNroHeader))
    {
        throw tc::Exception(mModuleName, "Corrupt NRO: file too small.");
    }

    // read nro
    tc::ByteData scratch = tc::ByteData(sizeof(pie::hac::sNroHeader));
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // parse nro header
    mHdr.fromBytes(scratch.data(), scratch.size());

    // setup homebrew extension
    pie::hac::sNroHeader *raw_hdr = (pie::hac::sNroHeader *)scratch.data();

    int64_t file_size = mFile->length();

    if (((tc::bn::le64<uint64_t> *)raw_hdr->reserved_0.data())->unwrap() == pie::hac::nro::kNroHomebrewStructMagic &&
        file_size > int64_t(mHdr.getNroSize()))
    {
        mIsHomebrewNro = true;
        mAssetProc.setInputFile(std::make_shared<tc::io::SubStream>(
            tc::io::SubStream(mFile, int64_t(mHdr.getNroSize()), file_size - int64_t(mHdr.getNroSize()))));
        mAssetProc.setVerifyMode(mVerify);
    }
    else
    {
        mIsHomebrewNro = false;
    }
}

void nstool::NroProcess::importCodeSegments()
{
    if (mHdr.getTextInfo().size > 0)
    {
        mTextBlob = tc::ByteData(mHdr.getTextInfo().size);
        mFile->seek(mHdr.getTextInfo().memory_offset, tc::io::SeekOrigin::Begin);
        mFile->read(mTextBlob.data(), mTextBlob.size());
    }

    if (mHdr.getRoInfo().size > 0)
    {
        mRoBlob = tc::ByteData(mHdr.getRoInfo().size);
        mFile->seek(mHdr.getRoInfo().memory_offset, tc::io::SeekOrigin::Begin);
        mFile->read(mRoBlob.data(), mRoBlob.size());
    }

    if (mHdr.getDataInfo().size > 0)
    {
        mDataBlob = tc::ByteData(mHdr.getDataInfo().size);
        mFile->seek(mHdr.getDataInfo().memory_offset, tc::io::SeekOrigin::Begin);
        mFile->read(mDataBlob.data(), mDataBlob.size());
    }
}

void nstool::NroProcess::displayHeader()
{
    Report &r = get_report();

    r.text("[NRO Header]");
    r.text("  RoCrt:");
    r.text(fmt::format("    EntryPoint: 0x{:x}", mHdr.getRoCrtEntryPoint()));
    r.text(fmt::format("    ModOffset:  0x{:x}", mHdr.getRoCrtModOffset()));
    r.text(fmt::format(
        "  ModuleId:    {:s}",
        tc::cli::FormatUtil::formatBytesAsString(mHdr.getModuleId().data(), mHdr.getModuleId().size(), false, "")));
    r.text(fmt::format("  NroSize:     0x{:x}", mHdr.getNroSize()));
    r.text("  Program Sections:");
    r.text("     .text:");
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getTextInfo().memory_offset));
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getTextInfo().size));
    r.text("    .ro:");
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getRoInfo().memory_offset));
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getRoInfo().size));
    r.text("    .api_info:", Report::TextType::Extended);
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getRoEmbeddedInfo().memory_offset), Report::TextType::Extended);
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getRoEmbeddedInfo().size), Report::TextType::Extended);
    r.text("    .dynstr:", Report::TextType::Extended);
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getRoDynStrInfo().memory_offset), Report::TextType::Extended);
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getRoDynStrInfo().size), Report::TextType::Extended);
    r.text("    .dynsym:", Report::TextType::Extended);
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getRoDynSymInfo().memory_offset), Report::TextType::Extended);
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getRoDynSymInfo().size), Report::TextType::Extended);
    r.text("    .data:");
    r.text(fmt::format("      Offset:     0x{:x}", mHdr.getDataInfo().memory_offset));
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getDataInfo().size));
    r.text("    .bss:");
    r.text(fmt::format("      Size:       0x{:x}", mHdr.getBssSize()));

    r.set(
        "data.nroHeader.roCrt", nlohmann::json{
                                    {"entryPoint", fmt::format("0x{:x}", mHdr.getRoCrtEntryPoint())},
                                    {"modOffset", fmt::format("0x{:x}", mHdr.getRoCrtModOffset())},
                                    {"moduleId", tc::cli::FormatUtil::formatBytesAsString(
                                                     mHdr.getModuleId().data(), mHdr.getModuleId().size(), false, "")},
                                    {"nroSize", fmt::format("0x{:x}", mHdr.getNroSize())}});
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "text"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getTextInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getTextInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "ro"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getRoInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getRoInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "api_info"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getRoEmbeddedInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getRoEmbeddedInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "dynstr"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getRoDynStrInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getRoDynStrInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "dynsym"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getRoDynSymInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getRoDynSymInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "data"},
                                              {"offset", fmt::format("0x{:x}", mHdr.getDataInfo().memory_offset)},
                                              {"size", fmt::format("0x{:x}", mHdr.getDataInfo().size)},
                                          });
    r.push(
        "data.nroHeader.programSections", nlohmann::json{
                                              {"name", "bss"},
                                              {"size", fmt::format("0x{:x}", mHdr.getBssSize())},
                                          });
}

void nstool::NroProcess::processRoMeta()
{
    if (mRoBlob.size())
    {
        // setup ro metadata
        mRoMeta.setApiInfo(mHdr.getRoEmbeddedInfo().memory_offset, mHdr.getRoEmbeddedInfo().size);
        mRoMeta.setDynSym(mHdr.getRoDynSymInfo().memory_offset, mHdr.getRoDynSymInfo().size);
        mRoMeta.setDynStr(mHdr.getRoDynStrInfo().memory_offset, mHdr.getRoDynStrInfo().size);
        mRoMeta.setRoBinary(mRoBlob);
        mRoMeta.process();
    }
}

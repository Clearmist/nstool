#include "NsoProcess.h"
#include "Report.hpp"
#include <lz4.h>

nstool::NsoProcess::NsoProcess()
    : mModuleName("nstool::NsoProcess"), mFile(), mVerify(false), mIs64BitInstruction(true), mListApi(false),
      mListSymbols(false)
{
}

void nstool::NsoProcess::process()
{
    importHeader();
    importCodeSegments();
    displayNsoHeader();
    processRoMeta();
}

void nstool::NsoProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::NsoProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

void nstool::NsoProcess::setIs64BitInstruction(bool flag)
{
    mRoMeta.setIs64BitInstruction(flag);
}

void nstool::NsoProcess::setListApi(bool listApi)
{
    mRoMeta.setListApi(listApi);
}

void nstool::NsoProcess::setListSymbols(bool listSymbols)
{
    mRoMeta.setListSymbols(listSymbols);
}

const nstool::RoMetadataProcess &nstool::NsoProcess::getRoMetadataProcess() const
{
    return mRoMeta;
}

void nstool::NsoProcess::importHeader()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size is smaller than NSO header size
    size_t file_size = tc::io::IOUtil::castInt64ToSize(mFile->length());

    if (file_size < sizeof(pie::hac::sNsoHeader))
    {
        throw tc::Exception(mModuleName, "Corrupt NSO: file too small.");
    }

    // read nso
    tc::ByteData scratch = tc::ByteData(sizeof(pie::hac::sNsoHeader));
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // parse nso header
    mHdr.fromBytes(scratch.data(), scratch.size());
}

void nstool::NsoProcess::importCodeSegments()
{
    tc::ByteData scratch;
    pie::hac::detail::sha256_hash_t calc_hash;

    // process text segment
    if (mHdr.getTextSegmentInfo().is_compressed)
    {
        // allocate/read compressed text
        scratch = tc::ByteData(mHdr.getTextSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getTextSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(scratch.data(), scratch.size());

        // allocate for decompressed text segment
        mTextBlob = tc::ByteData(mHdr.getTextSegmentInfo().memory_layout.size);

        // decompress text segment
        if (decompressData(scratch.data(), scratch.size(), mTextBlob.data(), mTextBlob.size()) != mTextBlob.size())
        {
            throw tc::Exception(mModuleName, "NSO text segment failed to decompress");
        }
    }
    else
    {
        // read text segment directly (not compressed)
        mTextBlob = tc::ByteData(mHdr.getTextSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getTextSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mTextBlob.data(), mTextBlob.size());
    }

    if (mHdr.getTextSegmentInfo().is_hashed)
    {
        tc::crypto::GenerateSha2256Hash(calc_hash.data(), mTextBlob.data(), mTextBlob.size());

        if (calc_hash != mHdr.getTextSegmentInfo().hash)
        {
            throw tc::Exception(mModuleName, "NSO text segment failed SHA256 verification");
        }
    }

    // process ro segment
    if (mHdr.getRoSegmentInfo().is_compressed)
    {
        // allocate/read compressed ro segment
        scratch = tc::ByteData(mHdr.getRoSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getRoSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(scratch.data(), scratch.size());

        // allocate for decompressed ro segment
        mRoBlob = tc::ByteData(mHdr.getRoSegmentInfo().memory_layout.size);

        // decompress ro segment
        if (decompressData(scratch.data(), scratch.size(), mRoBlob.data(), mRoBlob.size()) != mRoBlob.size())
        {
            throw tc::Exception(mModuleName, "NSO ro segment failed to decompress");
        }
    }
    else
    {
        // read ro segment directly (not compressed)
        mRoBlob = tc::ByteData(mHdr.getRoSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getRoSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mRoBlob.data(), mRoBlob.size());
    }

    if (mHdr.getRoSegmentInfo().is_hashed)
    {
        tc::crypto::GenerateSha2256Hash(calc_hash.data(), mRoBlob.data(), mRoBlob.size());

        if (calc_hash != mHdr.getRoSegmentInfo().hash)
        {
            throw tc::Exception(mModuleName, "NSO ro segment failed SHA256 verification");
        }
    }

    // process ro segment
    if (mHdr.getDataSegmentInfo().is_compressed)
    {
        // allocate/read compressed ro segment
        scratch = tc::ByteData(mHdr.getDataSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getDataSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(scratch.data(), scratch.size());

        // allocate for decompressed ro segment
        mDataBlob = tc::ByteData(mHdr.getDataSegmentInfo().memory_layout.size);

        // decompress ro segment
        if (decompressData(scratch.data(), scratch.size(), mDataBlob.data(), mDataBlob.size()) != mDataBlob.size())
        {
            throw tc::Exception(mModuleName, "NSO data segment failed to decompress");
        }
    }
    else
    {
        // read ro segment directly (not compressed)
        mDataBlob = tc::ByteData(mHdr.getDataSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getDataSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mDataBlob.data(), mDataBlob.size());
    }

    if (mHdr.getDataSegmentInfo().is_hashed)
    {
        tc::crypto::GenerateSha2256Hash(calc_hash.data(), mDataBlob.data(), mDataBlob.size());

        if (calc_hash != mHdr.getDataSegmentInfo().hash)
        {
            throw tc::Exception(mModuleName, "NSO data segment failed SHA256 verification");
        }
    }
}

void nstool::NsoProcess::displayNsoHeader()
{
    Report &r = get_report();

    r.text("[NSO Header]");
    r.text(fmt::format(
        "  ModuleId:           {:s}",
        tc::cli::FormatUtil::formatBytesAsString(mHdr.getModuleId().data(), mHdr.getModuleId().size(), false, "")));
    r.text("  Program Segments:", Report::TextType::Layout);
    r.text("     .module_name:", Report::TextType::Layout);
    r.text(fmt::format("      FileOffset:     0x{:x}", mHdr.getModuleNameInfo().offset), Report::TextType::Layout);
    r.text(fmt::format("      FileSize:       0x{:x}", mHdr.getModuleNameInfo().size), Report::TextType::Layout);
    r.text("    .text:", Report::TextType::Layout);
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getTextSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getTextSegmentInfo().file_layout.size,
            (mHdr.getTextSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text("    .ro:", Report::TextType::Layout);
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getRoSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getRoSegmentInfo().file_layout.size,
            (mHdr.getRoSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text("    .data:", Report::TextType::Layout);
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getDataSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getDataSegmentInfo().file_layout.size,
            (mHdr.getDataSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text("  Program Sections:");
    r.text("     .text:");
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getTextSegmentInfo().memory_layout.offset));
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getTextSegmentInfo().memory_layout.size));
    r.text(
        fmt::format(
            "      Hash:           {:s}",
            tc::cli::FormatUtil::formatBytesAsString(
                mHdr.getTextSegmentInfo().hash.data(), mHdr.getTextSegmentInfo().hash.size(), false, "")),
        Report::TextType::Extended);
    r.text("    .ro:");
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getRoSegmentInfo().memory_layout.offset));
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getRoSegmentInfo().memory_layout.size));
    r.text(
        fmt::format(
            "      Hash:           {:s}",
            tc::cli::FormatUtil::formatBytesAsString(
                mHdr.getRoSegmentInfo().hash.data(), mHdr.getRoSegmentInfo().hash.size(), false, "")),
        Report::TextType::Extended);
    r.text("    .api_info:", Report::TextType::Extended);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getRoEmbeddedInfo().offset), Report::TextType::Extended);
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getRoEmbeddedInfo().size), Report::TextType::Extended);
    r.text("    .dynstr:", Report::TextType::Extended);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getRoDynStrInfo().offset), Report::TextType::Extended);
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getRoDynStrInfo().size), Report::TextType::Extended);
    r.text("    .dynsym:", Report::TextType::Extended);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getRoDynSymInfo().offset), Report::TextType::Extended);
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getRoDynSymInfo().size), Report::TextType::Extended);
    r.text("    .data:", Report::TextType::Extended);
    r.text(
        fmt::format("      MemoryOffset:   0x{:x}", mHdr.getDataSegmentInfo().memory_layout.offset),
        Report::TextType::Extended);
    r.text(
        fmt::format("      MemorySize:     0x{:x}", mHdr.getDataSegmentInfo().memory_layout.size),
        Report::TextType::Extended);
    r.text(
        fmt::format(
            "      Hash:           {:s}",
            tc::cli::FormatUtil::formatBytesAsString(
                mHdr.getDataSegmentInfo().hash.data(), mHdr.getDataSegmentInfo().hash.size(), false, "")),
        Report::TextType::Extended);
    r.text("    .bss:");
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getBssSize()));

    r.set(
        "data.nsoHeader.moduleId",
        tc::cli::FormatUtil::formatBytesAsString(mHdr.getModuleId().data(), mHdr.getModuleId().size(), false, ""));
    r.push(
        "data.nsoHeader.programSegments", nlohmann::json{
                                              {"name", "module_name"},
                                              {"fileOffset", fmt::format("0x{:x}", mHdr.getModuleNameInfo().offset)},
                                              {"fileSize", fmt::format("0x{:x}", mHdr.getModuleNameInfo().size)}});
    r.push(
        "data.nsoHeader.programSegments",
        nlohmann::json{
            {"name", "text"},
            {"fileOffset", fmt::format("0x{:x}", mHdr.getTextSegmentInfo().file_layout.offset)},
            {"fileSize", fmt::format("0x{:x}", mHdr.getTextSegmentInfo().file_layout.size)},
            {"isCompressed", mHdr.getTextSegmentInfo().is_compressed}});
    r.push(
        "data.nsoHeader.programSegments",
        nlohmann::json{
            {"name", "ro"},
            {"fileOffset", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().file_layout.offset)},
            {"fileSize", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().file_layout.size)},
            {"isCompressed", mHdr.getRoSegmentInfo().is_compressed}});
    r.push(
        "data.nsoHeader.programSegments",
        nlohmann::json{
            {"name", "data"},
            {"fileOffset", fmt::format("0x{:x}", mHdr.getDataSegmentInfo().file_layout.offset)},
            {"fileSize", fmt::format("0x{:x}", mHdr.getDataSegmentInfo().file_layout.size)},
            {"isCompressed", mHdr.getDataSegmentInfo().is_compressed}});
    r.push(
        "data.nsoHeader.programSections",
        nlohmann::json{
            {"name", "text"},
            {"memoryOffset", fmt::format("0x{:x}", mHdr.getTextSegmentInfo().memory_layout.offset)},
            {"memorySize", fmt::format("0x{:x}", mHdr.getTextSegmentInfo().memory_layout.size)},
            {"hash", tc::cli::FormatUtil::formatBytesAsString(
                         mHdr.getTextSegmentInfo().hash.data(), mHdr.getTextSegmentInfo().hash.size(), false, "")}});
    r.push(
        "data.nsoHeader.programSections",
        nlohmann::json{
            {"name", "ro"},
            {"memoryOffset", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().memory_layout.offset)},
            {"memorySize", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().memory_layout.size)},
            {"hash", tc::cli::FormatUtil::formatBytesAsString(
                         mHdr.getRoSegmentInfo().hash.data(), mHdr.getRoSegmentInfo().hash.size(), false, "")}});
    r.push(
        "data.nsoHeader.programSections", nlohmann::json{
                                              {"name", "api_info"},
                                              {"memoryOffset", fmt::format("0x{:x}", mHdr.getRoEmbeddedInfo().offset)},
                                              {"memorySize", fmt::format("0x{:x}", mHdr.getRoEmbeddedInfo().size)}});
    r.push(
        "data.nsoHeader.programSections", nlohmann::json{
                                              {"name", "dynstr"},
                                              {"memoryOffset", fmt::format("0x{:x}", mHdr.getRoDynStrInfo().offset)},
                                              {"memorySize", fmt::format("0x{:x}", mHdr.getRoDynStrInfo().size)}});
    r.push(
        "data.nsoHeader.programSections", nlohmann::json{
                                              {"name", "dynsym"},
                                              {"memoryOffset", fmt::format("0x{:x}", mHdr.getRoDynSymInfo().offset)},
                                              {"memorySize", fmt::format("0x{:x}", mHdr.getRoDynSymInfo().size)}});
    r.push(
        "data.nsoHeader.programSections",
        nlohmann::json{
            {"name", "data"},
            {"memoryOffset", fmt::format("0x{:x}", mHdr.getDataSegmentInfo().memory_layout.offset)},
            {"memorySize", fmt::format("0x{:x}", mHdr.getDataSegmentInfo().memory_layout.size)},
            {"hash", tc::cli::FormatUtil::formatBytesAsString(
                         mHdr.getDataSegmentInfo().hash.data(), mHdr.getDataSegmentInfo().hash.size(), false, "")}});
    r.push(
        "data.nsoHeader.programSections",
        nlohmann::json{{"name", "bss"}, {"memorySize", fmt::format("0x{:x}", mHdr.getBssSize())}});
}

void nstool::NsoProcess::processRoMeta()
{
    if (mRoBlob.size())
    {
        // setup ro metadata
        mRoMeta.setApiInfo(mHdr.getRoEmbeddedInfo().offset, mHdr.getRoEmbeddedInfo().size);
        mRoMeta.setDynSym(mHdr.getRoDynSymInfo().offset, mHdr.getRoDynSymInfo().size);
        mRoMeta.setDynStr(mHdr.getRoDynStrInfo().offset, mHdr.getRoDynStrInfo().size);
        mRoMeta.setRoBinary(mRoBlob);
        mRoMeta.process();
    }
}

size_t nstool::NsoProcess::decompressData(const byte_t *src, size_t src_len, byte_t *dst, size_t dst_capacity)
{
    if (src_len >= LZ4_MAX_INPUT_SIZE)
    {
        return 0;
    }

    int32_t src_len_input = int32_t(src_len);
    int32_t dst_capcacity_input = (dst_capacity < LZ4_MAX_INPUT_SIZE) ? int32_t(dst_capacity) : LZ4_MAX_INPUT_SIZE;
    int32_t decomp_size = LZ4_decompress_safe((const char *)src, (char *)dst, src_len_input, dst_capcacity_input);

    if (decomp_size < 0)
    {
        memset(dst, 0, dst_capacity);
        return 0;
    }

    return size_t(decomp_size);
}

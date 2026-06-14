#include "KipProcess.h"
#include "Report.hpp"
#include <pietendo/hac/KernelCapabilityUtil.h>
#include <tc/NotImplementedException.h>

nstool::KipProcess::KipProcess() : mModuleName("nstool::KipProcess"), mFile(), mVerify(false) {}

void nstool::KipProcess::process()
{
    importHeader();

    // importCodeSegments(); // code segments not imported because compression not supported yet
    displayHeader();
    displayKernelCap(mHdr.getKernelCapabilities());
}

void nstool::KipProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::KipProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

void nstool::KipProcess::importHeader()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size is smaller than KIP header size
    if (tc::io::IOUtil::castInt64ToSize(mFile->length()) < sizeof(pie::hac::sKipHeader))
    {
        throw tc::Exception(mModuleName, "Corrupt KIP: file too small.");
    }

    // read kip
    tc::ByteData scratch = tc::ByteData(sizeof(pie::hac::sKipHeader));
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // parse kip header
    mHdr.fromBytes(scratch.data(), scratch.size());
}

void nstool::KipProcess::importCodeSegments()
{
    tc::ByteData scratch;

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
            throw tc::Exception(mModuleName, "KIP text segment failed to decompress");
        }
    }
    else
    {
        // read text segment directly (not compressed)
        mTextBlob = tc::ByteData(mHdr.getTextSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getTextSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mTextBlob.data(), mTextBlob.size());
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
            throw tc::Exception(mModuleName, "KIP ro segment failed to decompress");
        }
    }
    else
    {
        // read ro segment directly (not compressed)
        mRoBlob = tc::ByteData(mHdr.getRoSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getRoSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mRoBlob.data(), mRoBlob.size());
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
            throw tc::Exception(mModuleName, "KIP data segment failed to decompress");
        }
    }
    else
    {
        // read ro segment directly (not compressed)
        mDataBlob = tc::ByteData(mHdr.getDataSegmentInfo().file_layout.size);
        mFile->seek(mHdr.getDataSegmentInfo().file_layout.offset, tc::io::SeekOrigin::Begin);
        mFile->read(mDataBlob.data(), mDataBlob.size());
    }
}

size_t nstool::KipProcess::decompressData(const byte_t *src, size_t src_len, byte_t *dst, size_t dst_capacity)
{
    throw tc::NotImplementedException(mModuleName, "KIP decompression not implemented yet.");
}

void nstool::KipProcess::displayHeader()
{
    Report &r = get_report();

    r.text("[KIP Header]");
    r.text("  Meta:");
    r.text(fmt::format("    Name:                {:s}", mHdr.getName()));
    r.text(fmt::format("    TitleId:             0x{:016x}", mHdr.getTitleId()));
    r.text(fmt::format("    Version:             v{:d}", mHdr.getVersion()));
    r.text(fmt::format("    Is64BitInstruction:  {}", mHdr.getIs64BitInstructionFlag()));
    r.text(fmt::format("    Is64BitAddressSpace: {}", mHdr.getIs64BitAddressSpaceFlag()));
    r.text(fmt::format("    UseSecureMemory:     {}", mHdr.getUseSecureMemoryFlag()));
    r.text("  Program Sections:");
    r.text("     .text:");
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getTextSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getTextSegmentInfo().file_layout.size,
            (mHdr.getTextSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getTextSegmentInfo().memory_layout.offset));
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getTextSegmentInfo().memory_layout.size));
    r.text("    .ro:");
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getRoSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getRoSegmentInfo().file_layout.size,
            (mHdr.getRoSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getRoSegmentInfo().memory_layout.offset));
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getRoSegmentInfo().memory_layout.size));
    r.text("    .data:");
    r.text(
        fmt::format("      FileOffset:     0x{:x}", mHdr.getDataSegmentInfo().file_layout.offset),
        Report::TextType::Layout);
    r.text(
        fmt::format(
            "      FileSize:       0x{:x}{:s}", mHdr.getDataSegmentInfo().file_layout.size,
            (mHdr.getDataSegmentInfo().is_compressed ? " (COMPRESSED)" : "")),
        Report::TextType::Layout);
    r.text(fmt::format("      MemoryOffset:   0x{:x}", mHdr.getDataSegmentInfo().memory_layout.offset));
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getDataSegmentInfo().memory_layout.size));
    r.text("    .bss:");
    r.text(fmt::format("      MemorySize:     0x{:x}", mHdr.getBssSize()));

    r.set("data.kipHeader.meta.name", mHdr.getName());
    r.set("data.kipHeader.meta.titleId", fmt::format("0x{:016x}", mHdr.getTitleId()));
    r.set("data.kipHeader.meta.version", mHdr.getVersion());
    r.set("data.kipHeader.meta.is64BitInstruction", mHdr.getIs64BitInstructionFlag());
    r.set("data.kipHeader.meta.is64BitAddressSpace", mHdr.getIs64BitAddressSpaceFlag());
    r.set("data.kipHeader.meta.useSecureMemory", mHdr.getUseSecureMemoryFlag());
    r.set(
        "data.kipHeader.meta.program.text.fileOffset",
        fmt::format("0x{:x}", mHdr.getTextSegmentInfo().file_layout.offset));
    r.set(
        "data.kipHeader.meta.program.text.fileSize", fmt::format("0x{:x}", mHdr.getTextSegmentInfo().file_layout.size));
    r.set("data.kipHeader.meta.program.text.isCompressed", mHdr.getTextSegmentInfo().is_compressed);
    r.set(
        "data.kipHeader.meta.program.text.memoryOffset",
        fmt::format("0x{:x}", mHdr.getTextSegmentInfo().memory_layout.offset));
    r.set(
        "data.kipHeader.meta.program.text.memorySize",
        fmt::format("0x{:x}", mHdr.getTextSegmentInfo().memory_layout.size));
    r.set(
        "data.kipHeader.meta.program.ro.fileOffset", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().file_layout.offset));
    r.set("data.kipHeader.meta.program.ro.fileSize", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().file_layout.size));
    r.set("data.kipHeader.meta.program.ro.isCompressed", mHdr.getRoSegmentInfo().is_compressed);
    r.set(
        "data.kipHeader.meta.program.ro.memoryOffset",
        fmt::format("0x{:x}", mHdr.getRoSegmentInfo().memory_layout.offset));
    r.set(
        "data.kipHeader.meta.program.ro.memorySize", fmt::format("0x{:x}", mHdr.getRoSegmentInfo().memory_layout.size));
    r.set(
        "data.kipHeader.meta.program.data.fileOffset",
        fmt::format("0x{:x}", mHdr.getDataSegmentInfo().file_layout.offset));
    r.set(
        "data.kipHeader.meta.program.data.fileSize", fmt::format("0x{:x}", mHdr.getDataSegmentInfo().file_layout.size));
    r.set("data.kipHeader.meta.program.data.isCompressed", mHdr.getDataSegmentInfo().is_compressed);
    r.set(
        "data.kipHeader.meta.program.data.memoryOffset",
        fmt::format("0x{:x}", mHdr.getDataSegmentInfo().memory_layout.offset));
    r.set(
        "data.kipHeader.meta.program.data.memorySize",
        fmt::format("0x{:x}", mHdr.getDataSegmentInfo().memory_layout.size));
    r.set("data.kipHeader.meta.program.bss.memorySize", fmt::format("0x{:x}", mHdr.getBssSize()));
}

void nstool::KipProcess::displayKernelCap(const pie::hac::KernelCapabilityControl &kern)
{
    Report &r = get_report();

    r.text("[Kernel Capabilities]");

    if (kern.getThreadInfo().isSet())
    {
        const auto &threadInfo = kern.getThreadInfo();

        r.text("  Thread Priority:");
        r.text(fmt::format("    Min:     {:d}", threadInfo.getMinPriority()));
        r.text(fmt::format("    Max:     {:d}", threadInfo.getMaxPriority()));
        r.text("  CpuId:");
        r.text(fmt::format("    Min:     {:d}", threadInfo.getMinCpuId()));
        r.text(fmt::format("    Max:     {:d}", threadInfo.getMaxCpuId()));

        r.set(
            "data.kernel.threadPriority",
            nlohmann::json{{"min", threadInfo.getMinPriority()}, {"max", threadInfo.getMaxPriority()}});
        r.set(
            "data.kernel.cpuId", nlohmann::json{{"min", threadInfo.getMinCpuId()}, {"max", threadInfo.getMaxCpuId()}});
    }

    if (kern.getSystemCalls().isSet())
    {
        auto syscall_ids = kern.getSystemCalls().getSystemCallIds();

        r.text("  SystemCalls:");

        std::vector<std::string> syscall_names;

        for (size_t syscall_id = 0; syscall_id < syscall_ids.size(); syscall_id++)
        {
            if (syscall_ids.test(syscall_id))
            {
                std::string callString =
                    pie::hac::KernelCapabilityUtil::getSystemCallIdAsString(pie::hac::kc::SystemCallId(syscall_id));

                syscall_names.push_back(callString);

                r.push("data.systemCalls", callString);
            }
        }

        r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(syscall_names, 60, 4)));
    }

    if (kern.getMemoryMaps().isSet())
    {
        auto maps = kern.getMemoryMaps().getMemoryMaps();
        auto ioMaps = kern.getMemoryMaps().getIoMemoryMaps();

        r.text("  MemoryMaps:");

        for (size_t i = 0; i < maps.size(); i++)
        {
            r.text(fmt::format("    {:s}", formatMappingAsString(maps[i])));

            r.push("data.memoryMaps", formatMappingAsString(maps[i]));
        }

        for (size_t i = 0; i < ioMaps.size(); i++)
        {
            r.text(fmt::format("    {:s}", formatMappingAsString(ioMaps[i])));

            r.push("data.ioMemoryMaps", formatMappingAsString(ioMaps[i]));
        }
    }

    if (kern.getInterupts().isSet())
    {
        std::vector<std::string> interupts;

        for (auto itr = kern.getInterupts().getInteruptList().begin();
             itr != kern.getInterupts().getInteruptList().end(); itr++)
        {
            interupts.push_back(fmt::format("0x{:x}", *itr));

            r.push("data.interuptFlags", fmt::format("0x{:x}", *itr));
        }

        r.text("  Interupts Flags:");
        r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(interupts, 60, 4)));
    }

    if (kern.getMiscParams().isSet())
    {
        r.text(fmt::format(
            "  ProgramType:        {:s} ({:d})",
            pie::hac::KernelCapabilityUtil::getProgramTypeAsString(kern.getMiscParams().getProgramType()),
            (uint32_t)kern.getMiscParams().getProgramType()));

        r.set(
            "data.programType", nlohmann::json{
                                    {"string", pie::hac::KernelCapabilityUtil::getProgramTypeAsString(
                                                   kern.getMiscParams().getProgramType())},
                                    {"int", (uint32_t)kern.getMiscParams().getProgramType()}});
    }

    if (kern.getKernelVersion().isSet())
    {
        r.text(fmt::format(
            "  Kernel Version:     {:d}.{:d}", kern.getKernelVersion().getVerMajor(),
            kern.getKernelVersion().getVerMinor()));

        r.set(
            "data.kernelVersion",
            fmt::format("{:d}.{:d}", kern.getKernelVersion().getVerMajor(), kern.getKernelVersion().getVerMinor()));
    }

    if (kern.getHandleTableSize().isSet())
    {
        r.text(fmt::format("  Handle Table Size:  0x{:x}", kern.getHandleTableSize().getHandleTableSize()));

        r.set("data.handleTableSize", fmt::format("0x{:x}", kern.getHandleTableSize().getHandleTableSize()));
    }

    if (kern.getMiscFlags().isSet())
    {
        auto misc_flags = kern.getMiscFlags().getMiscFlags();

        r.text("  Misc Flags:");

        std::vector<std::string> misc_flags_names;

        for (size_t misc_flags_bit = 0; misc_flags_bit < misc_flags.size(); misc_flags_bit++)
        {
            if (misc_flags.test(misc_flags_bit))
            {
                misc_flags_names.push_back(pie::hac::KernelCapabilityUtil::getMiscFlagsBitAsString(
                    pie::hac::kc::MiscFlagsBit(misc_flags_bit)));

                r.set(
                    "data.miscellaneousFlags", pie::hac::KernelCapabilityUtil::getMiscFlagsBitAsString(
                                                   pie::hac::kc::MiscFlagsBit(misc_flags_bit)));
            }
        }

        r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(misc_flags_names, 60, 4)));
    }
}

std::string nstool::KipProcess::formatMappingAsString(const pie::hac::MemoryMappingHandler::sMemoryMapping &map) const
{
    return fmt::format(
        "0x{:016x} - 0x{:016x} (perm={:s}) (type={:s})", ((uint64_t)map.addr << 12),
        (((uint64_t)(map.addr + map.size) << 12) - 1),
        pie::hac::KernelCapabilityUtil::getMemoryPermissionAsString(map.perm),
        pie::hac::KernelCapabilityUtil::getMappingTypeAsString(map.type));
}

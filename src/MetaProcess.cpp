#include "MetaProcess.h"
#include "Report.hpp"
#include <pietendo/hac/AccessControlInfoUtil.h>
#include <pietendo/hac/FileSystemAccessUtil.h>
#include <pietendo/hac/KernelCapabilityUtil.h>
#include <pietendo/hac/MetaUtil.h>

nstool::MetaProcess::MetaProcess() : mModuleName("nstool::MetaProcess"), mFile(), mVerify(false) {}

void nstool::MetaProcess::process()
{
    importMeta();

    if (mVerify)
    {
        validateAcidSignature(mMeta.getAccessControlInfoDesc(), mMeta.getAccessControlInfoDescKeyGeneration());
        validateAciFromAcid(mMeta.getAccessControlInfo(), mMeta.getAccessControlInfoDesc());
    }

    // npdm binary
    displayMetaHeader(mMeta);

    // aci binary
    displayAciHdr(mMeta.getAccessControlInfo());
    displayFac(mMeta.getAccessControlInfo().getFileSystemAccessControl());
    displaySac(mMeta.getAccessControlInfo().getServiceAccessControl());
    displayKernelCap(mMeta.getAccessControlInfo().getKernelCapabilities());

    // acid binary
    displayAciDescHdr(mMeta.getAccessControlInfoDesc());
    displayFac(mMeta.getAccessControlInfoDesc().getFileSystemAccessControl());
    displaySac(mMeta.getAccessControlInfoDesc().getServiceAccessControl());
    displayKernelCap(mMeta.getAccessControlInfoDesc().getKernelCapabilities());
}

void nstool::MetaProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::MetaProcess::setKeyCfg(const KeyBag &keycfg)
{
    mKeyCfg = keycfg;
}

void nstool::MetaProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

const pie::hac::Meta &nstool::MetaProcess::getMeta() const
{
    return mMeta;
}

void nstool::MetaProcess::importMeta()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size is greater than 20MB, don't import.
    size_t file_size = tc::io::IOUtil::castInt64ToSize(mFile->length());

    if (file_size > (0x100000 * 20))
    {
        throw tc::Exception(mModuleName, "File too large.");
    }

    // read meta
    tc::ByteData scratch = tc::ByteData(file_size);
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    mMeta.fromBytes(scratch.data(), scratch.size());
}

void nstool::MetaProcess::validateAcidSignature(const pie::hac::AccessControlInfoDesc &acid, byte_t key_generation)
{
    try
    {
        if (mKeyCfg.acid_sign_key.find(key_generation) == mKeyCfg.acid_sign_key.end())
        {
            throw tc::Exception("Failed to load rsa public key");
        }

        acid.validateSignature(mKeyCfg.acid_sign_key.at(key_generation));
    }
    catch (tc::Exception &e)
    {
        Report &r = get_report();

        r.text(fmt::format("[WARNING] ACID Signature: FAIL ({:s})", e.error()));

        r.push(
            "events",
            nlohmann::json{{"severity", "warn"}, {"message", fmt::format("ACID Signature: FAIL ({:s})", e.error())}});
    }
}

void nstool::MetaProcess::validateAciFromAcid(
    const pie::hac::AccessControlInfo &aci, const pie::hac::AccessControlInfoDesc &acid)
{
    Report &r = get_report();

    // check Program ID
    if (acid.getProgramIdRestrict().min > 0 && aci.getProgramId() < acid.getProgramIdRestrict().min)
    {
        r.text(fmt::format("[WARNING] ACI ProgramId: FAIL (Outside Legal Range)"));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"}, {"message", fmt::format("ACI ProgramId: FAIL (Outside Legal Range)")}});
    }
    else if (acid.getProgramIdRestrict().max > 0 && aci.getProgramId() > acid.getProgramIdRestrict().max)
    {
        r.text(fmt::format("[WARNING] ACI ProgramId: FAIL (Outside Legal Range)"));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"}, {"message", fmt::format("ACI ProgramId: FAIL (Outside Legal Range)")}});
    }

    auto fs_access = aci.getFileSystemAccessControl().getFsAccess();
    auto desc_fs_access = acid.getFileSystemAccessControl().getFsAccess();

    for (size_t i = 0; i < fs_access.size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0; j < desc_fs_access.size() && rightFound == false; j++)
        {
            if (fs_access[i] == desc_fs_access[j])
                rightFound = true;
        }

        if (rightFound == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/FAC FsaRights: FAIL ({:s} not permitted)",
                pie::hac::FileSystemAccessUtil::getFsAccessFlagAsString(fs_access[i])));

            r.push(
                "events", nlohmann::json{
                              {"severity", "warn"},
                              {"message", fmt::format(
                                              "ACI/FAC FsaRights: FAIL ({:s} not permitted)",
                                              pie::hac::FileSystemAccessUtil::getFsAccessFlagAsString(fs_access[i]))}});
        }
    }

    for (size_t i = 0; i < aci.getFileSystemAccessControl().getContentOwnerIdList().size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0; j < acid.getFileSystemAccessControl().getContentOwnerIdList().size() && rightFound == false;
             j++)
        {
            if (aci.getFileSystemAccessControl().getContentOwnerIdList()[i] ==
                acid.getFileSystemAccessControl().getContentOwnerIdList()[j])
                rightFound = true;
        }

        if (rightFound == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/FAC ContentOwnerId: FAIL (0x{:016x} not permitted)",
                aci.getFileSystemAccessControl().getContentOwnerIdList()[i]));

            r.push(
                "events", nlohmann::json{
                              {"severity", "warn"},
                              {"message", fmt::format(
                                              "ACI/FAC ContentOwnerId: FAIL (0x{:016x} not permitted)",
                                              aci.getFileSystemAccessControl().getContentOwnerIdList()[i])}});
        }
    }

    // See https://github.com/jakcron/nstool/issues/92
    // Nintendo doesn't populate SaveDataOwnerIdList in ACID, so this field cannot be verified
#if 0
	for (size_t i = 0; i < aci.getFileSystemAccessControl().getSaveDataOwnerIdList().size(); i++)
	{
		bool rightFound = false;

		for (size_t j = 0; j < acid.getFileSystemAccessControl().getSaveDataOwnerIdList().size() && rightFound == false; j++)
		{
			if (aci.getFileSystemAccessControl().getSaveDataOwnerIdList()[i] == acid.getFileSystemAccessControl().getSaveDataOwnerIdList()[j]) {
				rightFound = true;
			}
		}

		if (rightFound == false)
		{
			r.text(fmt::format("[WARNING] ACI/FAC SaveDataOwnerId: FAIL (0x{:016x} ({:d}) not permitted)", aci.getFileSystemAccessControl().getSaveDataOwnerIdList()[i].id, (uint32_t)aci.getFileSystemAccessControl().getSaveDataOwnerIdList()[i].access_type));

			r.push("events", nlohmann::json{
				{"severity", "warn"},
				{"message", fmt::format("ACI/FAC SaveDataOwnerId: FAIL (0x{:016x} ({:d}) not permitted)", aci.getFileSystemAccessControl().getSaveDataOwnerIdList()[i].id, (uint32_t)aci.getFileSystemAccessControl().getSaveDataOwnerIdList()[i].access_type)}
			});
		}
	}
#endif

    // check SAC
    for (size_t i = 0; i < aci.getServiceAccessControl().getServiceList().size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0; j < acid.getServiceAccessControl().getServiceList().size() && rightFound == false; j++)
        {
            if (aci.getServiceAccessControl().getServiceList()[i] == acid.getServiceAccessControl().getServiceList()[j])
            {
                rightFound = true;
            }
        }

        if (rightFound == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/SAC ServiceList: FAIL ({:s}{:s} not permitted)",
                aci.getServiceAccessControl().getServiceList()[i].getName(),
                (aci.getServiceAccessControl().getServiceList()[i].isServer() ? " (Server)" : "")));

            r.push(
                "events",
                nlohmann::json{
                    {"severity", "warn"},
                    {"message",
                     fmt::format(
                         "ACI/SAC ServiceList: FAIL ({:s}{:s} not permitted)",
                         aci.getServiceAccessControl().getServiceList()[i].getName(),
                         (aci.getServiceAccessControl().getServiceList()[i].isServer() ? " (Server)" : ""))}});
        }
    }

    // check KC
    // check thread info
    if (aci.getKernelCapabilities().getThreadInfo().getMaxCpuId() !=
        acid.getKernelCapabilities().getThreadInfo().getMaxCpuId())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC ThreadInfo/MaxCpuId: FAIL ({:d} not permitted)",
            aci.getKernelCapabilities().getThreadInfo().getMaxCpuId()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC ThreadInfo/MaxCpuId: FAIL ({:d} not permitted)",
                                          aci.getKernelCapabilities().getThreadInfo().getMaxCpuId())}});
    }

    if (aci.getKernelCapabilities().getThreadInfo().getMinCpuId() !=
        acid.getKernelCapabilities().getThreadInfo().getMinCpuId())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC ThreadInfo/MinCpuId: FAIL ({:d} not permitted)",
            aci.getKernelCapabilities().getThreadInfo().getMinCpuId()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC ThreadInfo/MinCpuId: FAIL ({:d} not permitted)",
                                          aci.getKernelCapabilities().getThreadInfo().getMinCpuId())}});
    }

    if (aci.getKernelCapabilities().getThreadInfo().getMaxPriority() !=
        acid.getKernelCapabilities().getThreadInfo().getMaxPriority())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC ThreadInfo/MaxPriority: FAIL ({:d} not permitted)",
            aci.getKernelCapabilities().getThreadInfo().getMaxPriority()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC ThreadInfo/MaxPriority: FAIL ({:d} not permitted)",
                                          aci.getKernelCapabilities().getThreadInfo().getMaxPriority())}});
    }

    if (aci.getKernelCapabilities().getThreadInfo().getMinPriority() !=
        acid.getKernelCapabilities().getThreadInfo().getMinPriority())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC ThreadInfo/MinPriority: FAIL ({:d} not permitted)",
            aci.getKernelCapabilities().getThreadInfo().getMinPriority()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC ThreadInfo/MinPriority: FAIL ({:d} not permitted)",
                                          aci.getKernelCapabilities().getThreadInfo().getMinPriority())}});
    }

    // check system calls
    auto syscall_ids = aci.getKernelCapabilities().getSystemCalls().getSystemCallIds();
    auto desc_syscall_ids = acid.getKernelCapabilities().getSystemCalls().getSystemCallIds();

    for (size_t i = 0; i < syscall_ids.size(); i++)
    {
        if (syscall_ids.test(i) && desc_syscall_ids.test(i) == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/KC SystemCallList: FAIL ({:s} not permitted)",
                pie::hac::KernelCapabilityUtil::getSystemCallIdAsString(pie::hac::kc::SystemCallId(i))));

            r.push(
                "events",
                nlohmann::json{
                    {"severity", "warn"},
                    {"message",
                     fmt::format(
                         "ACI/KC SystemCallList: FAIL ({:s} not permitted)",
                         pie::hac::KernelCapabilityUtil::getSystemCallIdAsString(pie::hac::kc::SystemCallId(i)))}});
        }
    }

    // check memory maps
    for (size_t i = 0; i < aci.getKernelCapabilities().getMemoryMaps().getMemoryMaps().size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0;
             j < acid.getKernelCapabilities().getMemoryMaps().getMemoryMaps().size() && rightFound == false; j++)
        {
            if (aci.getKernelCapabilities().getMemoryMaps().getMemoryMaps()[i] ==
                acid.getKernelCapabilities().getMemoryMaps().getMemoryMaps()[j])
            {
                rightFound = true;
            }
        }

        if (rightFound == false)
        {
            const auto &map = aci.getKernelCapabilities().getMemoryMaps().getMemoryMaps()[i];

            r.text(fmt::format("[WARNING] ACI/KC MemoryMap: FAIL ({:s} not permitted)", formatMappingAsString(map)));

            r.push(
                "events",
                nlohmann::json{
                    {"severity", "warn"},
                    {"message",
                     fmt::format("ACI/KC MemoryMap: FAIL ({:s} not permitted)", formatMappingAsString(map))}});
        }
    }

    for (size_t i = 0; i < aci.getKernelCapabilities().getMemoryMaps().getIoMemoryMaps().size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0;
             j < acid.getKernelCapabilities().getMemoryMaps().getIoMemoryMaps().size() && rightFound == false; j++)
        {
            if (aci.getKernelCapabilities().getMemoryMaps().getIoMemoryMaps()[i] ==
                acid.getKernelCapabilities().getMemoryMaps().getIoMemoryMaps()[j])
            {
                rightFound = true;
            }
        }

        if (rightFound == false)
        {
            const auto &map = aci.getKernelCapabilities().getMemoryMaps().getIoMemoryMaps()[i];

            r.text(fmt::format("[WARNING] ACI/KC IoMemoryMap: FAIL ({:s} not permitted)", formatMappingAsString(map)));

            r.push(
                "events",
                nlohmann::json{
                    {"severity", "warn"},
                    {"message",
                     fmt::format("ACI/KC IoMemoryMap: FAIL ({:s} not permitted)", formatMappingAsString(map))}});
        }
    }

    // check interupts
    for (size_t i = 0; i < aci.getKernelCapabilities().getInterupts().getInteruptList().size(); i++)
    {
        bool rightFound = false;

        for (size_t j = 0;
             j < acid.getKernelCapabilities().getInterupts().getInteruptList().size() && rightFound == false; j++)
        {
            if (aci.getKernelCapabilities().getInterupts().getInteruptList()[i] ==
                acid.getKernelCapabilities().getInterupts().getInteruptList()[j])
            {
                rightFound = true;
            }
        }

        if (rightFound == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/KC InteruptsList: FAIL (0x{:x} not permitted)",
                aci.getKernelCapabilities().getInterupts().getInteruptList()[i]));

            r.push(
                "events", nlohmann::json{
                              {"severity", "warn"},
                              {"message", fmt::format(
                                              "ACI/KC InteruptsList: FAIL (0x{:x} not permitted)",
                                              aci.getKernelCapabilities().getInterupts().getInteruptList()[i])}});
        }
    }

    // check misc params
    if (aci.getKernelCapabilities().getMiscParams().getProgramType() !=
        acid.getKernelCapabilities().getMiscParams().getProgramType())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC ProgramType: FAIL ({:d} not permitted)",
            (uint32_t)aci.getKernelCapabilities().getMiscParams().getProgramType()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC ProgramType: FAIL ({:d} not permitted)",
                                          (uint32_t)aci.getKernelCapabilities().getMiscParams().getProgramType())}});
    }

    // check kernel version
    uint32_t aciKernelVersion = (uint32_t)aci.getKernelCapabilities().getKernelVersion().getVerMajor() << 16 |
                                (uint32_t)aci.getKernelCapabilities().getKernelVersion().getVerMinor();
    uint32_t acidKernelVersion = (uint32_t)acid.getKernelCapabilities().getKernelVersion().getVerMajor() << 16 |
                                 (uint32_t)acid.getKernelCapabilities().getKernelVersion().getVerMinor();

    if (aciKernelVersion < acidKernelVersion)
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC RequiredKernelVersion: FAIL ({:d}.{:d} not permitted)",
            aci.getKernelCapabilities().getKernelVersion().getVerMajor(),
            aci.getKernelCapabilities().getKernelVersion().getVerMinor()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC RequiredKernelVersion: FAIL ({:d}.{:d} not permitted)",
                                          aci.getKernelCapabilities().getKernelVersion().getVerMajor(),
                                          aci.getKernelCapabilities().getKernelVersion().getVerMinor())}});
    }

    // check handle table size
    if (aci.getKernelCapabilities().getHandleTableSize().getHandleTableSize() >
        acid.getKernelCapabilities().getHandleTableSize().getHandleTableSize())
    {
        r.text(fmt::format(
            "[WARNING] ACI/KC HandleTableSize: FAIL (0x{:x} too large)",
            aci.getKernelCapabilities().getHandleTableSize().getHandleTableSize()));

        r.push(
            "events", nlohmann::json{
                          {"severity", "warn"},
                          {"message", fmt::format(
                                          "ACI/KC HandleTableSize: FAIL (0x{:x} too large)",
                                          aci.getKernelCapabilities().getHandleTableSize().getHandleTableSize())}});
    }

    // check misc flags
    auto misc_flags = aci.getKernelCapabilities().getMiscFlags().getMiscFlags();
    auto desc_misc_flags = acid.getKernelCapabilities().getMiscFlags().getMiscFlags();

    for (size_t i = 0; i < misc_flags.size(); i++)
    {
        if (misc_flags.test(i) && desc_misc_flags.test(i) == false)
        {
            r.text(fmt::format(
                "[WARNING] ACI/KC MiscFlag: FAIL ({:s} not permitted)",
                pie::hac::KernelCapabilityUtil::getMiscFlagsBitAsString(pie::hac::kc::MiscFlagsBit(i))));

            r.push(
                "events",
                nlohmann::json{
                    {"severity", "warn"},
                    {"message",
                     fmt::format(
                         "ACI/KC MiscFlag: FAIL ({:s} not permitted)",
                         pie::hac::KernelCapabilityUtil::getMiscFlagsBitAsString(pie::hac::kc::MiscFlagsBit(i)))}});
        }
    }
}

void nstool::MetaProcess::displayMetaHeader(const pie::hac::Meta &hdr)
{
    Report &r = get_report();

    r.text("[Meta Header]");
    r.text(fmt::format("  ACID KeyGeneration: {:d}", hdr.getAccessControlInfoDescKeyGeneration()));
    r.text("  Flags:");
    r.text(fmt::format("    Is64BitInstruction:       {}", hdr.getIs64BitInstructionFlag()));
    r.text(fmt::format(
        "    ProcessAddressSpace:      {:s}",
        pie::hac::MetaUtil::getProcessAddressSpaceAsString(hdr.getProcessAddressSpace())));
    r.text(fmt::format("    OptimizeMemoryAllocation: {}", hdr.getOptimizeMemoryAllocationFlag()));
    r.text(fmt::format("  SystemResourceSize: 0x{:x}", hdr.getSystemResourceSize()));
    r.text("  Main Thread Params:");
    r.text(fmt::format("    Priority:      {:d}", hdr.getMainThreadPriority()));
    r.text(fmt::format("    CpuId:         {:d}", hdr.getMainThreadCpuId()));
    r.text(fmt::format("    StackSize:     0x{:x}", hdr.getMainThreadStackSize()));
    r.text("  TitleInfo:");
    r.text(fmt::format("    Version:       v{:d}", hdr.getVersion()));
    r.text(fmt::format("    Name:          {:s}", hdr.getName()));

    r.set("data.metaHeader.acidKeyGeneration", hdr.getAccessControlInfoDescKeyGeneration());
    r.set("data.metaHeader.flags.is64BitInstruction", hdr.getIs64BitInstructionFlag());
    r.set(
        "data.metaHeader.flags.processAddressSpace",
        pie::hac::MetaUtil::getProcessAddressSpaceAsString(hdr.getProcessAddressSpace()));
    r.set("data.metaHeader.flags.optimizeMemoryAllocation", hdr.getOptimizeMemoryAllocationFlag());
    r.set("data.metaHeader.systemResourceSize", fmt::format("0x{:x}", hdr.getSystemResourceSize()));
    r.set("data.metaHeader.mainThreadParameters.priority", hdr.getMainThreadPriority());
    r.set("data.metaHeader.mainThreadParameters.cpuId", hdr.getMainThreadCpuId());
    r.set("data.metaHeader.mainThreadParameters.stackSize", fmt::format("0x{:x}", hdr.getMainThreadStackSize()));
    r.set("data.metaHeader.title.version", hdr.getVersion());
    r.set("data.metaHeader.title.name", hdr.getName());

    if (hdr.getProductCode().length())
    {
        r.text(fmt::format("    ProductCode:   {:s}", hdr.getProductCode()));

        r.set("data.metaHeader.title.productCode", hdr.getProductCode());
    }
}

void nstool::MetaProcess::displayAciHdr(const pie::hac::AccessControlInfo &aci)
{
    Report &r = get_report();

    r.text("[Access Control Info]");
    r.text(fmt::format("  ProgramID:       0x{:016x}", aci.getProgramId()));

    r.set("data.accessControl.programId", fmt::format("0x{:016x}", aci.getProgramId()));
}

void nstool::MetaProcess::displayAciDescHdr(const pie::hac::AccessControlInfoDesc &acid)
{
    Report &r = get_report();

    r.text("[Access Control Info Desc]");
    r.text("  Flags:");
    r.text(fmt::format("    Production:            {}", acid.getProductionFlag()));
    r.text(fmt::format("    Unqualified Approval:  {}", acid.getUnqualifiedApprovalFlag()));
    r.text(fmt::format(
        "    Memory Region:         {:s} ({:d})",
        pie::hac::AccessControlInfoUtil::getMemoryRegionAsString(acid.getMemoryRegion()),
        (uint32_t)acid.getMemoryRegion()));
    r.text("  ProgramID Restriction");
    r.text(fmt::format("    Min:           0x{:016x}", acid.getProgramIdRestrict().min));
    r.text(fmt::format("    Max:           0x{:016x}", acid.getProgramIdRestrict().max));

    r.set("data.accessControl.flags.production", acid.getProductionFlag());
    r.set("data.accessControl.flags.unqualifiedApproval", acid.getUnqualifiedApprovalFlag());
    r.set(
        "data.accessControl.flags.memoryRegion",
        nlohmann::json{
            {"string", pie::hac::AccessControlInfoUtil::getMemoryRegionAsString(acid.getMemoryRegion())},
            {"int", (uint32_t)acid.getMemoryRegion()}});
    r.set(
        "data.accessControl.programIdRestriction",
        nlohmann::json{
            {"min", fmt::format("0x{:016x}", acid.getProgramIdRestrict().min)},
            {"max", fmt::format("0x{:016x}", acid.getProgramIdRestrict().max)}});
}

void nstool::MetaProcess::displayFac(const pie::hac::FileSystemAccessControl &fac)
{
    Report &r = get_report();

    r.text("[FS Access Control]");
    r.text(fmt::format("  Format Version:  {:d}", fac.getFormatVersion()));

    r.set("data.fsAccessControl.formatVersion", fac.getFormatVersion());

    if (fac.getFsAccess().size())
    {
        std::vector<std::string> fs_access_str_list;

        for (auto itr = fac.getFsAccess().begin(); itr != fac.getFsAccess().end(); itr++)
        {
            std::string flag_string =
                pie::hac::FileSystemAccessUtil::getFsAccessFlagAsString(pie::hac::fac::FsAccessFlag(*itr));

            fs_access_str_list.push_back(fmt::format("{:s} (bit {:d})", flag_string, (uint32_t)*itr));

            r.push("data.fsAccessControl.fsAccess", nlohmann::json{{"string", flag_string}, {"int", (uint32_t)*itr}});
        }

        r.text("  FsAccess:");
        r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(fs_access_str_list, 60, 4)));
    }

    if (fac.getContentOwnerIdList().size())
    {
        r.text("  Content Owner IDs:");

        for (size_t i = 0; i < fac.getContentOwnerIdList().size(); i++)
        {
            r.text(fmt::format("    0x{:016x}", fac.getContentOwnerIdList()[i]));

            r.push("data.fsAccessControl.contentOwnerIds", fmt::format("0x{:016x}", fac.getContentOwnerIdList()[i]));
        }
    }

    if (fac.getSaveDataOwnerIdList().size())
    {
        r.text("  Save Data Owner IDs:");

        for (size_t i = 0; i < fac.getSaveDataOwnerIdList().size(); i++)
        {
            r.text(fmt::format(
                "    0x{:016x} ({:s})", fac.getSaveDataOwnerIdList()[i].id,
                pie::hac::FileSystemAccessUtil::getSaveDataOwnerAccessModeAsString(
                    fac.getSaveDataOwnerIdList()[i].access_type)));

            r.push(
                "data.fsAccessControl.saveDataOwnerIds",
                nlohmann::json{
                    {"string", pie::hac::FileSystemAccessUtil::getSaveDataOwnerAccessModeAsString(
                                   fac.getSaveDataOwnerIdList()[i].access_type)},
                    {"hex", fac.getSaveDataOwnerIdList()[i].id}});
        }
    }
}

void nstool::MetaProcess::displaySac(const pie::hac::ServiceAccessControl &sac)
{
    Report &r = get_report();

    r.text("[Service Access Control]");
    r.text("  Service List:");

    std::vector<std::string> service_name_list;

    for (size_t i = 0; i < sac.getServiceList().size(); i++)
    {
        service_name_list.push_back(
            sac.getServiceList()[i].getName() + (sac.getServiceList()[i].isServer() ? "(isSrv)" : ""));

        r.push(
            "data.serviceAccessControl.services",
            nlohmann::json{
                {"name", sac.getServiceList()[i].getName()}, {"isServer", sac.getServiceList()[i].isServer()}});
    }

    r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(service_name_list, 60, 4)));
}

void nstool::MetaProcess::displayKernelCap(const pie::hac::KernelCapabilityControl &kern)
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
            "data.kernelCapabilities.threadPriority",
            nlohmann::json{{"min", threadInfo.getMinPriority()}, {"max", threadInfo.getMaxPriority()}});
        r.set(
            "data.kernelCapabilities.cpuId",
            nlohmann::json{{"min", threadInfo.getMinCpuId()}, {"max", threadInfo.getMaxCpuId()}});
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
                syscall_names.push_back(
                    pie::hac::KernelCapabilityUtil::getSystemCallIdAsString(pie::hac::kc::SystemCallId(syscall_id)));

                r.push(
                    "data.kernelCapabilities.systemCalls",
                    pie::hac::KernelCapabilityUtil::getSystemCallIdAsString(pie::hac::kc::SystemCallId(syscall_id)));
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

            r.push("data.kernelCapabilities.memoryMaps", formatMappingAsString(maps[i]));
        }

        for (size_t i = 0; i < ioMaps.size(); i++)
        {
            r.text(fmt::format("    {:s}", formatMappingAsString(ioMaps[i])));

            r.push("data.kernelCapabilities.memoryMaps", formatMappingAsString(ioMaps[i]));
        }
    }

    if (kern.getInterupts().isSet())
    {
        std::vector<std::string> interupts;

        for (auto itr = kern.getInterupts().getInteruptList().begin();
             itr != kern.getInterupts().getInteruptList().end(); itr++)
        {
            interupts.push_back(fmt::format("0x{:x}", *itr));

            r.push("data.kernelCapabilities.interuptFlags", fmt::format("0x{:x}", *itr));
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

        r.push(
            "data.kernelCapabilities.programType",
            nlohmann::json{
                {"string",
                 pie::hac::KernelCapabilityUtil::getProgramTypeAsString(kern.getMiscParams().getProgramType())},
                {"int", (uint32_t)kern.getMiscParams().getProgramType()}});
    }

    if (kern.getKernelVersion().isSet())
    {
        r.text(fmt::format(
            "  Kernel Version:     {:d}.{:d}", kern.getKernelVersion().getVerMajor(),
            kern.getKernelVersion().getVerMinor()));

        r.push(
            "data.kernelCapabilities.version",
            fmt::format("{:d}.{:d}", kern.getKernelVersion().getVerMajor(), kern.getKernelVersion().getVerMinor()));
    }

    if (kern.getHandleTableSize().isSet())
    {
        r.text(fmt::format("  Handle Table Size:  0x{:x}", kern.getHandleTableSize().getHandleTableSize()));

        r.set(
            "data.kernelCapabilities.handleTableSize",
            fmt::format("0x{:x}", kern.getHandleTableSize().getHandleTableSize()));
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

                r.push(
                    "data.kernelCapabilities.miscellaneousFlags",
                    pie::hac::KernelCapabilityUtil::getMiscFlagsBitAsString(
                        pie::hac::kc::MiscFlagsBit(misc_flags_bit)));
            }
        }

        r.text(fmt::format("{:s}", tc::cli::FormatUtil::formatListWithLineLimit(misc_flags_names, 60, 4)));
    }
}

std::string nstool::MetaProcess::formatMappingAsString(const pie::hac::MemoryMappingHandler::sMemoryMapping &map) const
{
    return fmt::format(
        "0x{:016x} - 0x{:016x} (perm={:s}) (type={:s})", ((uint64_t)map.addr << 12),
        (((uint64_t)(map.addr + map.size) << 12) - 1),
        pie::hac::KernelCapabilityUtil::getMemoryPermissionAsString(map.perm),
        pie::hac::KernelCapabilityUtil::getMappingTypeAsString(map.type));
}

#include "GameCardProcess.h"
#include "FsProcess.h"
#include "Report.hpp"
#include "util.h"
#include <pietendo/hac/ContentArchiveUtil.h>
#include <pietendo/hac/ContentMetaUtil.h>
#include <pietendo/hac/GameCardFsSnapshotGenerator.h>
#include <pietendo/hac/GameCardUtil.h>
#include <tc/crypto.h>
#include <tc/io/IOUtil.h>

nstool::GameCardProcess::GameCardProcess()
    : mModuleName("nstool::GameCardProcess"), mFile(), mCliOutputMode(), mVerify(false), mIsTrueSdkXci(false),
      mIsSdkXciEncrypted(false), mGcHeaderOffset(0), mProccessExtendedHeader(false), mFileSystem(), mFsProcess()
{
}

void nstool::GameCardProcess::process()
{
    importHeader();

    // validate header signature
    if (mVerify)
    {
        validateXciSignature();
    }

    // display header
    displayHeader();

    // process nested HFS0
    processRootPfs();
}

void nstool::GameCardProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::GameCardProcess::setOutputFile(const std::string &file)
{
    mOutputFile = file;
}

void nstool::GameCardProcess::setCliOutputMode(CliOutputMode type)
{
    mCliOutputMode = type;
    mFsProcess.setShowFsInfo(mCliOutputMode.show_basic_info);
}

void nstool::GameCardProcess::setKeyCfg(const KeyBag &keycfg)
{
    mKeyCfg = keycfg;
}

void nstool::GameCardProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

void nstool::GameCardProcess::setShowFsTree(bool show_fs_tree)
{
    mFsProcess.setShowFsTree(show_fs_tree);
}

void nstool::GameCardProcess::setExtractJobs(const std::vector<nstool::ExtractJob> extract_jobs)
{
    mFsProcess.setExtractJobs(extract_jobs);
    mFsProcess.setExtractFile(mOutputFile);
}

void nstool::GameCardProcess::importHeader()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check stream is large enough for header
    if (mFile->length() < tc::io::IOUtil::castSizeToInt64(sizeof(pie::hac::sSdkGcHeader)))
    {
        throw tc::Exception(mModuleName, "Corrupt GameCard Image: File too small.");
    }

    // allocate memory for header
    tc::ByteData scratch = tc::ByteData(sizeof(pie::hac::sSdkGcHeader));

    // read header region
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // determine if this is a SDK XCI or a "Community" XCI
    if (((pie::hac::sSdkGcHeader *)scratch.data())->signed_header.header.st_magic.unwrap() ==
        pie::hac::gc::kGcHeaderStructMagic)
    {
        mIsTrueSdkXci = true;
        mGcHeaderOffset = sizeof(pie::hac::sGcKeyDataRegion);
    }
    else if (
        ((pie::hac::sGcHeader_Rsa2048Signed *)scratch.data())->header.st_magic.unwrap() ==
        pie::hac::gc::kGcHeaderStructMagic)
    {
        mIsTrueSdkXci = false;
        mGcHeaderOffset = 0;
    }
    else
    {
        throw tc::Exception(mModuleName, "Corrupt GameCard Image: Unexpected magic bytes.");
    }

    pie::hac::sGcHeader_Rsa2048Signed *hdr_ptr =
        (pie::hac::sGcHeader_Rsa2048Signed *)(scratch.data() + mGcHeaderOffset);

    // generate hash of raw header
    tc::crypto::GenerateSha2256Hash(mHdrHash.data(), (byte_t *)&hdr_ptr->header, sizeof(pie::hac::sGcHeader));

    // save the signature
    memcpy(mHdrSignature.data(), hdr_ptr->signature.data(), mHdrSignature.size());

    // decrypt extended header
    byte_t xci_header_key_index = hdr_ptr->header.key_flag & 0xf;
    if (mKeyCfg.xci_header_key.find(xci_header_key_index) != mKeyCfg.xci_header_key.end())
    {
        pie::hac::GameCardUtil::decryptXciHeader(&hdr_ptr->header, mKeyCfg.xci_header_key[xci_header_key_index].data());
        mProccessExtendedHeader = true;
    }

    // deserialise header
    mHdr.fromBytes((byte_t *)&hdr_ptr->header, sizeof(pie::hac::sGcHeader));
}

void nstool::GameCardProcess::displayHeader()
{
    Report &r = get_report();

    const pie::hac::sGcHeader *raw_hdr = (const pie::hac::sGcHeader *)mHdr.getBytes().data();

    r.text("[GameCard/Header]");
    r.text(fmt::format("  CardHeaderVersion:      {:d}", mHdr.getCardHeaderVersion()));
    r.text(fmt::format(
        "  RomSize:                {:s}",
        pie::hac::GameCardUtil::getRomSizeAsString((pie::hac::gc::RomSize)mHdr.getRomSizeType())));
    r.text(fmt::format("  RomSize (hex):          (0x{:x})", mHdr.getRomSizeType()), Report::TextType::Extended);
    r.text(fmt::format("  PackageId:              0x{:016x}", mHdr.getPackageId()));
    r.text(fmt::format("  Flags:                  0x{:02x}", *((byte_t *)&raw_hdr->flags)));

    r.set("data.gameCardHeader.cardHeaderVersion", mHdr.getCardHeaderVersion());
    r.set(
        "data.gameCardHeader.romSize",
        nlohmann::json{
            {"string", pie::hac::GameCardUtil::getRomSizeAsString((pie::hac::gc::RomSize)mHdr.getRomSizeType())},
            {"hex", fmt::format("0x{:x}", mHdr.getRomSizeType())}});
    r.set("data.gameCardHeader.packageId", fmt::format("0x{:016x}", mHdr.getPackageId()));
    r.set("data.gameCardHeader.flagHex", fmt::format("0x{:02x}", *((byte_t *)&raw_hdr->flags)));

    for (auto itr = mHdr.getFlags().begin(); itr != mHdr.getFlags().end(); itr++)
    {
        r.text(
            fmt::format("    {:s}", pie::hac::GameCardUtil::getHeaderFlagsAsString((pie::hac::gc::HeaderFlags)*itr)));
        r.push(
            "data.gameCardHeader.flags",
            pie::hac::GameCardUtil::getHeaderFlagsAsString((pie::hac::gc::HeaderFlags)*itr));
    }

    if (mCliOutputMode.show_extended_info)
    {
        fmt::print(
            "  KekIndex:               {:s} ({:d})\n",
            pie::hac::GameCardUtil::getKekIndexAsString((pie::hac::gc::KekIndex)mHdr.getKekIndex()),
            mHdr.getKekIndex());
        fmt::print("  TitleKeyDecIndex:       {:d}\n", mHdr.getTitleKeyDecIndex());
        fmt::print("  InitialData:\n");
        fmt::print("    Hash:\n");
        fmt::print(
            "      {:s}",
            tc::cli::FormatUtil::formatBytesAsStringWithLineLimit(
                mHdr.getInitialDataHash().data(), mHdr.getInitialDataHash().size(), true, "", 0x10, 6, false));
    }

    r.set(
        "data.gameCardHeader.extendedHeaderAesCbcIV",
        tc::cli::FormatUtil::formatBytesAsString(mHdr.getAesCbcIv().data(), mHdr.getAesCbcIv().size(), true, ""));
    r.set("data.gameCardHeader.selSec", fmt::format("0x{:x}", mHdr.getSelSec()));
    r.set("data.gameCardHeader.selT1Key", fmt::format("0x{:x}", mHdr.getSelT1Key()));
    r.set("data.gameCardHeader.selKey", fmt::format("0x{:x}", mHdr.getSelKey()));

    r.text(fmt::format("  RomAreaStartPage:       0x{:x}", mHdr.getRomAreaStartPage()), Report::TextType::Layout);
    r.set("data.gameCardHeader.romAreaStartPage.block", fmt::format("0x{:x}", mHdr.getRomAreaStartPage()));

    if (mHdr.getRomAreaStartPage() != (uint32_t)(-1))
    {
        r.text(
            fmt::format(
                "  RomAreaStartPageAddr    0x{:x}", pie::hac::GameCardUtil::blockToAddr(mHdr.getRomAreaStartPage())),
            Report::TextType::Layout);
        r.set(
            "data.gameCardHeader.romAreaStartPage.addr",
            fmt::format("0x{:x}", pie::hac::GameCardUtil::blockToAddr(mHdr.getRomAreaStartPage())));
    }

    r.text(fmt::format("  BackupAreaStartPage:    0x{:x}", mHdr.getBackupAreaStartPage()), Report::TextType::Layout);
    r.set("data.gameCardHeader.backupAreaStartPage.block", fmt::format("0x{:x}", mHdr.getBackupAreaStartPage()));

    if (mHdr.getBackupAreaStartPage() != (uint32_t)(-1))
    {
        r.text(
            fmt::format(
                "  BackupAreaStartPageAddr: 0x{:x}",
                pie::hac::GameCardUtil::blockToAddr(mHdr.getBackupAreaStartPage())),
            Report::TextType::Layout);
        r.set(
            "data.gameCardHeader.backupAreaStartPage.addr",
            fmt::format("0x{:x}", pie::hac::GameCardUtil::blockToAddr(mHdr.getBackupAreaStartPage())));
    }

    r.text(fmt::format("  ValidDataEndPage:       0x{:x}", mHdr.getValidDataEndPage()), Report::TextType::Layout);
    r.set("data.gameCardHeader.validDataEndPage.block", fmt::format("0x{:x}", mHdr.getValidDataEndPage()));

    if (mHdr.getValidDataEndPage() != (uint32_t)(-1))
    {
        r.text(
            fmt::format(
                "  ValidDataEndPageAddr:   0x{:x}", pie::hac::GameCardUtil::blockToAddr(mHdr.getValidDataEndPage())),
            Report::TextType::Layout);
        r.set(
            "data.gameCardHeader.validDataEndPage.addr",
            fmt::format("0x{:x}", pie::hac::GameCardUtil::blockToAddr(mHdr.getValidDataEndPage())));
    }

    if (mProccessExtendedHeader)
    {
        fmt::print("[GameCard/ExtendedHeader]\n");
        fmt::print(
            "  FwVersion:              v{:d} ({:s})\n", mHdr.getFwVersion(),
            pie::hac::GameCardUtil::getCardFwVersionDescriptionAsString((pie::hac::gc::FwVersion)mHdr.getFwVersion()));
        fmt::print("  AccCtrl1:               0x{:x}\n", mHdr.getAccCtrl1());
        fmt::print(
            "    CardClockRate:        {:s}\n",
            pie::hac::GameCardUtil::getCardClockRateAsString((pie::hac::gc::CardClockRate)mHdr.getAccCtrl1()));
        fmt::print("  Wait1TimeRead:          0x{:x}\n", mHdr.getWait1TimeRead());
        fmt::print("  Wait2TimeRead:          0x{:x}\n", mHdr.getWait2TimeRead());
        fmt::print("  Wait1TimeWrite:         0x{:x}\n", mHdr.getWait1TimeWrite());
        fmt::print("  Wait2TimeWrite:         0x{:x}\n", mHdr.getWait2TimeWrite());
        fmt::print(
            "  SdkAddon Version:       {:s} (v{:d})\n",
            pie::hac::ContentArchiveUtil::getSdkAddonVersionAsString(mHdr.getFwMode()), mHdr.getFwMode());
        fmt::print(
            "  CompatibilityType:      {:s} ({:d})\n",
            pie::hac::GameCardUtil::getCompatibilityTypeAsString(
                (pie::hac::gc::CompatibilityType)mHdr.getCompatibilityType()),
            mHdr.getCompatibilityType());
        fmt::print("  Update Partition Info:\n");
        fmt::print(
            "    CUP Version:          {:s} (v{:d})\n",
            pie::hac::ContentMetaUtil::getVersionAsString(mHdr.getUppVersion()), mHdr.getUppVersion());
        fmt::print("    CUP TitleId:          0x{:016x}\n", mHdr.getUppId());
        fmt::print(
            "    CUP Digest:           {:s}\n",
            tc::cli::FormatUtil::formatBytesAsString(mHdr.getUppHash().data(), mHdr.getUppHash().size(), true, ""));
    }

    std::string partitionFsHash = trimTrailingNewline(tc::cli::FormatUtil::formatBytesAsStringWithLineLimit(
        mHdr.getPartitionFsHash().data(), mHdr.getPartitionFsHash().size(), true, "", 0x10, 6, false));
    std::vector<std::string> lines;

    r.text("  PartitionFs Header:");
    r.text(fmt::format("    Offset:               0x{:x}", mHdr.getPartitionFsAddress()), Report::TextType::Layout);
    r.text(fmt::format("    Size:                 0x{:x}", mHdr.getPartitionFsSize()), Report::TextType::Layout);
    r.text("    Hash:", Report::TextType::Layout);
    r.text(fmt::format("      {:s}", partitionFsHash), Report::TextType::Layout);

    r.set("data.partitionFsHeader.offset", fmt::format("0x{:x}", mHdr.getPartitionFsAddress()));
    r.set("data.partitionFsHeader.size", fmt::format("0x{:x}", mHdr.getPartitionFsSize()));

    lines = splitAndTrimLines(partitionFsHash);

    for (const auto &line : lines)
    {
        r.push("data.partitionFsHeader.hash", line);
    }

    r.text("[GameCard/ExtendedHeader]", Report::TextType::Extended);
    r.text(
        fmt::format(
            "  FwVersion:              v{:d} ({:s})", mHdr.getFwVersion(),
            pie::hac::GameCardUtil::getCardFwVersionDescriptionAsString((pie::hac::gc::FwVersion)mHdr.getFwVersion())),
        Report::TextType::Extended);
    r.text(fmt::format("  AccCtrl1:               0x{:x}", mHdr.getAccCtrl1()), Report::TextType::Extended);
    r.text(
        fmt::format(
            "    CardClockRate:        {:s}",
            pie::hac::GameCardUtil::getCardClockRateAsString((pie::hac::gc::CardClockRate)mHdr.getAccCtrl1())),
        Report::TextType::Extended);
    r.text(fmt::format("  Wait1TimeRead:          0x{:x}", mHdr.getWait1TimeRead()), Report::TextType::Extended);
    r.text(fmt::format("  Wait2TimeRead:          0x{:x}", mHdr.getWait2TimeRead()), Report::TextType::Extended);
    r.text(fmt::format("  Wait1TimeWrite:         0x{:x}", mHdr.getWait1TimeWrite()), Report::TextType::Extended);
    r.text(fmt::format("  Wait2TimeWrite:         0x{:x}", mHdr.getWait2TimeWrite()), Report::TextType::Extended);
    r.text(
        fmt::format(
            "  SdkAddon Version:       {:s} (v{:d})",
            pie::hac::ContentArchiveUtil::getSdkAddonVersionAsString(mHdr.getFwMode()), mHdr.getFwMode()),
        Report::TextType::Extended);
    r.text(
        fmt::format(
            "  CompatibilityType:      {:s} ({:d})",
            pie::hac::GameCardUtil::getCompatibilityTypeAsString(
                (pie::hac::gc::CompatibilityType)mHdr.getCompatibilityType()),
            mHdr.getCompatibilityType()),
        Report::TextType::Extended);
    r.text("  Update Partition Info:", Report::TextType::Extended);
    r.text(
        fmt::format(
            "    CUP Version:          {:s} (v{:d})",
            pie::hac::ContentMetaUtil::getVersionAsString(mHdr.getUppVersion()), mHdr.getUppVersion()),
        Report::TextType::Extended);
    r.text(fmt::format("    CUP TitleId:          0x{:016x}", mHdr.getUppId()), Report::TextType::Extended);
    r.text(
        fmt::format(
            "    CUP Digest:           {:s}",
            tc::cli::FormatUtil::formatBytesAsString(mHdr.getUppHash().data(), mHdr.getUppHash().size(), true, "")),
        Report::TextType::Extended);

    r.set(
        "data.gameCardHeader.firmwareVersion", nlohmann::json{
                                                   {"string", mHdr.getFwVersion()},
                                                   {"int", pie::hac::GameCardUtil::getCardFwVersionDescriptionAsString(
                                                               (pie::hac::gc::FwVersion)mHdr.getFwVersion())}});
    r.set("data.gameCardHeader.accCtrl1", fmt::format("0x{:x}", mHdr.getAccCtrl1()));
    r.set(
        "data.gameCardHeader.cardClockRate",
        pie::hac::GameCardUtil::getCardClockRateAsString((pie::hac::gc::CardClockRate)mHdr.getAccCtrl1()));
    r.set(
        "data.gameCardHeader.wait1Time", nlohmann::json{
                                             {"read", fmt::format("0x{:x}", mHdr.getWait1TimeRead())},
                                             {"write", fmt::format("0x{:x}", mHdr.getWait1TimeWrite())}});
    r.set(
        "data.gameCardHeader.wait2Time", nlohmann::json{
                                             {"read", fmt::format("0x{:x}", mHdr.getWait2TimeRead())},
                                             {"write", fmt::format("0x{:x}", mHdr.getWait2TimeWrite())}});
    r.set(
        "data.gameCardHeader.sdkAddonVersion",
        nlohmann::json{
            {"string", pie::hac::ContentArchiveUtil::getSdkAddonVersionAsString(mHdr.getFwMode())},
            {"int", mHdr.getFwMode()}});
    r.set(
        "data.gameCardHeader.compatibilityType",
        nlohmann::json{
            {"string", pie::hac::GameCardUtil::getCompatibilityTypeAsString(
                           (pie::hac::gc::CompatibilityType)mHdr.getCompatibilityType())},
            {"int", mHdr.getCompatibilityType()}});
    r.set(
        "data.gameCardHeader.updatePartition.cupVersion",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getVersionAsString(mHdr.getUppVersion())},
            {"int", mHdr.getUppVersion()}});
    r.set("data.gameCardHeader.updatePartition.cupTitleId", fmt::format("0x{:016x}", mHdr.getUppId()));
    r.set(
        "data.gameCardHeader.updatePartition.cupDigest",
        tc::cli::FormatUtil::formatBytesAsString(mHdr.getUppHash().data(), mHdr.getUppHash().size(), true, ""));
}

bool nstool::GameCardProcess::validateRegionOfFile(
    int64_t offset, int64_t len, const byte_t *test_hash, bool use_salt, byte_t salt)
{
    // read region into memory
    tc::ByteData scratch = tc::ByteData(tc::io::IOUtil::castInt64ToSize(len));
    mFile->seek(offset, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // update hash
    tc::crypto::Sha2256Generator sha256_gen;
    sha256_gen.initialize();
    sha256_gen.update(scratch.data(), scratch.size());

    if (use_salt)
    {
        sha256_gen.update(&salt, sizeof(salt));
    }

    // calculate hash
    pie::hac::detail::sha256_hash_t calc_hash;
    sha256_gen.getHash(calc_hash.data());

    return memcmp(calc_hash.data(), test_hash, calc_hash.size()) == 0;
}

bool nstool::GameCardProcess::validateRegionOfFile(int64_t offset, int64_t len, const byte_t *test_hash)
{
    return validateRegionOfFile(offset, len, test_hash, false, 0);
}

void nstool::GameCardProcess::validateXciSignature()
{
    Report &r = get_report();

    if (mKeyCfg.xci_header_sign_key.isSet())
    {
        if (tc::crypto::VerifyRsa2048Pkcs1Sha2256(
                mHdrSignature.data(), mHdrHash.data(), mKeyCfg.xci_header_sign_key.get()) == false)
        {
            r.text("[WARNING] GameCard Header Signature: FAIL");

            r.push("events", nlohmann::json{{"severity", "warn"}, {"message", "GameCard Header Signature: FAIL."}});
        }
    }
    else
    {
        r.text("[WARNING] GameCard Header Signature: FAIL (Failed to load rsa public key).");

        r.push(
            "events",
            nlohmann::json{
                {"severity", "warn"}, {"message", "GameCard Header Signature: FAIL (Failed to load rsa public key)."}});
    }
}

void nstool::GameCardProcess::processRootPfs()
{
    Report &r = get_report();

    if (mVerify && validateRegionOfFile(
                       mHdr.getPartitionFsAddress(), mHdr.getPartitionFsSize(), mHdr.getPartitionFsHash().data(),
                       mHdr.getCompatibilityType() != pie::hac::gc::CompatibilityType_Global,
                       mHdr.getCompatibilityType()) == false)
    {
        r.text("[WARNING] GameCard Root HFS0: FAIL (bad hash).");

        r.push("events", nlohmann::json{{"severity", "warn"}, {"message", "GameCard Root HFS0: FAIL (bad hash)."}});
    }

    std::shared_ptr<tc::io::IStream> gc_fs_raw = std::make_shared<tc::io::SubStream>(tc::io::SubStream(
        mFile, mHdr.getPartitionFsAddress(),
        pie::hac::GameCardUtil::blockToAddr(mHdr.getValidDataEndPage() + 1) - mHdr.getPartitionFsAddress()));

    auto gc_vfs_snapshot = pie::hac::GameCardFsSnapshotGenerator(
        gc_fs_raw, mHdr.getPartitionFsSize(),
        mVerify ? pie::hac::GameCardFsSnapshotGenerator::ValidationMode_Warn
                : pie::hac::GameCardFsSnapshotGenerator::ValidationMode_None);
    mFileSystem = std::make_shared<tc::io::VirtualFileSystem>(tc::io::VirtualFileSystem(gc_vfs_snapshot));

    mFsProcess.setInputFileSystem(mFileSystem);
    mFsProcess.setFsFormatName("PartitionFs");
    mFsProcess.setFsProperties(
        {fmt::format("Type:      Nested HFS0"),
         // Subtract 1 to not include the root directory.
         fmt::format(
             "DirNum:    {:d}", gc_vfs_snapshot.dir_entries.empty() ? 0 : gc_vfs_snapshot.dir_entries.size() - 1),
         fmt::format("FileNum:   {:d}", gc_vfs_snapshot.file_entries.size())});
    mFsProcess.setProperties("type", "Nested HFS0");
    mFsProcess.setProperties(
        "dirCount", gc_vfs_snapshot.dir_entries.empty() ? 0 : gc_vfs_snapshot.dir_entries.size() - 1);
    mFsProcess.setProperties("fileCount", gc_vfs_snapshot.file_entries.size());
    mFsProcess.setFsRootLabel(kXciMountPointName);
    mFsProcess.process();
}

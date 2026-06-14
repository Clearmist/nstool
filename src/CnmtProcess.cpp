#include "CnmtProcess.h"
#include "Report.hpp"
#include <pietendo/hac/ContentMetaUtil.h>

nstool::CnmtProcess::CnmtProcess() : mModuleName("nstool::CnmtProcess"), mFile(), mVerify(false) {}

void nstool::CnmtProcess::process()
{
    importCnmt();

    displayCnmt();
}

void nstool::CnmtProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::CnmtProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

const pie::hac::ContentMeta &nstool::CnmtProcess::getContentMeta() const
{
    return mCnmt;
}

void nstool::CnmtProcess::importCnmt()
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
    size_t cnmt_file_size = tc::io::IOUtil::castInt64ToSize(mFile->length());

    if (cnmt_file_size > (0x100000 * 20))
    {
        throw tc::Exception(mModuleName, "File too large.");
    }

    // read cnmt
    tc::ByteData scratch = tc::ByteData(cnmt_file_size);
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    // parse cnmt
    mCnmt.fromBytes(scratch.data(), scratch.size());
}

void nstool::CnmtProcess::displayCnmt()
{
    const pie::hac::sContentMetaHeader *cnmt_hdr = (const pie::hac::sContentMetaHeader *)mCnmt.getBytes().data();

    Report &r = get_report();

    r.text("[ContentMeta]");
    r.text(fmt::format("  TitleId:               0x{:016x}", mCnmt.getTitleId()));
    r.text(fmt::format(
        "  Version:               {:s} (v{:d})", pie::hac::ContentMetaUtil::getVersionAsString(mCnmt.getTitleVersion()),
        mCnmt.getTitleVersion()));
    r.text(fmt::format(
        "  Type:                  {:s} ({:d})",
        pie::hac::ContentMetaUtil::getContentMetaTypeAsString(mCnmt.getContentMetaType()),
        (uint32_t)mCnmt.getContentMetaType()));
    r.text(fmt::format("  Attributes:            0x{:x}", *((byte_t *)&cnmt_hdr->attributes)));

    r.set("data.contentMeta.titleId", fmt::format("{:016x}", mCnmt.getTitleId()));
    r.set(
        "data.contentMeta.version",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getVersionAsString(mCnmt.getTitleVersion())},
            {"int", mCnmt.getTitleVersion()}});
    r.set(
        "data.contentMeta.type",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getContentMetaTypeAsString(mCnmt.getContentMetaType())},
            {"int", mCnmt.getContentMetaType()}});
    r.set("data.contentMeta.attributes", fmt::format("0x{:x}", *((byte_t *)&cnmt_hdr->attributes)));

    if (mCnmt.getAttribute().size())
    {
        std::vector<std::string> attribute_list;

        for (auto itr = mCnmt.getAttribute().begin(); itr != mCnmt.getAttribute().end(); itr++)
        {
            attribute_list.push_back(pie::hac::ContentMetaUtil::getContentMetaAttributeFlagAsString(
                pie::hac::cnmt::ContentMetaAttributeFlag(*itr)));
        }

        std::string line = " [";

        for (auto itr = attribute_list.begin(); itr != attribute_list.end(); itr++)
        {
            line += *itr;

            if ((itr + 1) != attribute_list.end())
            {
                line += ", ";
            }
        }

        line += "]";

        r.text(line);
    }

    r.text(fmt::format(
        "  StorageId:             {:s} ({:d})", pie::hac::ContentMetaUtil::getStorageIdAsString(mCnmt.getStorageId()),
        (uint32_t)mCnmt.getStorageId()));
    r.text(fmt::format(
        "  ContentInstallType:    {:s} ({:d})",
        pie::hac::ContentMetaUtil::getContentInstallTypeAsString(mCnmt.getContentInstallType()),
        (uint32_t)mCnmt.getContentInstallType()));
    r.text(fmt::format(
        "  RequiredDownloadSystemVersion: {:s} (v{:d})",
        pie::hac::ContentMetaUtil::getVersionAsString(mCnmt.getRequiredDownloadSystemVersion()),
        mCnmt.getRequiredDownloadSystemVersion()));

    r.set(
        "data.contentMeta.storageId",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getStorageIdAsString(mCnmt.getStorageId())},
            {"int", mCnmt.getStorageId()}});
    r.set(
        "data.contentMeta.contentInstallType",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getContentInstallTypeAsString(mCnmt.getContentInstallType())},
            {"int", mCnmt.getContentInstallType()}});
    r.set(
        "data.contentMeta.requiredDownloadSystemVersion",
        nlohmann::json{
            {"string", pie::hac::ContentMetaUtil::getVersionAsString(mCnmt.getRequiredDownloadSystemVersion())},
            {"int", mCnmt.getRequiredDownloadSystemVersion()}});

    switch (mCnmt.getContentMetaType())
    {
    case (pie::hac::cnmt::ContentMetaType_Application):
        r.text("  ApplicationExtendedHeader:");
        r.text(fmt::format(
            "    RequiredApplicationVersion: {:s} (v{:d})",
            pie::hac::ContentMetaUtil::getVersionAsString(
                mCnmt.getApplicationMetaExtendedHeader().getRequiredApplicationVersion()),
            mCnmt.getApplicationMetaExtendedHeader().getRequiredApplicationVersion()));
        r.text(fmt::format(
            "    RequiredSystemVersion:      {:s} (v{:d})",
            pie::hac::ContentMetaUtil::getVersionAsString(
                mCnmt.getApplicationMetaExtendedHeader().getRequiredSystemVersion()),
            mCnmt.getApplicationMetaExtendedHeader().getRequiredSystemVersion()));
        r.text(fmt::format(
            "    PatchId:                    0x{:016x}", mCnmt.getApplicationMetaExtendedHeader().getPatchId()));

        r.set(
            "data.contentMeta.applicationExtendedHeader.requiredApplicationVersion",
            nlohmann::json{
                {"string", pie::hac::ContentMetaUtil::getVersionAsString(
                               mCnmt.getApplicationMetaExtendedHeader().getRequiredApplicationVersion())},
                {"int", mCnmt.getApplicationMetaExtendedHeader().getRequiredApplicationVersion()}});
        r.set(
            "data.contentMeta.applicationExtendedHeader.requiredSystemVersion",
            nlohmann::json{
                {"string", pie::hac::ContentMetaUtil::getVersionAsString(
                               mCnmt.getApplicationMetaExtendedHeader().getRequiredSystemVersion())},
                {"int", mCnmt.getApplicationMetaExtendedHeader().getRequiredSystemVersion()}});
        r.set(
            "data.contentMeta.applicationExtendedHeader.patchId",
            fmt::format("{:016x}", mCnmt.getApplicationMetaExtendedHeader().getPatchId()));
        break;
    case (pie::hac::cnmt::ContentMetaType_Patch):
        r.text("  PatchMetaExtendedHeader:");
        r.text(fmt::format(
            "    RequiredSystemVersion: {:s} (v{:d})",
            pie::hac::ContentMetaUtil::getVersionAsString(
                mCnmt.getPatchMetaExtendedHeader().getRequiredSystemVersion()),
            mCnmt.getPatchMetaExtendedHeader().getRequiredSystemVersion()));
        r.text(
            fmt::format("    ApplicationId:         0x{:016x}", mCnmt.getPatchMetaExtendedHeader().getApplicationId()));

        r.set(
            "data.contentMeta.patchMetaExtendedHeader.requiredSystemVersion",
            nlohmann::json{
                {"string", pie::hac::ContentMetaUtil::getVersionAsString(
                               mCnmt.getPatchMetaExtendedHeader().getRequiredSystemVersion())},
                {"int", mCnmt.getPatchMetaExtendedHeader().getRequiredSystemVersion()}});
        r.set(
            "data.contentMeta.patchMetaExtendedHeader.applicationId",
            fmt::format("{:016x}", mCnmt.getPatchMetaExtendedHeader().getApplicationId()));
        break;
    case (pie::hac::cnmt::ContentMetaType_AddOnContent):
        r.text("  AddOnContentMetaExtendedHeader:");
        r.text(fmt::format(
            "    RequiredApplicationVersion: {:s} (v{:d})",
            pie::hac::ContentMetaUtil::getVersionAsString(
                mCnmt.getAddOnContentMetaExtendedHeader().getRequiredApplicationVersion()),
            mCnmt.getAddOnContentMetaExtendedHeader().getRequiredApplicationVersion()));
        r.text(fmt::format(
            "    ApplicationId:         0x{:016x}", mCnmt.getAddOnContentMetaExtendedHeader().getApplicationId()));

        r.set(
            "data.contentMeta.addOnContentMetaExtendedHeader.requiredApplicationVersion",
            nlohmann::json{
                {"string", pie::hac::ContentMetaUtil::getVersionAsString(
                               mCnmt.getAddOnContentMetaExtendedHeader().getRequiredApplicationVersion())},
                {"int", mCnmt.getAddOnContentMetaExtendedHeader().getRequiredApplicationVersion()}});
        r.set(
            "data.contentMeta.addOnContentMetaExtendedHeader.applicationId",
            fmt::format("{:016x}", mCnmt.getAddOnContentMetaExtendedHeader().getApplicationId()));
        break;
    case (pie::hac::cnmt::ContentMetaType_Delta):
        r.text("  DeltaMetaExtendedHeader:");
        r.text(
            fmt::format("    ApplicationId:         0x{:016x}", mCnmt.getDeltaMetaExtendedHeader().getApplicationId()));

        r.set(
            "data.contentMeta.deltaMetaExtendedHeader.applicationId",
            fmt::format("{:016x}", mCnmt.getDeltaMetaExtendedHeader().getApplicationId()));
        break;
    default:
        break;
    }

    if (mCnmt.getContentInfo().size() > 0)
    {
        r.text("  ContentInfo:");

        for (size_t i = 0; i < mCnmt.getContentInfo().size(); i++)
        {
            const pie::hac::ContentInfo &info = mCnmt.getContentInfo()[i];

            r.text(fmt::format("    {:d}", i));
            r.text(fmt::format(
                "      Type:         {:s} ({:d})",
                pie::hac::ContentMetaUtil::getContentTypeAsString(info.getContentType()),
                (uint32_t)info.getContentType()));
            r.text(fmt::format(
                "      Id:           {:s}", tc::cli::FormatUtil::formatBytesAsString(
                                                info.getContentId().data(), info.getContentId().size(), false, "")));
            r.text(fmt::format("      Size:         0x{:x}", info.getContentSize()));
            r.text(fmt::format(
                "      Hash:         {:s}",
                tc::cli::FormatUtil::formatBytesAsString(
                    info.getContentHash().data(), info.getContentHash().size(), false, "")));

            r.push(
                "data.contentMeta.content",
                nlohmann::json{
                    {"type",
                     nlohmann::json{
                         {"string", pie::hac::ContentMetaUtil::getContentTypeAsString(info.getContentType())},
                         {"int", info.getContentType()}}},
                    {"id", tc::cli::FormatUtil::formatBytesAsString(
                               info.getContentId().data(), info.getContentId().size(), false, "")},
                    {"size",
                     nlohmann::json{
                         {"hex", fmt::format("0x{:x}", info.getContentSize())}, {"int", info.getContentSize()}}},
                    {"hash", tc::cli::FormatUtil::formatBytesAsString(
                                 info.getContentHash().data(), info.getContentHash().size(), false, "")}});
        }
    }

    if (mCnmt.getContentMetaInfo().size() > 0)
    {
        r.text("  ContentMetaInfo:");

        displayContentMetaInfoList(mCnmt.getContentMetaInfo(), "    ");
    }

    // print extended data
    if (mCnmt.getContentMetaType() == pie::hac::cnmt::ContentMetaType_Patch &&
        mCnmt.getPatchMetaExtendedHeader().getExtendedDataSize() != 0)
    {
        // this is stubbed as the raw output is for development purposes
        // r.text("  PatchMetaExtendedData:");
        // tc::cli::FormatUtil::formatBytesAsHxdHexString(mCnmt.getPatchMetaExtendedData().data(),
        // mCnmt.getPatchMetaExtendedData().size());
    }
    else if (
        mCnmt.getContentMetaType() == pie::hac::cnmt::ContentMetaType_Delta &&
        mCnmt.getDeltaMetaExtendedHeader().getExtendedDataSize() != 0)
    {
        // this is stubbed as the raw output is for development purposes
        // r.text("  DeltaMetaExtendedData:");
        // tc::cli::FormatUtil::formatBytesAsHxdHexString(mCnmt.getDeltaMetaExtendedData().data(),
        // mCnmt.getDeltaMetaExtendedData().size());
    }
    else if (
        mCnmt.getContentMetaType() == pie::hac::cnmt::ContentMetaType_SystemUpdate &&
        mCnmt.getSystemUpdateMetaExtendedHeader().getExtendedDataSize() != 0)
    {
        r.text("  SystemUpdateMetaExtendedData:");
        r.text(
            fmt::format("    FormatVersion:         {:d}", mCnmt.getSystemUpdateMetaExtendedData().getFormatVersion()));
        r.text("    FirmwareVariation:");

        r.set(
            "data.contentMeta.systemUpdateMetadata.formatVersion",
            mCnmt.getSystemUpdateMetaExtendedData().getFormatVersion());

        auto variation_info = mCnmt.getSystemUpdateMetaExtendedData().getFirmwareVariationInfo();

        for (size_t i = 0; i < mCnmt.getSystemUpdateMetaExtendedData().getFirmwareVariationInfo().size(); i++)
        {
            r.text(fmt::format("      {:d}", i));
            r.text(fmt::format("        FirmwareVariationId:  0x{:x}", variation_info[i].variation_id));

            if (mCnmt.getSystemUpdateMetaExtendedData().getFormatVersion() == 2)
            {
                r.text(fmt::format("        ReferToBase:          {}", variation_info[i].meta.empty()));

                if (variation_info[i].meta.empty() == false)
                {
                    r.text("        ContentMeta:");

                    displayContentMetaInfoList(variation_info[i].meta, "          ");
                }
            }
        }
    }

    r.text(fmt::format(
        "  Digest:   {:s}",
        tc::cli::FormatUtil::formatBytesAsString(mCnmt.getDigest().data(), mCnmt.getDigest().size(), false, "")));
    r.set(
        "data.contentMeta.digest",
        tc::cli::FormatUtil::formatBytesAsString(mCnmt.getDigest().data(), mCnmt.getDigest().size(), false, ""));
}

void nstool::CnmtProcess::displayContentMetaInfo(
    const pie::hac::ContentMetaInfo &content_meta_info, const std::string &prefix)
{
    const pie::hac::sContentMetaInfo *content_meta_info_raw =
        (const pie::hac::sContentMetaInfo *)content_meta_info.getBytes().data();

    fmt::print("{:s}Id:           0x{:016x}\n", prefix, content_meta_info.getTitleId());
    fmt::print(
        "{:s}Version:      {:s} (v{:d})\n", prefix,
        pie::hac::ContentMetaUtil::getVersionAsString(content_meta_info.getTitleVersion()),
        content_meta_info.getTitleVersion());
    fmt::print(
        "{:s}Type:         {:s} ({:d})\n", prefix,
        pie::hac::ContentMetaUtil::getContentMetaTypeAsString(content_meta_info.getContentMetaType()),
        (uint32_t)content_meta_info.getContentMetaType());
    fmt::print("{:s}Attributes:   0x{:x}", prefix, *((byte_t *)&content_meta_info_raw->attributes));

    if (content_meta_info.getAttribute().size())
    {
        std::vector<std::string> attribute_list;

        for (auto itr = content_meta_info.getAttribute().begin(); itr != content_meta_info.getAttribute().end(); itr++)
        {
            attribute_list.push_back(pie::hac::ContentMetaUtil::getContentMetaAttributeFlagAsString(
                pie::hac::cnmt::ContentMetaAttributeFlag(*itr)));
        }

        fmt::print(" [");

        for (auto itr = attribute_list.begin(); itr != attribute_list.end(); itr++)
        {
            fmt::print("{:s}", *itr);

            if ((itr + 1) != attribute_list.end())
            {
                fmt::print(", ");
            }
        }

        fmt::print("]");
    }

    fmt::print("\n");
}

void nstool::CnmtProcess::displayContentMetaInfoList(
    const std::vector<pie::hac::ContentMetaInfo> &content_meta_info_list, const std::string &prefix)
{
    for (size_t i = 0; i < content_meta_info_list.size(); i++)
    {
        const pie::hac::ContentMetaInfo &info = mCnmt.getContentMetaInfo()[i];

        fmt::print("{}{}\n", prefix, i);

        displayContentMetaInfo(info, prefix + "  ");
    }
}

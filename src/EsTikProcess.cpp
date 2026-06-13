#include "EsTikProcess.h"
#include "PkiValidator.h"
#include "Report.hpp"

#include <pietendo/hac/es/SignUtils.h>

nstool::EsTikProcess::EsTikProcess() : mModuleName("nstool::EsTikProcess"), mFile(), mVerify(false) {}

void nstool::EsTikProcess::process()
{
    importTicket();

    if (mVerify)
    {
        verifyTicket();
    }

    displayTicket();
}

void nstool::EsTikProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file) { mFile = file; }

void nstool::EsTikProcess::setKeyCfg(const KeyBag &keycfg) { mKeyCfg = keycfg; }

void nstool::EsTikProcess::setCertificateChain(
    const std::vector<pie::hac::es::SignedData<pie::hac::es::CertificateBody>> &certs)
{
    mCerts = certs;
}

void nstool::EsTikProcess::setVerifyMode(bool verify) { mVerify = verify; }

void nstool::EsTikProcess::importTicket()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size is greater than 20MB. Don't import large files.
    size_t file_size = tc::io::IOUtil::castInt64ToSize(mFile->length());

    if (file_size > (0x100000 * 20))
    {
        throw tc::Exception(mModuleName, "File too large.");
    }

    // read ticket
    tc::ByteData scratch = tc::ByteData(file_size);
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    mTik.fromBytes(scratch.data(), scratch.size());
}

void nstool::EsTikProcess::verifyTicket()
{
    PkiValidator pki_validator;
    tc::ByteData tik_hash;

    switch (pie::hac::es::sign::getHashAlgo(mTik.getSignature().getSignType()))
    {
    case (pie::hac::es::sign::HASH_ALGO_SHA1):
        tik_hash = tc::ByteData(tc::crypto::Sha1Generator::kHashSize);
        tc::crypto::GenerateSha1Hash(tik_hash.data(), mTik.getBody().getBytes().data(),
                                     mTik.getBody().getBytes().size());
        break;
    case (pie::hac::es::sign::HASH_ALGO_SHA256):
        tik_hash = tc::ByteData(tc::crypto::Sha2256Generator::kHashSize);
        tc::crypto::GenerateSha2256Hash(tik_hash.data(), mTik.getBody().getBytes().data(),
                                        mTik.getBody().getBytes().size());
        break;
    }

    try
    {
        pki_validator.setKeyCfg(mKeyCfg);
        pki_validator.addCertificates(mCerts);
        pki_validator.validateSignature(mTik.getBody().getIssuer(), mTik.getSignature().getSignType(),
                                        mTik.getSignature().getSignature(), tik_hash);
    }
    catch (const tc::Exception &e)
    {
        get_report().text(fmt::format("[WARNING] Ticket signature could not be validated ({:s})", e.error()));

        get_report().push(
            "events",
            nlohmann::json{{"severity", "warn"},
                           {"message", fmt::format("Ticket signature could not be validated ({:s})", e.error())}});
    }
}

void nstool::EsTikProcess::displayTicket()
{
    Report &r = get_report();

    const pie::hac::es::TicketBody_V2 &body = mTik.getBody();

    r.text("[ES Ticket]");
    r.text(fmt::format("  SignType:         {:s}", getSignTypeStr(mTik.getSignature().getSignType())));
    r.text(fmt::format("  SignType hex:     0x{:x}", (uint32_t)mTik.getSignature().getSignType()),
           Report::TextType::Extended);
    r.text(fmt::format("  Issuer:           {:s}", body.getIssuer()));
    r.text("  Title Key:");
    r.text(fmt::format("    EncMode:        {:s}", getTitleKeyPersonalisationStr(body.getTitleKeyEncType())));
    r.text(fmt::format("    KeyGeneration:  {:d}", (uint32_t)body.getCommonKeyId()));

    r.set("data.esTicket.signType", nlohmann::json{{"string", getSignTypeStr(mTik.getSignature().getSignType())},
                                                   {"int", (uint32_t)mTik.getSignature().getSignType()}});
    r.set("data.esTicket.issuer", body.getIssuer());
    r.set("data.esTicket.titleKey",
          nlohmann::json{{"encodingMode", getTitleKeyPersonalisationStr(body.getTitleKeyEncType())},
                         {"keyGeneration", (uint32_t)body.getCommonKeyId()}});

    if (body.getTitleKeyEncType() == pie::hac::es::ticket::RSA2048)
    {
        r.text("    Data:");
        r.text(fmt::format("      {:s}", tc::cli::FormatUtil::formatBytesAsStringWithLineLimit(
                                             body.getEncTitleKey(), 0x100, true, "", 0x10, 6, false)));

        r.set("data.esTicket.titleKey.data", tc::cli::FormatUtil::formatBytesAsStringWithLineLimit(
                                                 body.getEncTitleKey(), 0x100, true, "", 0x10, 6, false));
    }
    else if (body.getTitleKeyEncType() == pie::hac::es::ticket::AES128_CBC)
    {
        r.text("    Data:");
        r.text(
            fmt::format("      {:s}", tc::cli::FormatUtil::formatBytesAsString(body.getEncTitleKey(), 0x10, true, "")));

        r.set("data.esTicket.titleKey.data",
              tc::cli::FormatUtil::formatBytesAsString(body.getEncTitleKey(), 0x10, true, ""));
    }
    else
    {
        r.text("    Data:           <cannot display>");

        r.set("data.esTicket.titleKey.data", "<cannot display>");
    }

    r.text(fmt::format("  Version:          {:s} (v{:d})", getTitleVersionStr(body.getTicketVersion()),
                       body.getTicketVersion()));
    r.text(fmt::format("  License Type:     {:s}", getLicenseTypeStr(body.getLicenseType())));

    r.set("data.esTicket.version",
          nlohmann::json{{"string", getTitleVersionStr(body.getTicketVersion())}, {"int", body.getTicketVersion()}});
    r.set("data.esTicket.licenseType", getLicenseTypeStr(body.getLicenseType()));

    if (body.getPropertyFlags().size() > 0)
    {
        pie::hac::es::sTicketBody_v2 *raw_body = (pie::hac::es::sTicketBody_v2 *)body.getBytes().data();

        r.text(
            fmt::format("  PropertyMask:     0x{:04x}", ((tc::bn::le16<uint16_t> *)&raw_body->property_mask)->unwrap()),
            Report::TextType::Extended);

        r.set("data.esTicket.propertyMask",
              fmt::format("0x{:04x}", ((tc::bn::le16<uint16_t> *)&raw_body->property_mask)->unwrap()));

        for (size_t i = 0; i < body.getPropertyFlags().size(); i++)
        {
            r.text(fmt::format("    {:s}", getPropertyFlagStr(body.getPropertyFlags()[i])), Report::TextType::Extended);

            r.push("data.esTicket.propertyFlags", getPropertyFlagStr(body.getPropertyFlags()[i]));
        }
    }

    r.text("  Reserved Region:", Report::TextType::Extended);
    r.text(fmt::format("    {:s}", tc::cli::FormatUtil::formatBytesAsString(body.getReservedRegion(), 8, true, "")),
           Report::TextType::Extended);
    r.text(fmt::format("  TicketId:         0x{:016x}", body.getTicketId()), Report::TextType::Extended);
    r.text(fmt::format("  DeviceId:         0x{:016x}", body.getDeviceId()), Report::TextType::Extended);
    r.text("  RightsId:");
    r.text(fmt::format("    {:s}", tc::cli::FormatUtil::formatBytesAsString(body.getRightsId(), 16, true, "")));
    r.text(fmt::format("  SectionTotalSize:       0x{:x}", body.getSectionTotalSize()));
    r.text(fmt::format("  SectionHeaderOffset:    0x{:x}", body.getSectionHeaderOffset()));
    r.text(fmt::format("  SectionNum:             0x{:x}", body.getSectionNum()));
    r.text(fmt::format("  SectionEntrySize:       0x{:x}", body.getSectionEntrySize()));

    r.set("data.esTicket.reservedRegion",
          tc::cli::FormatUtil::formatBytesAsString(body.getReservedRegion(), 8, true, ""));
    r.set("data.esTicket.ticketId", fmt::format("0x{:016x}", body.getTicketId()));
    r.set("data.esTicket.deviceId", fmt::format("0x{:016x}", body.getDeviceId()));
    r.set("data.esTicket.rightsId", tc::cli::FormatUtil::formatBytesAsString(body.getRightsId(), 16, true, ""));
    r.set("data.esTicket.section",
          nlohmann::json{{"totalSize", fmt::format("0x{:x}", body.getSectionTotalSize())},
                         {"headerOffset", fmt::format("0x{:x}", body.getSectionHeaderOffset())},
                         {"number", fmt::format("0x{:x}", body.getSectionNum())},
                         {"entrySize", fmt::format("0x{:x}", body.getSectionEntrySize())}});
}

std::string nstool::EsTikProcess::getSignTypeStr(uint32_t type) const
{
    std::string str;

    switch (type)
    {
    case (pie::hac::es::sign::SIGN_ID_RSA4096_SHA1):
        str = "RSA4096-SHA1";
        break;
    case (pie::hac::es::sign::SIGN_ID_RSA2048_SHA1):
        str = "RSA2048-SHA1";
        break;
    case (pie::hac::es::sign::SIGN_ID_ECDSA240_SHA1):
        str = "ECDSA240-SHA1";
        break;
    case (pie::hac::es::sign::SIGN_ID_RSA4096_SHA256):
        str = "RSA4096-SHA256";
        break;
    case (pie::hac::es::sign::SIGN_ID_RSA2048_SHA256):
        str = "RSA2048-SHA256";
        break;
    case (pie::hac::es::sign::SIGN_ID_ECDSA240_SHA256):
        str = "ECDSA240-SHA256";
        break;
    default:
        str = "Unknown";
        break;
    }

    return str;
}

std::string nstool::EsTikProcess::getTitleKeyPersonalisationStr(byte_t flag) const
{
    std::string str;

    switch (flag)
    {
    case (pie::hac::es::ticket::AES128_CBC):
        str = "Generic (AESCBC)";
        break;
    case (pie::hac::es::ticket::RSA2048):
        str = "Personalised (RSA2048)";
        break;
    default:
        str = fmt::format("Unknown ({:d})", flag);
        break;
    }

    return str;
}

std::string nstool::EsTikProcess::getLicenseTypeStr(byte_t flag) const
{
    std::string str;

    switch (flag)
    {
    case (pie::hac::es::ticket::LICENSE_PERMANENT):
        str = "Permanent";
        break;
    case (pie::hac::es::ticket::LICENSE_DEMO):
        str = "Demo";
        break;
    case (pie::hac::es::ticket::LICENSE_TRIAL):
        str = "Trial";
        break;
    case (pie::hac::es::ticket::LICENSE_RENTAL):
        str = "Rental";
        break;
    case (pie::hac::es::ticket::LICENSE_SUBSCRIPTION):
        str = "Subscription";
        break;
    case (pie::hac::es::ticket::LICENSE_SERVICE):
        str = "Service";
        break;
    default:
        str = fmt::format("Unknown ({:d})", flag);
        break;
    }

    return str;
}

std::string nstool::EsTikProcess::getPropertyFlagStr(byte_t flag) const
{
    std::string str;

    switch (flag)
    {
    case (pie::hac::es::ticket::FLAG_PRE_INSTALL):
        str = "PreInstall";
        break;
    case (pie::hac::es::ticket::FLAG_SHARED_TITLE):
        str = "SharedTitle";
        break;
    case (pie::hac::es::ticket::FLAG_ALLOW_ALL_CONTENT):
        str = "AllContent";
        break;
    case (pie::hac::es::ticket::FLAG_DEVICE_LINK_INDEPENDENT):
        str = "DeviceLinkIndependent";
        break;
    case (pie::hac::es::ticket::FLAG_VOLATILE):
        str = "Volatile";
        break;
    case (pie::hac::es::ticket::FLAG_ELICENSE_REQUIRED):
        str = "ELicenseRequired";
        break;
    default:
        str = fmt::format("Unknown ({:d})", flag);
        break;
    }

    return str;
}

std::string nstool::EsTikProcess::getTitleVersionStr(uint16_t version) const
{
    return fmt::format("{:d}.{:d}.{:d}", ((version >> 10) & 0x3f), ((version >> 4) & 0x3f), ((version >> 0) & 0xf));
}

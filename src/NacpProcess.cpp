#include "NacpProcess.h"
#include "Report.hpp"
#include <pietendo/hac/ApplicationControlPropertyUtil.h>

nstool::NacpProcess::NacpProcess() : mModuleName("nstool::NacpProcess"), mFile(), mVerify(false) {}

void nstool::NacpProcess::process()
{
    importNacp();

    displayNacp();
}

void nstool::NacpProcess::setInputFile(const std::shared_ptr<tc::io::IStream> &file)
{
    mFile = file;
}

void nstool::NacpProcess::setVerifyMode(bool verify)
{
    mVerify = verify;
}

const pie::hac::ApplicationControlProperty &nstool::NacpProcess::getApplicationControlProperty() const
{
    return mNacp;
}

void nstool::NacpProcess::importNacp()
{
    if (mFile == nullptr)
    {
        throw tc::Exception(mModuleName, "No file reader set.");
    }

    if (mFile->canRead() == false || mFile->canSeek() == false)
    {
        throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
    }

    // check if file_size does matches expected size
    size_t file_size = tc::io::IOUtil::castInt64ToSize(mFile->length());

    if (file_size != sizeof(pie::hac::sApplicationControlProperty))
    {
        throw tc::Exception(mModuleName, "File was incorrect size.");
    }

    // read cnmt
    tc::ByteData scratch = tc::ByteData(file_size);
    mFile->seek(0, tc::io::SeekOrigin::Begin);
    mFile->read(scratch.data(), scratch.size());

    mNacp.fromBytes(scratch.data(), scratch.size());
}

void nstool::NacpProcess::displayNacp()
{
    Report &r = get_report();

    r.text("[ApplicationControlProperty]");

    // Title
    if (mNacp.getTitle().size() > 0)
    {
        r.text("  Title:");

        for (auto itr = mNacp.getTitle().begin(); itr != mNacp.getTitle().end(); itr++)
        {
            r.text(
                fmt::format("    {:s}:", pie::hac::ApplicationControlPropertyUtil::getLanguageAsString(itr->language)));
            r.text(fmt::format("      Name:       {:s}", itr->name));
            r.text(fmt::format("      Publisher:  {:s}", itr->publisher));

            r.push(
                "data.applicationControlProperty.title",
                nlohmann::json{
                    {"language",
                     nlohmann::json{
                         {"string", pie::hac::ApplicationControlPropertyUtil::getLanguageAsString(itr->language)},
                         {"int", itr->language}}},
                    {"name", itr->name},
                    {"publisher", itr->publisher}});
        }
    }
    else
    {
        r.text("  Title:                                  None", Report::TextType::Extended);

        r.set("data.applicationControlProperty.title", nlohmann::json::array());
    }

    // Isbn
    if (mNacp.getIsbn().empty() == false)
    {
        r.text(fmt::format("  ISBN:                                   {:s}", mNacp.getIsbn()));

        r.set("data.applicationControlProperty.isbn", mNacp.getIsbn());
    }
    else
    {
        r.text("  ISBN:                                   (NotSet)", Report::TextType::Extended);

        r.set("data.applicationControlProperty.isbn", nullptr);
    }

    // StartupUserAccount
    r.text(
        fmt::format(
            "  StartupUserAccount:                     {:s}",
            pie::hac::ApplicationControlPropertyUtil::getStartupUserAccountAsString(mNacp.getStartupUserAccount())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.startupUserAccount",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getStartupUserAccountAsString(mNacp.getStartupUserAccount())},
            {"int", mNacp.getStartupUserAccount()}});

    // UserAccountSwitchLock
    r.text(
        fmt::format(
            "  UserAccountSwitchLock:                  {:s}",
            pie::hac::ApplicationControlPropertyUtil::getUserAccountSwitchLockAsString(
                mNacp.getUserAccountSwitchLock())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.useAccountSwitchLock",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getUserAccountSwitchLockAsString(
                           mNacp.getUserAccountSwitchLock())},
            {"int", mNacp.getUserAccountSwitchLock()}});

    // AddOnContentRegistrationType
    r.text(
        fmt::format(
            "  AddOnContentRegistrationType:           {:s}",
            pie::hac::ApplicationControlPropertyUtil::getAddOnContentRegistrationTypeAsString(
                mNacp.getAddOnContentRegistrationType())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.addOnContentRegistrationType",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getAddOnContentRegistrationTypeAsString(
                           mNacp.getAddOnContentRegistrationType())},
            {"int", mNacp.getAddOnContentRegistrationType()}});

    // Attribute
    if (mNacp.getAttribute().size() > 0)
    {
        r.text("  Attribute:");

        for (auto itr = mNacp.getAttribute().begin(); itr != mNacp.getAttribute().end(); itr++)
        {
            r.text(fmt::format("    {:s}", pie::hac::ApplicationControlPropertyUtil::getAttributeFlagAsString(*itr)));

            r.push(
                "data.applicationControlProperty.attributes",
                nlohmann::json{
                    {"string", pie::hac::ApplicationControlPropertyUtil::getAttributeFlagAsString(*itr)},
                    {"int", *itr}});
        }
    }
    else
    {
        r.text("  Attribute:                              None", Report::TextType::Extended);

        r.set("data.applicationControlProperty.attributes", nlohmann::json::array());
    }

    // SupportedLanguage
    if (mNacp.getSupportedLanguage().size() > 0)
    {
        r.text("  SupportedLanguage:");

        for (auto itr = mNacp.getSupportedLanguage().begin(); itr != mNacp.getSupportedLanguage().end(); itr++)
        {
            r.text(fmt::format("    {:s}", pie::hac::ApplicationControlPropertyUtil::getLanguageAsString(*itr)));

            r.push(
                "data.applicationControlProperty.supportedLanguages",
                nlohmann::json{
                    {"string", pie::hac::ApplicationControlPropertyUtil::getLanguageAsString(*itr)}, {"int", *itr}});
        }
    }
    else
    {
        r.text("  SupportedLanguage:                      None", Report::TextType::Extended);

        r.set("data.applicationControlProperty.supportedLanguages", nlohmann::json::array());
    }

    // ParentalControl
    if (mNacp.getParentalControl().size() > 0)
    {
        r.text("  ParentalControl:");

        for (auto itr = mNacp.getParentalControl().begin(); itr != mNacp.getParentalControl().end(); itr++)
        {
            r.text(fmt::format(
                "    {:s}", pie::hac::ApplicationControlPropertyUtil::getParentalControlFlagAsString(*itr)));

            r.push(
                "data.applicationControlProperty.parentalControlFlags",
                nlohmann::json{
                    {"string", pie::hac::ApplicationControlPropertyUtil::getParentalControlFlagAsString(*itr)},
                    {"int", *itr}});
        }
    }
    else
    {
        r.text("  ParentalControl:                        None", Report::TextType::Extended);

        r.set("data.applicationControlProperty.parentalControlFlags", nlohmann::json::array());
    }

    // Screenshot
    r.text(
        fmt::format(
            "  Screenshot:                             {:s}",
            pie::hac::ApplicationControlPropertyUtil::getScreenshotAsString(mNacp.getScreenshot())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.screenshot",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getScreenshotAsString(mNacp.getScreenshot())},
            {"int", mNacp.getScreenshot()}});

    // VideoCapture
    r.text(
        fmt::format(
            "  VideoCapture:                           {:s}",
            pie::hac::ApplicationControlPropertyUtil::getVideoCaptureAsString(mNacp.getVideoCapture())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.videoCapture",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getVideoCaptureAsString(mNacp.getVideoCapture())},
            {"int", mNacp.getVideoCapture()}});

    // DataLossConfirmation
    r.text(
        fmt::format(
            "  DataLossConfirmation:                   {:s}",
            pie::hac::ApplicationControlPropertyUtil::getDataLossConfirmationAsString(mNacp.getDataLossConfirmation())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.dataLossConfirmation",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getDataLossConfirmationAsString(
                           mNacp.getDataLossConfirmation())},
            {"int", mNacp.getDataLossConfirmation()}});

    // PlayLogPolicy
    r.text(
        fmt::format(
            "  PlayLogPolicy:                          {:s}",
            pie::hac::ApplicationControlPropertyUtil::getPlayLogPolicyAsString(mNacp.getPlayLogPolicy())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.playLogPolicy",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getPlayLogPolicyAsString(mNacp.getPlayLogPolicy())},
            {"int", mNacp.getPlayLogPolicy()}});

    // PresenceGroupId
    r.text(
        fmt::format("  PresenceGroupId:                        0x{:016x}", mNacp.getPresenceGroupId()),
        Report::TextType::Extended);
    r.set("data.applicationControlProperty.presenceGroupId", fmt::format("0x{:016x}", mNacp.getPresenceGroupId()));

    // RatingAge
    if (mNacp.getRatingAge().size() > 0)
    {
        r.text("  RatingAge:");

        for (auto itr = mNacp.getRatingAge().begin(); itr != mNacp.getRatingAge().end(); itr++)
        {
            r.text(fmt::format(
                "    {:s}:", pie::hac::ApplicationControlPropertyUtil::getOrganisationAsString(itr->organisation)));
            r.text(fmt::format("      Age: {:d}", itr->age));

            r.push(
                "data.applicationControlProperty.ratingAge",
                nlohmann::json{
                    {"organization",
                     nlohmann::json{
                         {"string",
                          pie::hac::ApplicationControlPropertyUtil::getOrganisationAsString(itr->organisation)},
                         {"int", itr->organisation}}},
                    {"age", itr->age}});
        }
    }
    else
    {
        r.text("  RatingAge:                              None", Report::TextType::Extended);

        r.set("data.applicationControlProperty.ratingAge", nlohmann::json::array());
    }

    // DisplayVersion
    if (mNacp.getDisplayVersion().empty() == false)
    {
        r.text(fmt::format("  DisplayVersion:                         {:s}", mNacp.getDisplayVersion()));

        r.set("data.applicationControlProperty.displayVersion", mNacp.getDisplayVersion());
    }
    else
    {
        r.text("  DisplayVersion:                         (NotSet)", Report::TextType::Extended);

        r.set("data.applicationControlProperty.displayVersion", nullptr);
    }

    // AddOnContentBaseId
    r.text(
        fmt::format("  AddOnContentBaseId:                     0x{:016x}", mNacp.getAddOnContentBaseId()),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.addOnContentBaseId", fmt::format("0x{:016x}", mNacp.getAddOnContentBaseId()));

    // SaveDataOwnerId
    r.text(
        fmt::format("  SaveDataOwnerId:                        0x{:016x}", mNacp.getSaveDataOwnerId()),
        Report::TextType::Extended);
    r.set("data.applicationControlProperty.saveDataOwnerId", fmt::format("0x{:016x}", mNacp.getSaveDataOwnerId()));

    // UserAccountSaveDataSize
    r.text(
        fmt::format(
            "  UserAccountSaveDataSize:                {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getUserAccountSaveDataSize().size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.userAccountSaveDataSize",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getUserAccountSaveDataSize().size)},
            {"int", mNacp.getUserAccountSaveDataSize().size}});

    // UserAccountSaveDataJournalSize
    r.text(
        fmt::format(
            "  UserAccountSaveDataJournalSize:         {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                mNacp.getUserAccountSaveDataSize().journal_size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.userAccountSaveDataJournalSize",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getUserAccountSaveDataSize().journal_size)},
            {"int", mNacp.getUserAccountSaveDataSize().journal_size}});

    // DeviceSaveDataSize
    r.text(
        fmt::format(
            "  DeviceSaveDataSize:                     {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getDeviceSaveDataSize().size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.deviceSaveDataSize",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getDeviceSaveDataSize().size)},
            {"int", mNacp.getDeviceSaveDataSize().size}});

    // DeviceSaveDataJournalSize
    r.text(
        fmt::format(
            "  DeviceSaveDataJournalSize:              {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                mNacp.getDeviceSaveDataSize().journal_size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.deviceSaveDataJournalSize",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getDeviceSaveDataSize().journal_size)},
            {"int", mNacp.getDeviceSaveDataSize().journal_size}});

    // BcatDeliveryCacheStorageSize
    r.text(
        fmt::format(
            "  BcatDeliveryCacheStorageSize:           {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getBcatDeliveryCacheStorageSize())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.bcatDeliveryCacheStorageSize",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getBcatDeliveryCacheStorageSize())},
            {"int", mNacp.getBcatDeliveryCacheStorageSize()}});

    // ApplicationErrorCodeCategory
    if (mNacp.getApplicationErrorCodeCategory().empty() == false)
    {
        r.text(fmt::format("  ApplicationErrorCodeCategory:           {:s}", mNacp.getApplicationErrorCodeCategory()));
        r.set("data.applicationControlProperty.applicationErrorCodeCategory", mNacp.getApplicationErrorCodeCategory());
    }
    else
    {
        r.text(fmt::format("  ApplicationErrorCodeCategory:           (NotSet)"), Report::TextType::Extended);
        r.set("data.applicationControlProperty.applicationErrorCodeCategory", nullptr);
    }

    // LocalCommunicationId
    if (mNacp.getLocalCommunicationId().size() > 0)
    {
        r.text("  LocalCommunicationId:");

        for (auto itr = mNacp.getLocalCommunicationId().begin(); itr != mNacp.getLocalCommunicationId().end(); itr++)
        {
            r.text(fmt::format("    0x{:016x}", *itr));
            r.push("data.applicationControlProperty.localCommunicationId", fmt::format("0x{:016x}", *itr));
        }
    }
    else
    {
        r.text("  LocalCommunicationId:                   None", Report::TextType::Extended);
        r.push("data.applicationControlProperty.localCommunicationId", nlohmann::json::array());
    }

    // LogoType
    r.text(fmt::format(
        "  LogoType:                               {:s}",
        pie::hac::ApplicationControlPropertyUtil::getLogoTypeAsString(mNacp.getLogoType())));
    r.set(
        "data.applicationControlProperty.logoType",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getLogoTypeAsString(mNacp.getLogoType())},
            {"int", mNacp.getLogoType()}});

    // LogoHandling
    r.text(
        fmt::format(
            "  LogoHandling:                           {:s}",
            pie::hac::ApplicationControlPropertyUtil::getLogoHandlingAsString(mNacp.getLogoHandling())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.logoHandling",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getLogoHandlingAsString(mNacp.getLogoHandling())},
            {"int", mNacp.getLogoHandling()}});

    // RuntimeAddOnContentInstall
    r.text(
        fmt::format(
            "  RuntimeAddOnContentInstall:             {:s}",
            pie::hac::ApplicationControlPropertyUtil::getRuntimeAddOnContentInstallAsString(
                mNacp.getRuntimeAddOnContentInstall())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.runtimeAddOnContentInstall",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getRuntimeAddOnContentInstallAsString(
                           mNacp.getRuntimeAddOnContentInstall())},
            {"int", mNacp.getRuntimeAddOnContentInstall()}});

    // RuntimeParameterDelivery
    r.text(
        fmt::format(
            "  RuntimeParameterDelivery:               {:s}",
            pie::hac::ApplicationControlPropertyUtil::getRuntimeParameterDeliveryAsString(
                mNacp.getRuntimeParameterDelivery())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.runtimeParameterDelivery",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getRuntimeParameterDeliveryAsString(
                           mNacp.getRuntimeParameterDelivery())},
            {"int", mNacp.getRuntimeParameterDelivery()}});

    // CrashReport
    r.text(
        fmt::format(
            "  CrashReport:                            {:s}",
            pie::hac::ApplicationControlPropertyUtil::getCrashReportAsString(mNacp.getCrashReport())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.crashReport",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getCrashReportAsString(mNacp.getCrashReport())},
            {"int", mNacp.getCrashReport()}});

    // Hdcp
    r.text(
        fmt::format(
            "  Hdcp:                                   {:s}",
            pie::hac::ApplicationControlPropertyUtil::getHdcpAsString(mNacp.getHdcp())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.hdcp",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getHdcpAsString(mNacp.getHdcp())},
            {"int", mNacp.getHdcp()}});

    // SeedForPsuedoDeviceId
    r.text(
        fmt::format("  SeedForPsuedoDeviceId:                  0x{:016x}", mNacp.getSeedForPsuedoDeviceId()),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.seedForPsuedoDeviceId",
        fmt::format("0x{:016x}", mNacp.getSeedForPsuedoDeviceId()));

    // BcatPassphase
    if (mNacp.getBcatPassphase().empty() == false)
    {
        r.text(fmt::format("  BcatPassphase:                          {:s}", mNacp.getBcatPassphase()));
        r.set("data.applicationControlProperty.bcatPassphrase", mNacp.getBcatPassphase());
    }
    else
    {
        r.text("  BcatPassphase:                          (NotSet)", Report::TextType::Extended);
        r.set("data.applicationControlProperty.bcatPassphrase", nullptr);
    }

    // StartupUserAccountOption
    if (mNacp.getStartupUserAccountOption().size() > 0)
    {
        r.text("  StartupUserAccountOption:");

        for (auto itr = mNacp.getStartupUserAccountOption().begin(); itr != mNacp.getStartupUserAccountOption().end();
             itr++)
        {
            r.text(fmt::format(
                "    {:s}", pie::hac::ApplicationControlPropertyUtil::getStartupUserAccountOptionFlagAsString(*itr)));
            r.push(
                "data.applicationControlProperty.startupUserAccountOption",
                nlohmann::json{
                    {"string", pie::hac::ApplicationControlPropertyUtil::getStartupUserAccountOptionFlagAsString(*itr)},
                    {"int", *itr}});
        }
    }
    else
    {
        r.text("  StartupUserAccountOption:               None", Report::TextType::Extended);
        r.set("data.applicationControlProperty.startupUserAccountOption", nlohmann::json::array());
    }

    // UserAccountSaveDataSizeMax
    r.text(
        fmt::format(
            "  UserAccountSaveDataSizeMax:             {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getUserAccountSaveDataMax().size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.userAccountSaveDataSizeMax",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getUserAccountSaveDataMax().size)},
            {"int", mNacp.getUserAccountSaveDataMax().size}});

    // UserAccountSaveDataJournalSizeMax
    r.text(
        fmt::format(
            "  UserAccountSaveDataJournalSizeMax:      {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                mNacp.getUserAccountSaveDataMax().journal_size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.userAccountSaveDataJournalSizeMax",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getUserAccountSaveDataMax().journal_size)},
            {"int", mNacp.getUserAccountSaveDataMax().journal_size}});

    // DeviceSaveDataSizeMax
    r.text(
        fmt::format(
            "  DeviceSaveDataSizeMax:                  {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getDeviceSaveDataMax().size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.deviceSaveDataSizeMax",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getDeviceSaveDataMax().size)},
            {"int", mNacp.getDeviceSaveDataMax().size}});

    // DeviceSaveDataJournalSizeMax
    r.text(
        fmt::format(
            "  DeviceSaveDataJournalSizeMax:           {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                mNacp.getDeviceSaveDataMax().journal_size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.deviceSaveDataJournalSizeMax",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getDeviceSaveDataMax().journal_size)},
            {"int", mNacp.getDeviceSaveDataMax().journal_size}});

    // TemporaryStorageSize
    r.text(
        fmt::format(
            "  TemporaryStorageSize:                   {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getTemporaryStorageSize())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.temporaryStorageSize",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getTemporaryStorageSize())},
            {"int", mNacp.getTemporaryStorageSize()}});

    // CacheStorageSize
    r.text(
        fmt::format(
            "  CacheStorageSize:                       {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getCacheStorageSize().size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.cacheStorageSize",
        nlohmann::json{
            {"string",
             pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(mNacp.getCacheStorageSize().size)},
            {"int", mNacp.getCacheStorageSize().size}});

    // CacheStorageJournalSize
    r.text(
        fmt::format(
            "  CacheStorageJournalSize:                {:s}",
            pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                mNacp.getCacheStorageSize().journal_size)),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.cacheStorageJournalSize",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getCacheStorageSize().journal_size)},
            {"int", mNacp.getCacheStorageSize().journal_size}});

    // CacheStorageDataAndJournalSizeMax
    r.text(fmt::format(
        "  CacheStorageDataAndJournalSizeMax:      {:s}",
        pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
            mNacp.getCacheStorageDataAndJournalSizeMax())));
    r.set(
        "data.applicationControlProperty.cacheStorageDataAndJournalSizeMax",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getSaveDataSizeAsString(
                           mNacp.getCacheStorageDataAndJournalSizeMax())},
            {"int", mNacp.getCacheStorageDataAndJournalSizeMax()}});

    // CacheStorageIndexMax
    r.text(
        fmt::format("  CacheStorageIndexMax:                   0x{:04x}", mNacp.getCacheStorageIndexMax()),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.cacheStorageIndexMax",
        fmt::format("0x{:04x}", mNacp.getCacheStorageIndexMax()));

    // PlayLogQueryableApplicationId
    if (mNacp.getPlayLogQueryableApplicationId().size() > 0)
    {
        r.text("  PlayLogQueryableApplicationId:");

        for (auto itr = mNacp.getPlayLogQueryableApplicationId().begin();
             itr != mNacp.getPlayLogQueryableApplicationId().end(); itr++)
        {
            r.text(fmt::format("    0x{:016x}", *itr));
            r.push("data.applicationControlProperty.playLogQueryableApplicationId", fmt::format("0x{:016x}", *itr));
        }
    }
    else
    {
        r.text("  PlayLogQueryableApplicationId:          None", Report::TextType::Extended);
        r.set("data.applicationControlProperty.playLogQueryableApplicationId", nlohmann::json::array());
    }

    // PlayLogQueryCapability
    r.text(
        fmt::format(
            "  PlayLogQueryCapability:                 {:s}",
            pie::hac::ApplicationControlPropertyUtil::getPlayLogQueryCapabilityAsString(
                mNacp.getPlayLogQueryCapability())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.playLogQueryCapability",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getPlayLogQueryCapabilityAsString(
                           mNacp.getPlayLogQueryCapability())},
            {"int", mNacp.getPlayLogQueryCapability()}});

    // Repair
    if (mNacp.getRepair().size() > 0)
    {
        r.text("  Repair:");

        for (auto itr = mNacp.getRepair().begin(); itr != mNacp.getRepair().end(); itr++)
        {
            r.text(fmt::format("    {:s}", pie::hac::ApplicationControlPropertyUtil::getRepairFlagAsString(*itr)));
            r.push(
                "data.applicationControlProperty.repairFlags",
                nlohmann::json{
                    {"string", pie::hac::ApplicationControlPropertyUtil::getRepairFlagAsString(*itr)}, {"int", *itr}});
        }
    }
    else
    {
        r.text("  Repair:                                 None", Report::TextType::Extended);
        r.set("data.applicationControlProperty.repairFlags", nlohmann::json::array());
    }

    // ProgramIndex
    r.text(
        fmt::format("  ProgramIndex:                           0x{:02x}", mNacp.getProgramIndex()),
        Report::TextType::Extended);
    r.set("data.applicationControlProperty.programIndex", fmt::format("0x{:02x}", mNacp.getProgramIndex()));

    // RequiredNetworkServiceLicenseOnLaunch
    if (mNacp.getRequiredNetworkServiceLicenseOnLaunch().size() > 0)
    {
        r.text("  RequiredNetworkServiceLicenseOnLaunch:");

        for (auto itr = mNacp.getRequiredNetworkServiceLicenseOnLaunch().begin();
             itr != mNacp.getRequiredNetworkServiceLicenseOnLaunch().end(); itr++)
        {
            r.text(fmt::format(
                "    {:s}",
                pie::hac::ApplicationControlPropertyUtil::getRequiredNetworkServiceLicenseOnLaunchFlagAsString(*itr)));
            r.push(
                "data.applicationControlProperty.requiredNetworkServiceLicenseOnLaunch",
                nlohmann::json{
                    {"string",
                     pie::hac::ApplicationControlPropertyUtil::getRequiredNetworkServiceLicenseOnLaunchFlagAsString(
                         *itr)},
                    {"int", *itr}});
        }
    }
    else
    {
        r.text("  RequiredNetworkServiceLicenseOnLaunch:  None", Report::TextType::Extended);
        r.set("data.applicationControlProperty.requiredNetworkServiceLicenseOnLaunch", nlohmann::json::array());
    }

    // NeighborDetectionClientConfiguration
    const auto &detect_config = mNacp.getNeighborDetectionClientConfiguration();

    if (detect_config.countSendGroupConfig() > 0 || detect_config.countReceivableGroupConfig() > 0)
    {
        r.text("  NeighborDetectionClientConfiguration:");

        if (detect_config.countSendGroupConfig() > 0)
        {
            r.text("    SendGroupConfig:");
            r.text(fmt::format("      GroupId:  0x{:016x}", detect_config.send_data_configuration.group_id));
            r.text(fmt::format(
                "        Key:    {:s}", tc::cli::FormatUtil::formatBytesAsString(
                                            detect_config.send_data_configuration.key.data(),
                                            detect_config.send_data_configuration.key.size(), false, "")));

            r.set(
                "data.applicationControlProperty.neighborDetectionClientConfiguration.sendGroupConfig",
                nlohmann::json{
                    {"groupId", fmt::format("0x{:016x}", detect_config.send_data_configuration.group_id)},
                    {"key", tc::cli::FormatUtil::formatBytesAsString(
                                detect_config.send_data_configuration.key.data(),
                                detect_config.send_data_configuration.key.size(), false, "")}});
        }
        else
        {
            r.text("    SendGroupConfig: None", Report::TextType::Extended);
            r.set(
                "data.applicationControlProperty.neighborDetectionClientConfiguration.sendGroupConfig",
                nlohmann::json{{"groupId", nullptr}, {"key", nullptr}});
        }

        if (detect_config.countReceivableGroupConfig() > 0)
        {
            r.text("    ReceivableGroupConfig:");

            for (size_t i = 0; i < pie::hac::nacp::kReceivableGroupConfigurationCount; i++)
            {
                if (detect_config.receivable_data_configuration[i].isNull())
                {
                    continue;
                }

                r.text(
                    fmt::format("      GroupId:  0x{:016x}", detect_config.receivable_data_configuration[i].group_id));
                r.text(fmt::format(
                    "        Key:    {:s}", tc::cli::FormatUtil::formatBytesAsString(
                                                detect_config.receivable_data_configuration[i].key.data(),
                                                detect_config.receivable_data_configuration[i].key.size(), false, "")));

                r.push(
                    "data.applicationControlProperty.neighborDetectionClientConfiguration.receivableGroupConfig",
                    nlohmann::json{
                        {"groupId", fmt::format("0x{:016x}", detect_config.receivable_data_configuration[i].group_id)},
                        {"key", tc::cli::FormatUtil::formatBytesAsString(
                                    detect_config.receivable_data_configuration[i].key.data(),
                                    detect_config.receivable_data_configuration[i].key.size(), false, "")}});
            }
        }
        else
        {
            r.text("    ReceivableGroupConfig: None", Report::TextType::Extended);
            r.set(
                "data.applicationControlProperty.neighborDetectionClientConfiguration.receivableGroupConfig",
                nlohmann::json::array());
        }
    }
    else
    {
        r.text("  NeighborDetectionClientConfiguration:   None", Report::TextType::Extended);
        r.set(
            "data.applicationControlProperty.neighborDetectionClientConfiguration",
            nlohmann::json{
                {"sendGroupConfig", nlohmann::json{{"groupId", nullptr}, {"key", nullptr}}},
                {"receivableGroupConfig", nlohmann::json::array()}});
    }

    // JitConfiguration
    r.text("  JitConfiguration:", Report::TextType::Extended);
    r.text(fmt::format("    IsEnabled:  {}", mNacp.getJitConfiguration().is_enabled), Report::TextType::Extended);
    r.text(
        fmt::format("    MemorySize: 0x{:016x}", mNacp.getJitConfiguration().memory_size), Report::TextType::Extended);
    r.set("data.applicationControlProperty.jitConfiguration.isEnabled", mNacp.getJitConfiguration().is_enabled);
    r.set(
        "data.applicationControlProperty.jitConfiguration.memorySize",
        fmt::format("0x{:016x}", mNacp.getJitConfiguration().memory_size));

    // PlayReportPermission
    r.text(
        fmt::format(
            "  PlayReportPermission:                   {:s}",
            pie::hac::ApplicationControlPropertyUtil::getPlayReportPermissionAsString(mNacp.getPlayReportPermission())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.playReportPermission",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getPlayReportPermissionAsString(
                           mNacp.getPlayReportPermission())},
            {"int", mNacp.getPlayReportPermission()}});

    // CrashScreenshotForProd
    r.text(
        fmt::format(
            "  CrashScreenshotForProd:                 {:s}",
            pie::hac::ApplicationControlPropertyUtil::getCrashScreenshotForProdAsString(
                mNacp.getCrashScreenshotForProd())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.crashScreenshotForProduction",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getCrashScreenshotForProdAsString(
                           mNacp.getCrashScreenshotForProd())},
            {"int", mNacp.getCrashScreenshotForProd()}});

    // CrashScreenshotForDev
    r.text(
        fmt::format(
            "  CrashScreenshotForDev:                  {:s}",
            pie::hac::ApplicationControlPropertyUtil::getCrashScreenshotForDevAsString(
                mNacp.getCrashScreenshotForDev())),
        Report::TextType::Extended);
    r.set(
        "data.applicationControlProperty.crashScreenshotForDevelopment",
        nlohmann::json{
            {"string", pie::hac::ApplicationControlPropertyUtil::getCrashScreenshotForDevAsString(
                           mNacp.getCrashScreenshotForDev())},
            {"int", mNacp.getCrashScreenshotForDev()}});

    // AccessibleLaunchRequiredVersion
    if (mNacp.getAccessibleLaunchRequiredVersionApplicationId().size() > 0)
    {
        r.text("  AccessibleLaunchRequiredVersion:");
        r.text("    ApplicationId:");

        for (auto itr = mNacp.getAccessibleLaunchRequiredVersionApplicationId().begin();
             itr != mNacp.getAccessibleLaunchRequiredVersionApplicationId().end(); itr++)
        {
            r.text(fmt::format("      0x{:016x}", *itr));
            r.push(
                "data.applicationControlProperty.accessibleLaunchRequiredVersion.applicationId",
                fmt::format("0x{:016x}", *itr));
        }
    }
    else
    {
        r.text("  AccessibleLaunchRequiredVersion:        None", Report::TextType::Extended);
        r.set("data.applicationControlProperty.accessibleLaunchRequiredVersion.applicationId", nlohmann::json::array());
    }
}

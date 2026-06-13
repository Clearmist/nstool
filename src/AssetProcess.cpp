#include "AssetProcess.h"
#include "Report.hpp"

#include "util.h"

nstool::AssetProcess::AssetProcess() :
	mModuleName("nstool::AssetProcess"),
	mFile(),
	mVerify(false)
{
}

void nstool::AssetProcess::process()
{
	importHeader();
	displayHeader();
	processSections();
}

void nstool::AssetProcess::setInputFile(const std::shared_ptr<tc::io::IStream>& file)
{
	mFile = file;
}

void nstool::AssetProcess::setVerifyMode(bool verify)
{
	mVerify = verify;
}

void nstool::AssetProcess::setIconExtractPath(const tc::io::Path& path)
{
	mIconExtractPath = path;
}

void nstool::AssetProcess::setNacpExtractPath(const tc::io::Path& path)
{
	mNacpExtractPath = path;
}

void nstool::AssetProcess::setRomfsShowFsTree(bool show_fs_tree)
{
	mRomfs.setShowFsTree(show_fs_tree);
}

void nstool::AssetProcess::setRomfsExtractJobs(const std::vector<nstool::ExtractJob>& extract_jobs)
{
	mRomfs.setExtractJobs(extract_jobs);
}

void nstool::AssetProcess::importHeader()
{
	if (mFile == nullptr)
	{
		throw tc::Exception(mModuleName, "No file reader set.");
	}

	if (mFile->canRead() == false || mFile->canSeek() == false)
	{
		throw tc::NotSupportedException(mModuleName, "Input stream requires read/seek permissions.");
	}

	if (mFile->length() < tc::io::IOUtil::castSizeToInt64(sizeof(pie::hac::sAssetHeader)))
	{
		throw tc::Exception(mModuleName, "Corrupt ASET: file too small");
	}

	tc::ByteData scratch = tc::ByteData(sizeof(pie::hac::sAssetHeader));
	mFile->seek(0, tc::io::SeekOrigin::Begin);
	mFile->read(scratch.data(), scratch.size());

	mHdr.fromBytes(scratch.data(), scratch.size());
}

void nstool::AssetProcess::processSections()
{
	int64_t file_size = mFile->length();

	Report& r = get_report();

	if (mHdr.getIconInfo().size > 0 && mIconExtractPath.isSet())
	{
		if ((mHdr.getIconInfo().size + mHdr.getIconInfo().offset) > file_size) {
			throw tc::Exception(mModuleName, "ASET geometry for icon beyond file size");
		}

		r.push("events", fmt::format("Saving {:s}", mIconExtractPath.get().to_string()));
		r.text(fmt::format("Saving {:s}...", mIconExtractPath.get().to_string()));

		writeSubStreamToFile(mFile, mHdr.getIconInfo().offset, mHdr.getIconInfo().size, mIconExtractPath.get());
	}

	if (mHdr.getNacpInfo().size > 0)
	{
		if ((mHdr.getNacpInfo().size + mHdr.getNacpInfo().offset) > file_size) {
			throw tc::Exception(mModuleName, "ASET geometry for nacp beyond file size");
		}

		if (mNacpExtractPath.isSet())
		{
			r.push("events", fmt::format("Saving {:s}", mNacpExtractPath.get().to_string()));
			r.text(fmt::format("Saving {:s}...", mNacpExtractPath.get().to_string()));

			writeSubStreamToFile(mFile, mHdr.getNacpInfo().offset, mHdr.getNacpInfo().size, mNacpExtractPath.get());
		}

		mNacp.setInputFile(std::make_shared<tc::io::SubStream>(mFile, mHdr.getNacpInfo().offset, mHdr.getNacpInfo().size));
		mNacp.setVerifyMode(mVerify);

		mNacp.process();
	}

	if (mHdr.getRomfsInfo().size > 0)
	{
		if ((mHdr.getRomfsInfo().size + mHdr.getRomfsInfo().offset) > file_size) {
			throw tc::Exception(mModuleName, "ASET geometry for romfs beyond file size");
		}

		mRomfs.setInputFile(std::make_shared<tc::io::SubStream>(mFile, mHdr.getRomfsInfo().offset, mHdr.getRomfsInfo().size));
		mRomfs.setVerifyMode(mVerify);

		mRomfs.process();
	}
}

void nstool::AssetProcess::displayHeader()
{
	Report& r = get_report();

	r.text("[ASET Header]", Report::TextType::Layout);
	r.text("  Icon:", Report::TextType::Layout);
	r.text(fmt::format("    Offset:       0x{:x}", mHdr.getIconInfo().offset), Report::TextType::Layout);
	r.text(fmt::format("    Size:         0x{:x}", mHdr.getIconInfo().size), Report::TextType::Layout);
	r.text("  NACP:", Report::TextType::Layout);
	r.text(fmt::format("    Offset:       0x{:x}", mHdr.getNacpInfo().offset), Report::TextType::Layout);
	r.text(fmt::format("    Size:         0x{:x}", mHdr.getNacpInfo().size), Report::TextType::Layout);
	r.text("  RomFs:", Report::TextType::Layout);
	r.text(fmt::format("    Offset:       0x{:x}", mHdr.getRomfsInfo().offset), Report::TextType::Layout);
	r.text(fmt::format("    Size:         0x{:x}", mHdr.getRomfsInfo().size), Report::TextType::Layout);

	r.set("data.asetHeader.icon.offset", mHdr.getIconInfo().offset);
	r.set("data.asetHeader.icon.size", mHdr.getIconInfo().size);
	r.set("data.asetHeader.nacp.offset", mHdr.getNacpInfo().offset);
	r.set("data.asetHeader.nacp.size", mHdr.getNacpInfo().size);
	r.set("data.asetHeader.romfs.offset", mHdr.getRomfsInfo().offset);
	r.set("data.asetHeader.romfs.size", mHdr.getRomfsInfo().size);
}

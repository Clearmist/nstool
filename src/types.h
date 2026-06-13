#pragma once
#include <tc/types.h>
#include <tc/Exception.h>
#include <tc/Optional.h>
#include <tc/io.h>
#include <tc/io/IOUtil.h>
#include <tc/cli.h>
#include <fmt/core.h>

namespace nstool {

struct ExtractJob {
	tc::io::Path virtual_path;
	tc::io::Path extract_path;
};

}
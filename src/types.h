#pragma once
#include <fmt/core.h>
#include <tc/Exception.h>
#include <tc/Optional.h>
#include <tc/cli.h>
#include <tc/io.h>
#include <tc/io/IOUtil.h>
#include <tc/types.h>

namespace nstool
{

struct CliOutputMode
{
    bool show_basic_info;
    bool show_extended_info;
    bool show_layout;
    bool show_keydata;
    bool show_verbose;
    bool show_json;

    CliOutputMode()
        : show_basic_info(true), show_extended_info(false), show_layout(false), show_keydata(false),
          show_verbose(false), show_json(false)
    {
    }
};

struct ExtractJob
{
    tc::io::Path virtual_path;
    tc::io::Path extract_path;
};

} // namespace nstool

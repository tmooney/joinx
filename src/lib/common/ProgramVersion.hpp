#pragma once

#include "version.h"

#include <sstream>
#include <string>

namespace {
    std::string makeProgramVersionInfo(const std::string& progname) {
        using namespace std;
        ostringstream ss;
        ss << progname << " version " << __g_prog_version;
        if (!string(__g_commit_hash).empty())
            ss << " (commit " << __g_commit_hash << ")";

        if (!string(__g_build_type).empty())
            ss << " (" << __g_build_type << ")";
        return ss.str();
    }
}

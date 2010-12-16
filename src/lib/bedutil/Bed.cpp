#include "LineUtils.hpp"
#include "Bed.hpp"

#include <cstring>

using namespace std;

Bed Bed::parseLine(const std::string& line) {
    Bed bed;
    bed.line = line;

    string::size_type fldBegin = 0;
    string::size_type fldEnd = 0;
    extractField(line, 0, 0, fldBegin, fldEnd);
    bed.chrom = line.substr(fldBegin, fldEnd-fldBegin);

    extractField(line, fldEnd+1, 0, fldBegin, fldEnd);
    if (!extractInt(&line[fldBegin], &line[fldEnd], bed.start)) {
        stringstream errmsg;
        errmsg << "Failed to extract bed start position from line '" << line <<
        "' at position [" << fldBegin << ", " << fldEnd << ")";
        throw runtime_error(errmsg.str());
    }

    extractField(line, fldEnd+1, 0, fldBegin, fldEnd);
    if (!extractInt(&line[fldBegin], &line[fldEnd], bed.end)) {
        stringstream errmsg;
        errmsg << "Failed to extract bed end position from line '" << line <<
        "' at position [" << fldBegin << ", " << fldEnd << ")";
        throw runtime_error(errmsg.str());
    }

    extractField(line, fldEnd+1, 0, fldBegin, fldEnd);
    bed.refCall = line.substr(fldBegin, fldEnd-fldBegin);
    
    extractField(line, fldEnd+1, 0, fldBegin, fldEnd);
    bed.qual = line.substr(fldBegin, fldEnd-fldBegin);

    return bed;
}

int Bed::cmp(const Bed& rhs) const {
    int rv = strverscmp(chrom.c_str(), rhs.chrom.c_str());
    if (rv != 0)
        return rv;

    if (start < rhs.start)
        return -1;
    if (rhs.start < start)
        return 1;

    if (end < rhs.end)
        return -1;
    if (rhs.end < end)
        return 1;

    return 0;
}
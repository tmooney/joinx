#pragma once

#include "Entry.hpp"

#include "common/Sequence.hpp"
#include "common/namespaces.hpp"

#include <vector>
#include <string>
#include <utility>

class Fasta;

BEGIN_NAMESPACE(Vcf)

// VCF can have a lot of extra padding around a variant
// RawVariant strips this away to expose just the relevant bases
// The sequence name is not represented since it is assumed that
// instances of this class will only be compared on the same
// sequence anyway.
class RawVariant {
public:
    static RawVariant None;

    static std::vector<RawVariant> processEntry(Entry const& e) {
        vector<RawVariant> rv;
        for (auto alt = e.alt().begin(); alt != e.alt().end(); ++alt)
            rv.emplace_back(e.pos(), e.ref(), *alt);
        return rv;
    }

    RawVariant(int64_t pos, std::string const& ref, std::string const& alt)
        : pos(pos)
        , ref(ref)
        , alt(alt)
    {
        normalize();
    }

    RawVariant(int64_t pos, std::string&& ref, std::string&& alt)
        : pos(pos)
        , ref(ref)
        , alt(alt)
    {
        normalize();
    }

    bool operator==(RawVariant const& rhs) const {
        return pos == rhs.pos && ref == rhs.ref && alt == rhs.alt;
    }

    bool operator!=(RawVariant const& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(RawVariant const& rhs) const {
        if (pos < rhs.pos) return true;
        if (pos > rhs.pos) return false;

        if (ref < rhs.ref) return true;
        if (ref > rhs.ref) return false;

        if (alt < rhs.alt) return true;
        return false;
    }

    int64_t lastRefPos() const {
        return pos + ref.size() - 1;
    }

    int64_t lastAltPos() const {
        return pos + alt.size() - 1;
    }

    std::pair<RawVariant, RawVariant> splitIndelWithSubstitution() const;
    RawVariant mergeIndelWithSubstitution(std::pair<RawVariant, RawVariant> const& vars) const;

protected:
    void normalize();

public:
    int64_t pos;
    std::string ref;
    std::string alt;
};

END_NAMESPACE(Vcf)
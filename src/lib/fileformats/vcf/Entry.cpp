#include "Entry.hpp"
#include "EntryMerger.hpp"
#include "CustomValue.hpp"
#include "GenotypeFormatter.hpp"
#include "Header.hpp"
#include "MergeStrategy.hpp"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>

using boost::format;
using boost::lexical_cast;
using namespace placeholders;
using namespace std;

VCF_NAMESPACE_BEGIN
namespace {
    string::size_type commonPrefix(const string& a, const string& b) {
        string::size_type p = 0;
        while (p < a.size() && p < b.size() && a[p] == b[p])
            ++p;
        return p;
    }
}

const double Entry::MISSING_QUALITY = numeric_limits<double>::min();

Entry::Entry()
    : _header(0)
    , _pos(0)
    , _qual(MISSING_QUALITY)
    , _start(0)
    , _stop(0)
{
}

Entry::Entry(const Header* h)
    : _header(h)
    , _pos(0)
    , _qual(MISSING_QUALITY)
    , _start(0)
    , _stop(0)
{
}

Entry::Entry(const Header* h, const string& s)
    : _header(h)
    , _start(0)
    , _stop(0)
{
    parse(h, s);
}

Entry::Entry(const EntryMerger& merger)
    : _header(merger.mergedHeader())
    , _chrom(merger.chrom())
    , _pos(merger.pos())
    , _ref(merger.ref())
    , _qual(merger.qual())
    , _start(0)
    , _stop(0)
{
    const std::set<std::string>& idents = merger.identifiers();
    copy(idents.begin(), idents.end(), back_inserter(_identifiers));

    const std::set<std::string>& filts = merger.failedFilters();
    copy(filts.begin(), filts.end(), back_inserter(_failedFilters));

    merger.setInfo(_info);
    merger.setAltAndGenotypeData(_alt, _formatDescription, _genotypeData);
    setPositions();
}

const Header& Entry::header() const {
    if (!_header)
        throw runtime_error("Attempted to use Vcf Entry with no header!");
    return *_header;
}

void Entry::parse(const Header* h, const string& s) {
    _header = h;

    Tokenizer<char> tok(s, '\t');
    if (!tok.extract(_chrom))
        throw runtime_error("Failed to extract chromosome from vcf entry: " + s);
    if (!tok.extract(_pos))
        throw runtime_error("Failed to extract position from vcf entry: " + s);

    string tmp;

    // ids
    if (!tok.extract(tmp))
        throw runtime_error("Failed to extract id from vcf entry: " + s);
    extractList(_identifiers, tmp);

    // ref alleles
    if (!tok.extract(_ref))
        throw runtime_error("Failed to extract ref alleles from vcf entry: " + s);

    // alt alleles
    if (!tok.extract(tmp))
        throw runtime_error("Failed to extract alt alleles from vcf entry: " + s);
    extractList(_alt, tmp, ',');

    // phred quality
    string qualstr;
    if (!tok.extract(qualstr))
        throw runtime_error("Failed to extract quality from vcf entry: " + s);
    if (qualstr == ".")
        _qual = MISSING_QUALITY;
    else
        _qual = lexical_cast<double>(qualstr);

    // failed filters
    if (!tok.extract(tmp))
        throw runtime_error("Failed to extract filters from vcf entry: " + s);
    extractList(_failedFilters, tmp);
    if (_failedFilters.size() == 1 && _failedFilters[0] == "PASS")
        _failedFilters.clear();

    // info entries
    if (!tok.extract(tmp))
        throw runtime_error("Failed to extract info from vcf entry: " + s);
    vector<string> infoStrings;
    extractList(infoStrings, tmp);

    // TODO: refactor into function addInfoField(s)
    _info.clear();
    for (auto i = infoStrings.begin(); i != infoStrings.end(); ++i) {
        if (i->empty())
            continue;

        Tokenizer<char> kv(*i, '=');
        string key;
        string value;
        kv.extract(key);
        kv.remaining(value);
        const CustomType* type = header().infoType(key);
        if (type == NULL)
            throw runtime_error(str(format("Failed to lookup type for info field '%1%'") %key));
        auto inserted = _info.insert(make_pair(key, CustomValue(type, value)));
        if (!inserted.second)
            throw runtime_error(str(format("Duplicate value for info field '%1%'") %key));
    }

    // TODO: refactor into function
    // format description
    if (tok.extract(tmp)) {
        extractList(_formatDescription, tmp, ':');
        for (auto i = _formatDescription.begin(); i != _formatDescription.end(); ++i) {
            if (i->empty())
                continue;
            if (!header().formatType(*i))
                throw runtime_error(str(format("Unknown id in FORMAT field: %1%") %*i));
        }

        _genotypeData.clear();
        // per sample formatted data
        while (tok.extract(tmp)) {
            vector<string> data;
            extractList(data, tmp, ':');
            if (data.size() > _formatDescription.size())
                throw runtime_error("More per-sample values than described in format section");

            vector<CustomValue> perSampleValues;
            for (uint32_t i = 0; i < data.size(); ++i) {
                const CustomType* type = header().formatType(_formatDescription[i]);
                perSampleValues.push_back(CustomValue(type, data[i]));
            }
            _genotypeData.push_back(perSampleValues);
        }
    }
    setPositions();
}

string Entry::toString() const {
    stringstream ss;
    ss << *this;
    return ss.str();
}

int Entry::cmp(const Entry& rhs) const {
    int rv = strverscmp(_chrom.c_str(), rhs._chrom.c_str());
    if (rv != 0)
        return rv;

    if (_pos < rhs._pos)
        return -1;
    if (rhs._pos < _pos)
        return 1;

    return 0;
}

void Entry::swap(Entry& other) {
    _chrom.swap(other._chrom);
    std::swap(_pos, other._pos);
    _identifiers.swap(other._identifiers);
    _ref.swap(other._ref);
    _alt.swap(other._alt);
    std::swap(_qual, other._qual);
    _failedFilters.swap(other._failedFilters);
    _info.swap(other._info);
    _formatDescription.swap(other._formatDescription);
    _genotypeData.swap(other._genotypeData);
    std::swap(_header, other._header);
}

int32_t Entry::altIdx(const string& alt) const {
    auto i = find(_alt.begin(), _alt.end(), alt);
    if (i == _alt.end())
        return -1;
    return distance(_alt.begin(), i);
}

const CustomValue* Entry::info(const string& key) const {
    auto i = _info.find(key);
    if (i == _info.end())
        return 0;
    return &i->second;
}

const CustomValue* Entry::genotypeData(uint32_t sampleIdx, const string& key) const {
    // no data for that sample
    if (sampleIdx >= _genotypeData.size() || _genotypeData[sampleIdx].empty())
        return 0;

    // no info for that format key
    auto i = find(_formatDescription.begin(), _formatDescription.end(), key);
    if (i == _formatDescription.end())
        return 0;

    uint32_t offset = distance(_formatDescription.begin(), i);
    if (offset >= _genotypeData[sampleIdx].size())
        return 0;
    return &_genotypeData[sampleIdx][offset];
}

void Entry::removeLowDepthGenotypes(uint32_t lowDepth) {
    auto i = find(_formatDescription.begin(), _formatDescription.end(), "DP");
    if (i == _formatDescription.end())
        return;

    uint32_t offset = distance(_formatDescription.begin(), i);
    for (auto i = _genotypeData.begin(); i != _genotypeData.end(); ++i) {
        if (i->empty())
            continue;
        const int64_t *v;
        if ((*i)[offset].empty() || (v = (*i)[offset].get<int64_t>(0)) == NULL || *v < lowDepth)
            i->clear();
    }
}

uint32_t Entry::samplesWithData() const {
    uint32_t rv = 0;
    for (auto i = _genotypeData.begin(); i != _genotypeData.end(); ++i) {
        if (i->size())
            ++rv;
    }
    return rv;
}

void Entry::setPositions() {
    _start = _stop = _pos;
    for (uint32_t idx = 0; idx < _alt.size(); ++idx) {
        string::size_type prefix = commonPrefix(_ref, _alt[idx]);
        int64_t start = _pos - 1 + prefix;
        int64_t stop;
        if (_alt[idx].size() == _ref.size()) {
            stop = start + _alt[idx].size() - prefix;
        } else {
            // VCF prepends 1 base to indels
            if (_alt[idx].size() < _ref.size()) { // deletion
                stop = start + _ref.size();
            } else if (_alt[idx].size() > _ref.size()) { // insertion
                ++start;
                stop = start;
            } else // let's see if this ever happens!
                throw runtime_error(str(format("Unknown variant type, allele %1%: %2%") %idx %toString()));
        }
        _start = min(_start, start);
        _stop = max(_stop, stop);
    }
}

int64_t Entry::start() const {
    return _start;
}

int64_t Entry::stop() const {
    return _stop;
}

VCF_NAMESPACE_END

ostream& operator<<(ostream& s, const Vcf::Entry& e) {
    s << e.chrom() << '\t' << e.pos() << '\t';
    e.printList(s, e.identifiers());
    s << '\t' << e.ref() << '\t';
    e.printList(s, e.alt(), ',');
    if (e.qual() <= Vcf::Entry::MISSING_QUALITY)
        s << "\t.\t";
    else
        s << '\t' << e.qual() << '\t';

    if (e.failedFilters().empty())
        s << "PASS";
    else
        e.printList(s, e.failedFilters());
    s << '\t';

    const Vcf::Entry::CustomValueMap& info = e.info();
    if (info.empty()) {
        s << '.';
    } else {
        for (auto i = info.begin(); i != info.end(); ++i) {
            if (i != info.begin())
                s << ';';
            s << i->second.type().id();
            string value = i->second.toString();
            if (!value.empty())
                s << "=" << value;
        }
    }
    s << '\t';

    e.printList(s, e.formatDescription(), ':');
    const vector< vector<Vcf::CustomValue> >& psd = e.genotypeData();
    for (auto i = psd.begin(); i != psd.end(); ++i) {
        s << '\t';
        if (!i->empty()) {
            for (auto j = i->begin(); j != i->end(); ++j) {
                if (j != i->begin())
                    s << ':';
                if (j->empty())
                    s << '.';
                else
                    j->toStream(s);
            }
        } else
            s << ".";
    }
    return s;
}

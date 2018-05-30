#include "Utils/PrismLog.hpp"
#include "StgenCapnpParser.hpp"
#include <fcntl.h>

//-----------------------------------------------------------------------------
inline AutoCloseGz::AutoCloseGz(): fz(nullptr) {}
inline AutoCloseGz::AutoCloseGz(gzFile fz): fz(fz) {}
inline AutoCloseGz::AutoCloseGz(AutoCloseGz&& other) noexcept: fz(other.fz) {
    other.fz = nullptr;
}
AutoCloseGz::~AutoCloseGz() {
    if (fz != nullptr) {
        int res = gzclose(fz);
        if (res != Z_OK) {
            PrismLog::warn("error closing gz file");
        }
    }
}

inline AutoCloseGz& AutoCloseGz::operator=(AutoCloseGz&& other) {
    AutoCloseGz old(kj::mv(*this));
    fz = other.fz;
    other.fz = nullptr;
    return *this;
}

inline AutoCloseGz::operator gzFile() const {
    return fz;
}

inline AutoCloseGz::operator bool() const {
    return (fz == nullptr) ? false : true;
}

inline gzFile AutoCloseGz::release() {
    gzFile res = fz;
    fz = nullptr;
    return res;
}


//-----------------------------------------------------------------------------
GzInputStream::GzInputStream(gzFile fz): fz(fz) {}
GzInputStream::GzInputStream(AutoCloseGz fz): fz(fz), autoclose(std::move(fz)) {}
GzInputStream::~GzInputStream() {}

auto GzInputStream::tryRead(void* buffer, size_t minBytes, size_t maxBytes) -> size_t
{
    kj::byte *pos = reinterpret_cast<kj::byte*>(buffer);
    kj::byte *min = pos + minBytes;
    kj::byte *max = pos + maxBytes;

    while (pos < min) {
        int n = gzread(fz, pos, max-pos);

        if (n == 0) {
            int errnum;
            const char* errmsg = gzerror(fz, &errnum);
            if (errnum == Z_STREAM_END ||
                errnum == Z_OK) {
                break;
            } else  {
                PrismLog::fatal("Reading from gz\n"
                                "Code    : {}\n"
                                "Message : {}", errnum, errmsg);
            }
        }

        pos += n;
    }

    return pos - reinterpret_cast<kj::byte*>(buffer);
}

inline gzFile GzInputStream::getFz() const {
    return fz;
}


//-----------------------------------------------------------------------------
PackedGzMessageReader::PackedGzMessageReader(gzFile fz, capnp::ReaderOptions options,
                                             kj::ArrayPtr<capnp::word> scratchspace)
    : GzInputStream(fz),
      BufferedInputStreamWrapper(static_cast<GzInputStream&>(*this)),
      PackedMessageReader(static_cast<BufferedInputStream&>(*this),
                          options, scratchspace) {}

PackedGzMessageReader::PackedGzMessageReader(AutoCloseGz fz, capnp::ReaderOptions options,
                                             kj::ArrayPtr<capnp::word> scratchspace)
    : GzInputStream(std::move(fz)),
      BufferedInputStreamWrapper(static_cast<GzInputStream&>(*this)),
      PackedMessageReader(static_cast<BufferedInputStreamWrapper&>(*this),
                          options, scratchspace) {}

PackedGzMessageReader::~PackedGzMessageReader() {}


//-----------------------------------------------------------------------------
PackedGzMessageGenerator::PackedGzMessageGenerator(AutoCloseGz ac,
                                                   capnp::ReaderOptions options)
    : GzInputStream(std::move(ac))
    , BufferedInputStreamWrapper(static_cast<GzInputStream&>(*this))
    , options(options) {}

PackedGzMessageGenerator::~PackedGzMessageGenerator() {}

auto PackedGzMessageGenerator::next()
    -> std::unique_ptr<capnp::PackedMessageReader>
{
    if (tryGetReadBuffer().size() != 0) {
        return std::make_unique<capnp::PackedMessageReader>
            (static_cast<kj::BufferedInputStreamWrapper&>(*this), options);
    } else {
        return nullptr;
    }
}


PackedFdMessageGenerator::PackedFdMessageGenerator(kj::AutoCloseFd ac,
                                                   capnp::ReaderOptions options)
    : kj::FdInputStream(std::move(ac))
    , BufferedInputStreamWrapper(static_cast<FdInputStream&>(*this))
    , options(options) {}

PackedFdMessageGenerator::~PackedFdMessageGenerator() {}

auto PackedFdMessageGenerator::next()
    -> PackedMultipleMessageGenerator::iterator::value_type
{
    if (tryGetReadBuffer().size() != 0) {
        return std::make_unique<capnp::PackedMessageReader>
            (static_cast<kj::BufferedInputStreamWrapper&>(*this), options);
    } else {
        return nullptr;
    }
}


//-----------------------------------------------------------------------------
PackedMultipleMessageGenerator::PackedMultipleMessageGenerator(std::filesystem::path fpath,
                                                               capnp::ReaderOptions options)
    : it({nullptr, nullptr})
{
    auto ext = fpath.extension();
    if (ext.compare(".gz") == 0) {
        PrismLog::info("Parsing .gz file: {}", fpath.string());
        auto ac = AutoCloseGz(gzopen(fpath.c_str(), "rb"));
        if (ac == Z_NULL) PrismLog::fatal("Error opening gz file");
        auto generator = std::make_unique<PackedGzMessageGenerator>(std::move(ac), options);
        it = {generator->next(), std::move(generator)};
    } else if (ext.compare(".bin") == 0) {
        PrismLog::info("Parsing a raw file: {}", fpath.string());
        auto ac = kj::AutoCloseFd(open(fpath.c_str(), O_RDONLY));
        if ((int)ac == -1) PrismLog::fatal("Error opening bin file");
        auto generator = std::make_unique<PackedFdMessageGenerator>(std::move(ac), options);
        it = {generator->next(), std::move(generator)};
    } else {
        PrismLog::fatal("unexpected file extension: {}", ext.string());
    }
}

auto PackedMultipleMessageGenerator::begin() -> iterator {
    if (it == nullptr) {
        throw std::runtime_error("Invalid state for MultipleMessageGenerator! "
                                 "Did you call .begin() twice?");
    }

    return std::move(it);
}

auto PackedMultipleMessageGenerator::end() -> iterator {
    return {nullptr, nullptr};
}

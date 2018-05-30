#include "STEventTraceSchemas/STEventTraceCompressed.capnp.h"
#include "STEventTraceSchemas/STEventTraceUncompressed.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>
#include <filesystem> // use new C++17 coolness

// Based off of fd implementations in kj/capnp libraries

#define DEBUG

class AutoCloseGz {
  public:
    AutoCloseGz();
    explicit AutoCloseGz(gzFile fz);
    AutoCloseGz(AutoCloseGz&& other) noexcept;
    KJ_DISALLOW_COPY(AutoCloseGz);
    ~AutoCloseGz();

    AutoCloseGz& operator=(AutoCloseGz&& other);
    operator gzFile() const;
    operator bool() const;

    inline gzFile release();

  private:
    gzFile fz;
};


class GzInputStream: public kj::InputStream {
  public:
    explicit GzInputStream(gzFile fz);
    explicit GzInputStream(AutoCloseGz fz);
    KJ_DISALLOW_COPY(GzInputStream);
    ~GzInputStream();

    virtual auto tryRead(void* buffer, size_t minBytes, size_t maxBytes) -> size_t
        override final;

    auto getFz() const -> gzFile;

  private:
    gzFile fz;
    AutoCloseGz autoclose;
};


class PackedGzMessageReader: private GzInputStream, private kj::BufferedInputStreamWrapper,
                             public capnp::PackedMessageReader {
  public:
    PackedGzMessageReader(gzFile fz, capnp::ReaderOptions options = capnp::ReaderOptions(),
                          kj::ArrayPtr<capnp::word> scratchspace = nullptr);
    PackedGzMessageReader(AutoCloseGz fz, capnp::ReaderOptions options = capnp::ReaderOptions(),
                          kj::ArrayPtr<capnp::word> scratchspace = nullptr);
    KJ_DISALLOW_COPY(PackedGzMessageReader);
    ~PackedGzMessageReader();
};


//-----------------------------------------------------------------------------
// Convenience containers

class PackedMessageGenerator {
  public:
    // returns nullptr if no messages left/available
    virtual auto next() -> std::unique_ptr<::capnp::PackedMessageReader> = 0;
    virtual ~PackedMessageGenerator() noexcept(false) {};
};


class PackedGzMessageGenerator
    : GzInputStream
    , kj::BufferedInputStreamWrapper
    , public PackedMessageGenerator {
  public:
    PackedGzMessageGenerator(AutoCloseGz ac,
                             capnp::ReaderOptions options = capnp::ReaderOptions());
    ~PackedGzMessageGenerator();
    virtual auto next() -> std::unique_ptr<::capnp::PackedMessageReader> override final;

  private:
    capnp::ReaderOptions options;
};


class PackedFdMessageGenerator
    : kj::FdInputStream
    , kj::BufferedInputStreamWrapper
    , public PackedMessageGenerator {
  public:
    PackedFdMessageGenerator(kj::AutoCloseFd ac,
                             capnp::ReaderOptions options = capnp::ReaderOptions());
    ~PackedFdMessageGenerator();
    virtual auto next() -> std::unique_ptr<::capnp::PackedMessageReader> override final;

  private:
    capnp::ReaderOptions options;
};


class PackedMultipleMessageGenerator {
  public:
    // XXX Can only iterate over (begin -> end) once!

    class iterator {
        // used for iterating over messages in both normal files (fd)
        // and compressed files (gz)
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::unique_ptr<::capnp::PackedMessageReader>;
        using pointer = std::unique_ptr<const value_type>;
        using reference = const value_type&;
        using difference_type = size_t;

        iterator(value_type message, std::unique_ptr<PackedMessageGenerator> gen)
            : message(std::move(message)), generator(std::move(gen)) {}

        value_type operator* () { return std::move(message); };
        iterator& operator++() {
            message = generator->next();
            return *this;
        }

        bool operator==(const iterator& rhs) { return message == rhs.message; }
        bool operator==(const decltype(nullptr) rhs) { return message == rhs; }
        bool operator!=(const iterator& rhs) { return !(*this == rhs); }

      private:
        value_type message;
        std::unique_ptr<PackedMessageGenerator> generator;
    };

    PackedMultipleMessageGenerator(std::filesystem::path fpath,
                                   capnp::ReaderOptions options = capnp::ReaderOptions());
    iterator begin();
    iterator end();

  private:
    iterator it;
};

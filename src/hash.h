// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HASH_H
#define BITCOIN_HASH_H

#include <arith_uint256.h>
#include <attributes.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
//#include <crypto/sha512.h>
#include <prevector.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>

#include <crypto/sph_blake.h>
#include <crypto/sph_bmw.h>
#include <crypto/sph_groestl.h>
#include <crypto/sph_jh.h>
#include <crypto/sph_keccak.h>
#include <crypto/sph_skein.h>
#include <crypto/sph_luffa.h>
#include <crypto/sph_cubehash.h>
#include <crypto/sph_shavite.h>
#include <crypto/sph_simd.h>
#include <crypto/sph_echo.h>
#include <crypto/sph_hamsi.h>
#include <crypto/sph_fugue.h>
#include <crypto/sph_shabal.h>
#include <crypto/sph_whirlpool.h>
#include <crypto/sph_sha2.h>
#include <crypto/sph_haval.h>

#include <string>
#include <vector>

typedef uint256 ChainCode;

/* ----------- XUEZ Hash ------------------------------------------------- */
/** A hasher class for XUEZ's 256-bit hash (double SHA-256). */
class CHash256 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash256& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};

/** A hasher class for XUEZ's 160-bit hash (SHA-256 + RIPEMD-160). */
class CHash160 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CRIPEMD160::OUTPUT_SIZE;

    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        CRIPEMD160().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash160& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash160& Reset() {
        sha.Reset();
        return *this;
    }
};

/** A hasher class for XUEZ's 160-bit hash (double SHA-1). */
class CHash1 {
private:
    CSHA1 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA1::OUTPUT_SIZE;

    void Finalize(Span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA1::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA1::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash1& Write(Span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash1& Reset() {
        sha.Reset();
        return *this;
    }
};

/** Compute the 256-bit hash of an object. */
template<typename T>
inline uint256 Hash(const T& in1)
{
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template<typename T1, typename T2>
inline uint256 Hash(const T1& in1, const T2& in2) {
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Write(MakeUCharSpan(in2)).Finalize(result);
    return result;
}

/** Compute the 160-bit hash of an object. */
template<typename T1>
inline uint160 Hash160(const T1& in1)
{
    uint160 result;
    CHash160().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** Compute the 160-bit hash of an object. */
template<typename T1>
inline uint256 Hash1(const T1& in1)
{
    static const arith_uint256 TRAILING_BITS("0000000000000000000000000000000000000000ffffffffffffffffffffffff");
    uint160 result;
    CHash1().Write(MakeUCharSpan(in1)).Finalize(result);
    //const Span<const unsigned char> &input = MakeUCharSpan(in1);
    //CSHA1().Write(input.data(), input.size()).Finalize(result.data());
    return ArithToUint256((Uint160ToArith256(result) << 96) | TRAILING_BITS);
}

/** A writer stream (for serialization) that computes a 256-bit hash. */
class CHashWriter
{
private:
    CSHA256 ctx;

    const int nType;
    const int nVersion;
public:

    CHashWriter(int nTypeIn, int nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) {}

    int GetType() const { return nType; }
    int GetVersion() const { return nVersion; }

    void write(const char *pch, size_t size) {
        ctx.Write((const unsigned char*)pch, size);
    }

    /** Compute the double-SHA256 hash of all data written to this object.
     *
     * Invalidates this object.
     */
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize(result.begin());
        ctx.Reset().Write(result.begin(), CSHA256::OUTPUT_SIZE).Finalize(result.begin());
        return result;
    }

    /** Compute the SHA256 hash of all data written to this object.
     *
     * Invalidates this object.
     */
    uint256 GetSHA256() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }

    /**
     * Returns the first 64 bits from the resulting hash.
     */
    inline uint64_t GetCheapHash() {
        uint256 result = GetHash();
        return ReadLE64(result.begin());
    }

    template<typename T>
    CHashWriter& operator<<(const T& obj) {
        // Serialize to this stream
        ::Serialize(*this, obj);
        return (*this);
    }
};

/** Reads data from an underlying stream, while hashing the read data. */
template<typename Source>
class CHashVerifier : public CHashWriter
{
private:
    Source* source;

public:
    explicit CHashVerifier(Source* source_) : CHashWriter(source_->GetType(), source_->GetVersion()), source(source_) {}

    void read(char* pch, size_t nSize)
    {
        source->read(pch, nSize);
        this->write(pch, nSize);
    }

    void ignore(size_t nSize)
    {
        char data[1024];
        while (nSize > 0) {
            size_t now = std::min<size_t>(nSize, 1024);
            read(data, now);
            nSize -= now;
        }
    }

    template<typename T>
    CHashVerifier<Source>& operator>>(T&& obj)
    {
        // Unserialize from this stream
        ::Unserialize(*this, obj);
        return (*this);
    }
};

/** Compute the 256-bit hash of an object's serialization. */
template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=PROTOCOL_VERSION)
{
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}

/** Single-SHA256 a 32-byte input (represented as uint256). */
NODISCARD uint256 SHA256Uint256(const uint256& input);

unsigned int MurmurHash3(unsigned int nHashSeed, Span<const unsigned char> vDataToHash);

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]);

/** Return a CHashWriter primed for tagged hashes (as specified in BIP 340).
 *
 * The returned object will have SHA256(tag) written to it twice (= 64 bytes).
 * A tagged hash can be computed by feeding the message into this object, and
 * then calling CHashWriter::GetSHA256().
 */
CHashWriter TaggedHash(const std::string& tag);

/* ----------- Quark Hash ------------------------------------------------- */
template<typename T1>
inline uint256 HashQuark(const T1& in1)
{
    sph_blake512_context      ctx_blake;
    sph_bmw512_context        ctx_bmw;
    sph_groestl512_context    ctx_groestl;
    sph_jh512_context         ctx_jh;
    sph_keccak512_context     ctx_keccak;
    sph_skein512_context      ctx_skein;

    constexpr unsigned int inputHashLength = 64;

    const Span<const unsigned char> &input = MakeUCharSpan(in1);
    unsigned char hashA[inputHashLength], hashB[inputHashLength];

    sph_blake512_init(&ctx_blake);
    // ZBLAKE;
    sph_blake512(&ctx_blake, input.data(), input.size());
    sph_blake512_close(&ctx_blake, static_cast<void*>(hashA));

    sph_bmw512_init(&ctx_bmw);
    // ZBMW;
    sph_bmw512(&ctx_bmw, static_cast<const void*>(hashA), inputHashLength);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(hashB));

    if (hashB[0] & 0x8) {
        sph_groestl512_init(&ctx_groestl);
        // ZGROESTL;
        sph_groestl512(&ctx_groestl, static_cast<const void*>(hashB), inputHashLength);
        sph_groestl512_close(&ctx_groestl, static_cast<void*>(hashA));
    } else {
        sph_skein512_init(&ctx_skein);
        // ZSKEIN;
        sph_skein512(&ctx_skein, static_cast<const void*>(hashB), inputHashLength);
        sph_skein512_close(&ctx_skein, static_cast<void*>(hashA));
    }

    sph_groestl512_init(&ctx_groestl);
    // ZGROESTL;
    sph_groestl512(&ctx_groestl, static_cast<const void*>(hashA), inputHashLength);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(hashB));

    sph_jh512_init(&ctx_jh);
    // ZJH;
    sph_jh512(&ctx_jh, static_cast<const void*>(hashB), inputHashLength);
    sph_jh512_close(&ctx_jh, static_cast<void*>(hashA));

    if (hashA[0] & 0x8) {
        sph_blake512_init(&ctx_blake);
        // ZBLAKE;
        sph_blake512(&ctx_blake, static_cast<const void*>(hashA), inputHashLength);
        sph_blake512_close(&ctx_blake, static_cast<void*>(hashB));
    } else {
        sph_bmw512_init(&ctx_bmw);
        // ZBMW;
        sph_bmw512(&ctx_bmw, static_cast<const void*>(hashA), inputHashLength);
        sph_bmw512_close(&ctx_bmw, static_cast<void*>(hashB));
    }

    sph_keccak512_init(&ctx_keccak);
    // ZKECCAK;
    sph_keccak512(&ctx_keccak, static_cast<const void*>(hashB), inputHashLength);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(hashA));

    sph_skein512_init(&ctx_skein);
    // SKEIN;
    sph_skein512(&ctx_skein, static_cast<const void*>(hashA), inputHashLength);
    sph_skein512_close(&ctx_skein, static_cast<void*>(hashB));

    if (hashB[0] & 0x8) {
        sph_keccak512_init(&ctx_keccak);
        // ZKECCAK;
        sph_keccak512(&ctx_keccak, static_cast<const void*>(hashB), inputHashLength);
        sph_keccak512_close(&ctx_keccak, static_cast<void*>(hashA));
    } else {
        sph_jh512_init(&ctx_jh);
        // ZJH;
        sph_jh512(&ctx_jh, static_cast<const void*>(hashB), inputHashLength);
        sph_jh512_close(&ctx_jh, static_cast<void*>(hashA));
    }

    uint256 result;
    memcpy(result.data(), hashA, 32);
    return result;
}

/* ----------- Xevan Hash ------------------------------------------------- */
template<typename T1>
inline uint256 HashXevan(const T1& in1)
{
    sph_blake512_context      ctx_blake;
    sph_bmw512_context        ctx_bmw;
    sph_groestl512_context    ctx_groestl;
    sph_jh512_context         ctx_jh;
    sph_keccak512_context     ctx_keccak;
    sph_skein512_context      ctx_skein;
    sph_luffa512_context      ctx_luffa;
    sph_cubehash512_context   ctx_cubehash;
    sph_shavite512_context    ctx_shavite;
    sph_simd512_context       ctx_simd;
    sph_echo512_context       ctx_echo;
    sph_hamsi512_context      ctx_hamsi;
    sph_fugue512_context      ctx_fugue;
    sph_shabal512_context     ctx_shabal;
    sph_whirlpool_context     ctx_whirlpool;
    sph_sha512_context        ctx_sha2;
    sph_haval256_5_context    ctx_haval;

    constexpr unsigned int inputHashLength = 128; // Note this causes 64 bytes of zeroed memory to be read beyond the hash

    const Span<const unsigned char> &input = MakeUCharSpan(in1);
    unsigned char hashA[inputHashLength], hashB[inputHashLength];
    memset(&hashA[64], '\0', inputHashLength - 64); // Write zeroes to the bytes which are not overwritten by the hash output
    memset(&hashB[64], '\0', inputHashLength - 64);

    sph_blake512_init(&ctx_blake);
    sph_blake512(&ctx_blake, input.data(), input.size());
    sph_blake512_close(&ctx_blake, static_cast<void*>(hashA));

    sph_bmw512_init(&ctx_bmw);
    sph_bmw512(&ctx_bmw, static_cast<const void*>(hashA), inputHashLength);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(hashB));

    sph_groestl512_init(&ctx_groestl);
    sph_groestl512(&ctx_groestl, static_cast<const void*>(hashB), inputHashLength);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(hashA));

    sph_skein512_init(&ctx_skein);
    sph_skein512(&ctx_skein, static_cast<const void*>(hashA), inputHashLength);
    sph_skein512_close(&ctx_skein, static_cast<void*>(hashB));

    sph_jh512_init(&ctx_jh);
    sph_jh512(&ctx_jh, static_cast<const void*>(hashB), inputHashLength);
    sph_jh512_close(&ctx_jh, static_cast<void*>(hashA));

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512(&ctx_keccak, static_cast<const void*>(hashA), inputHashLength);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(hashB));

    sph_luffa512_init(&ctx_luffa);
    sph_luffa512(&ctx_luffa, static_cast<void*>(hashB), inputHashLength);
    sph_luffa512_close(&ctx_luffa, static_cast<void*>(hashA));

    sph_cubehash512_init(&ctx_cubehash);
    sph_cubehash512(&ctx_cubehash, static_cast<const void*>(hashA), inputHashLength);
    sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(hashB));

    sph_shavite512_init(&ctx_shavite);
    sph_shavite512(&ctx_shavite, static_cast<const void*>(hashB), inputHashLength);
    sph_shavite512_close(&ctx_shavite, static_cast<void*>(hashA));

    sph_simd512_init(&ctx_simd);
    sph_simd512(&ctx_simd, static_cast<const void*>(hashA), inputHashLength);
    sph_simd512_close(&ctx_simd, static_cast<void*>(hashB));

    sph_echo512_init(&ctx_echo);
    sph_echo512(&ctx_echo, static_cast<const void*>(hashB), inputHashLength);
    sph_echo512_close(&ctx_echo, static_cast<void*>(hashA));

    sph_hamsi512_init(&ctx_hamsi);
    sph_hamsi512(&ctx_hamsi, static_cast<const void*>(hashA), inputHashLength);
    sph_hamsi512_close(&ctx_hamsi, static_cast<void*>(hashB));

    sph_fugue512_init(&ctx_fugue);
    sph_fugue512(&ctx_fugue, static_cast<const void*>(hashB), inputHashLength);
    sph_fugue512_close(&ctx_fugue, static_cast<void*>(hashA));

    sph_shabal512_init(&ctx_shabal);
    sph_shabal512(&ctx_shabal, static_cast<const void*>(hashA), inputHashLength);
    sph_shabal512_close(&ctx_shabal, static_cast<void*>(hashB));

    sph_whirlpool_init(&ctx_whirlpool);
    sph_whirlpool(&ctx_whirlpool, static_cast<const void*>(hashB), inputHashLength);
    sph_whirlpool_close(&ctx_whirlpool, static_cast<void*>(hashA));

    sph_sha512_init(&ctx_sha2);
    sph_sha512(&ctx_sha2, static_cast<const void*>(hashA), inputHashLength);
    sph_sha512_close(&ctx_sha2, static_cast<void*>(hashB));
    //CSHA512().Write(reinterpret_cast<const unsigned char*>(hashA), inputHashLength).Finalize(reinterpret_cast<unsigned char*>(hashB));

    sph_haval256_5_init(&ctx_haval);
    sph_haval256_5(&ctx_haval, static_cast<const void*>(hashB), inputHashLength);
    sph_haval256_5_close(&ctx_haval, static_cast<void*>(hashA));
    memset(&hashA[32], '\0', 32); // Write zeroes to the other 256 bits which were not already overwritten

    // Part 2
    sph_blake512_init(&ctx_blake);
    sph_blake512(&ctx_blake, static_cast<const void*>(hashA), inputHashLength);
    sph_blake512_close(&ctx_blake, static_cast<void*>(hashB));

    sph_bmw512_init(&ctx_bmw);
    sph_bmw512(&ctx_bmw, static_cast<const void*>(hashB), inputHashLength);
    sph_bmw512_close(&ctx_bmw, static_cast<void*>(hashA));

    sph_groestl512_init(&ctx_groestl);
    sph_groestl512(&ctx_groestl, static_cast<const void*>(hashA), inputHashLength);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(hashB));

    sph_skein512_init(&ctx_skein);
    sph_skein512(&ctx_skein, static_cast<const void*>(hashB), inputHashLength);
    sph_skein512_close(&ctx_skein, static_cast<void*>(hashA));

    sph_jh512_init(&ctx_jh);
    sph_jh512(&ctx_jh, static_cast<const void*>(hashA), inputHashLength);
    sph_jh512_close(&ctx_jh, static_cast<void*>(hashB));

    sph_keccak512_init(&ctx_keccak);
    sph_keccak512(&ctx_keccak, static_cast<const void*>(hashB), inputHashLength);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(hashA));

    sph_luffa512_init(&ctx_luffa);
    sph_luffa512(&ctx_luffa, static_cast<void*>(hashA), inputHashLength);
    sph_luffa512_close(&ctx_luffa, static_cast<void*>(hashB));

    sph_cubehash512_init(&ctx_cubehash);
    sph_cubehash512(&ctx_cubehash, static_cast<const void*>(hashB), inputHashLength);
    sph_cubehash512_close(&ctx_cubehash, static_cast<void*>(hashA));

    sph_shavite512_init(&ctx_shavite);
    sph_shavite512(&ctx_shavite, static_cast<const void*>(hashA), inputHashLength);
    sph_shavite512_close(&ctx_shavite, static_cast<void*>(hashB));

    sph_simd512_init(&ctx_simd);
    sph_simd512(&ctx_simd, static_cast<const void*>(hashB), inputHashLength);
    sph_simd512_close(&ctx_simd, static_cast<void*>(hashA));

    sph_echo512_init(&ctx_echo);
    sph_echo512(&ctx_echo, static_cast<const void*>(hashA), inputHashLength);
    sph_echo512_close(&ctx_echo, static_cast<void*>(hashB));

    sph_hamsi512_init(&ctx_hamsi);
    sph_hamsi512(&ctx_hamsi, static_cast<const void*>(hashB), inputHashLength);
    sph_hamsi512_close(&ctx_hamsi, static_cast<void*>(hashA));

    sph_fugue512_init(&ctx_fugue);
    sph_fugue512(&ctx_fugue, static_cast<const void*>(hashA), inputHashLength);
    sph_fugue512_close(&ctx_fugue, static_cast<void*>(hashB));

    sph_shabal512_init(&ctx_shabal);
    sph_shabal512(&ctx_shabal, static_cast<const void*>(hashB), inputHashLength);
    sph_shabal512_close(&ctx_shabal, static_cast<void*>(hashA));

    sph_whirlpool_init(&ctx_whirlpool);
    sph_whirlpool(&ctx_whirlpool, static_cast<const void*>(hashA), inputHashLength);
    sph_whirlpool_close(&ctx_whirlpool, static_cast<void*>(hashB));

    sph_sha512_init(&ctx_sha2);
    sph_sha512(&ctx_sha2, static_cast<const void*>(hashB), inputHashLength);
    sph_sha512_close(&ctx_sha2, static_cast<void*>(hashA));
    //CSHA512().Write(reinterpret_cast<const unsigned char*>(hashB), inputHashLength).Finalize(reinterpret_cast<unsigned char*>(hashA));

    sph_haval256_5_init(&ctx_haval);
    sph_haval256_5(&ctx_haval, static_cast<const void*>(hashA), inputHashLength);
    sph_haval256_5_close(&ctx_haval, static_cast<void*>(hashB));

    uint256 result;
    memcpy(result.data(), hashB, 32);
    return result;
}

/* ----------- Nist5 Hash ------------------------------------------------- */
template<typename T1>
inline uint256 HashNist5(const T1& in1)
{
    sph_blake512_context      ctx_blake;
    sph_groestl512_context    ctx_groestl;
    sph_jh512_context         ctx_jh;
    sph_keccak512_context     ctx_keccak;
    sph_skein512_context      ctx_skein;

    constexpr unsigned int inputHashLength = 64;

    const Span<const unsigned char> &input = MakeUCharSpan(in1);
    unsigned char hashA[inputHashLength], hashB[inputHashLength];

    sph_blake512_init(&ctx_blake);
    // ZBLAKE;
    sph_blake512(&ctx_blake, input.data(), input.size());
    sph_blake512_close(&ctx_blake, static_cast<void*>(hashA));

    sph_groestl512_init(&ctx_groestl);
    // ZGROESTL;
    sph_groestl512(&ctx_groestl, static_cast<const void*>(hashA), inputHashLength);
    sph_groestl512_close(&ctx_groestl, static_cast<void*>(hashB));

    sph_jh512_init(&ctx_jh);
    // ZJH;
    sph_jh512(&ctx_jh, static_cast<const void*>(hashB), inputHashLength);
    sph_jh512_close(&ctx_jh, static_cast<void*>(hashA));

    sph_keccak512_init(&ctx_keccak);
    // ZKECCAK;
    sph_keccak512(&ctx_keccak, static_cast<const void*>(hashA), inputHashLength);
    sph_keccak512_close(&ctx_keccak, static_cast<void*>(hashB));

    sph_skein512_init(&ctx_skein);
    // SKEIN;
    sph_skein512(&ctx_skein, static_cast<const void*>(hashB), inputHashLength);
    sph_skein512_close(&ctx_skein, static_cast<void*>(hashA));

    uint256 result;
    memcpy(result.data(), hashA, 32);
    return result;
}

#endif // BITCOIN_HASH_H

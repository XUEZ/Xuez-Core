// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Copyright (c) 2018-2020 John "ComputerCraftr" Studnicka
// Copyright (c) 2018-2020 The Simplicity developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <streams.h>
#include <tinyformat.h>

uint256 CBlockHeader::GetHash() const
{
    if (nVersion > 4)
        return SerializeHash(*this);
    else if (nVersion == 4) {
        std::vector<unsigned char> vch(112); // block header size with accumulator checkpoint
        CVectorWriter ss(SER_NETWORK, PROTOCOL_VERSION, vch, 0);
        ss << *this;
        return HashXevan(vch);
    } else {
        std::vector<unsigned char> vch(80); // block header size in bytes
        CVectorWriter ss(SER_NETWORK, PROTOCOL_VERSION, vch, 0);
        ss << *this;
        return HashXevan(vch);
    }
}

uint256 CBlockHeader::GetPoWHash() const
{
    if (nVersion != 4) {
        std::vector<unsigned char> vch(80); // block header size in bytes
        CVectorWriter ss(SER_NETWORK, PROTOCOL_VERSION, vch, 0);
        ss << *this;
        return HashXevan(vch);
    } else {
        std::vector<unsigned char> vch(112); // block header size with accumulator checkpoint
        CVectorWriter ss(SER_NETWORK, PROTOCOL_VERSION, vch, 0);
        ss << *this;
        return HashXevan(vch);
    }
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, type=%i, vtx=%u, vchBlockSig=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        GetAlgo(nVersion) == -1 ? IsProofOfWork() : GetAlgo(nVersion),
        vtx.size(),
        vchBlockSig.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}

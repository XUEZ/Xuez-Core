// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2018-2020 John "ComputerCraftr" Studnicka
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
//#include <logging.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

#include <mutex>

RecursiveMutex cs_target_cache;

// peercoin: find last block index up to pindex
static inline const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, const bool fProofOfStake)
{
    while (pindex && pindex->pprev && pindex->IsProofOfStake() != fProofOfStake)
        pindex = pindex->pprev;
    return pindex;
}

static inline const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, const int& algo)
{
    while (pindex && pindex->pprev && CBlockHeader::GetAlgo(pindex->nVersion) != algo)
        pindex = pindex->pprev;
    return pindex;
}

static inline const CBlockIndex* GetASERTReferenceBlockAndHeightForAlgo(const CBlockIndex* pindex, const uint32_t& nProofOfWorkLimit, const int& nASERTStartHeight, const int& algo, uint32_t& nBlocksPassed)
{
    nBlocksPassed = 1; // Account for the ASERT reference block here
    while (pindex && pindex->pprev && pindex->nHeight >= nASERTStartHeight) {
        const CBlockIndex* pprev = GetLastBlockIndexForAlgo(pindex->pprev, algo);
        if (pprev)
            pindex = pprev;
        else
            break;

        //if (pindex->nBits != (nProofOfWorkLimit - 1))
            nBlocksPassed++;
    }
    return pindex;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int algo = CBlockHeader::GetAlgo(pblock->nVersion);
    const bool fProofOfStake = pblock->IsProofOfStake();
    const uint32_t nProofOfWorkLimit = UintToArith256(params.powLimit[algo == -1 ? CBlockHeader::ALGO_POW_XEVAN : algo]).GetCompact();
    if (pindexLast == nullptr || params.fPowNoRetargeting)
        return nProofOfWorkLimit;
    const uint32_t nHeight = pindexLast->nHeight + 1;

    if (pblock->nVersion >= CBlockHeader::FIRST_FORK_VERSION && params.fPowAllowMinDifficultyBlocks && algo != -1) {
        // Special difficulty rule:
        // If the new block's timestamp is more than 30 minutes (be careful to ensure this is at least twice the actual PoW target spacing to avoid interfering with retargeting)
        // then allow mining of a min-difficulty block.
        const CBlockIndex* pindexPrev = GetLastBlockIndexForAlgo(pindexLast, algo);
        if (pindexPrev->nHeight > 10 && pblock->GetBlockTime() > pindexPrev->GetBlockTime() + (30*60))
            return (nProofOfWorkLimit - 1);
        if (pindexPrev->pprev && pindexPrev->nBits == (nProofOfWorkLimit - 1)) {
            // Return the block before the last non-special-min-difficulty-rules-block
            const CBlockIndex* pindex = pindexPrev;
            while (pindex->pprev && (pindex->nBits == (nProofOfWorkLimit - 1) || CBlockHeader::GetAlgo(pindex->nVersion) != algo))
                pindex = pindex->pprev;
            const CBlockIndex* pprev = GetLastBlockIndexForAlgo(pindex->pprev, algo);
            if (pprev && pprev->nHeight > 10) {
                // Don't return pprev->nBits if it is another min-difficulty block; instead return pindex->nBits
                if (pprev->nBits != (nProofOfWorkLimit - 1))
                    return pprev->nBits;
                else
                    return pindex->nBits;
            }
        }
    }

    if (nHeight >= (unsigned)params.nMandatoryUpgradeBlock) {
        return AverageTargetASERT(pindexLast, pblock, params);
    } else {
        if (fProofOfStake) {
            // Account for a mistake which caused the previous PoW difficulty calculation to be used on the first PoS block
            if (nHeight == 43201 && pblock->nVersion == 4 && pblock->nTime == 1525809739 && fProofOfStake && algo == -1 && pindexLast->GetBlockHash() == uint256S("0x00000001697a06cf5e6f8a067770fa42f15218ca0d7637e4d3078c7857e3512a")) return 0x1d044309;
            if (nHeight == 43202 && pblock->nVersion == 4 && pblock->nTime == 1525809751 && fProofOfStake && algo == -1 && pindexLast->GetBlockHash() == uint256S("0x0fbe72517981d3fd9defebf0ba252ff744f44fdb0b73f1403d9a8de9325c7ae3")) return 0x1d04369d;

            return PeercoinWTEMA(pindexLast, pblock, params);
        } else {
            return SimpleMovingAverageTarget(pindexLast, pblock, params);
        }
    }
}

unsigned int GetNextWorkRequiredXUEZ(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit[CBlockHeader::ALGO_POW_XEVAN]).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit[CBlockHeader::ALGO_POW_XEVAN]);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

unsigned int WeightedTargetExponentialMovingAverage(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int algo = CBlockHeader::GetAlgo(pblock->nVersion);
    const bool fProofOfStake = pblock->IsProofOfStake();
    const arith_uint256 bnPowLimit = algo == -1 ? UintToArith256(params.powLimit[fProofOfStake ? CBlockHeader::ALGO_POS : CBlockHeader::ALGO_POW_XEVAN]) : UintToArith256(params.powLimit[algo]);
    const uint32_t nProofOfWorkLimit = bnPowLimit.GetCompact();
    if (pindexLast == nullptr)
        return nProofOfWorkLimit; // genesis block

    const CBlockIndex* pindexPrev = algo == -1 ? GetLastBlockIndex(pindexLast, fProofOfStake) : GetLastBlockIndexForAlgo(pindexLast, algo);
    if (pindexPrev->pprev == nullptr)
        return nProofOfWorkLimit; // first block

    const CBlockIndex* pindexPrevPrev = algo == -1 ? GetLastBlockIndex(pindexPrev->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindexPrev->pprev, algo);
    if (pindexPrevPrev->pprev == nullptr)
        return nProofOfWorkLimit; // second block

    int nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime(); // Difficulty for PoW and PoS are calculated separately

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int nTargetSpacing = params.nPowTargetSpacing;
    const uint32_t nTargetTimespan = params.nPowTargetTimespan;
    //nTargetSpacing *= 2; // 160 second block time for PoW + 160 second block time for PoS = 80 second effective block time
    if (!fProofOfStake) {
        //nTargetSpacing *= (CBlockHeader::ALGO_COUNT - 1); // Multiply by the number of PoW algos
        nTargetSpacing = 10 * 60; // PoW spacing is 10 minutes
    }
    const int nInterval = nTargetTimespan / (nTargetSpacing * 2); // alpha_reciprocal = (N(SMA) + 1) / 2 for same "center of mass" as SMA

    // nActualSpacing must be restricted as to not produce a negative number below
    if (nActualSpacing <= -((nInterval - 1) * nTargetSpacing))
        nActualSpacing = -((nInterval - 1) * nTargetSpacing) + 1;

    const uint32_t numerator = (nInterval - 1) * nTargetSpacing + nActualSpacing;
    const uint32_t denominator = nInterval * nTargetSpacing;

    // Keep in mind the order of operations and integer division here - this is why the *= operator cannot be used, as it could cause overflow or integer division to occur
    arith_uint512 bnNew512 = arith_uint512(bnNew) * numerator / denominator; // For WTEMA: next_target = prev_target * (nInterval - 1 + prev_solvetime/target_solvetime) / nInterval
    bnNew = bnNew512.trim256();

    if (bnNew512 > arith_uint512(bnPowLimit) || bnNew == arith_uint256())
        bnNew = bnPowLimit;

    return bnNew.GetCompactRounded();
}

unsigned int PeercoinWTEMA(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int algo = CBlockHeader::GetAlgo(pblock->nVersion);
    const bool fProofOfStake = pblock->IsProofOfStake();
    const arith_uint256 bnPowLimit = algo == -1 ? UintToArith256(params.powLimit[fProofOfStake ? CBlockHeader::ALGO_POS : CBlockHeader::ALGO_POW_XEVAN]) : UintToArith256(params.powLimit[algo]);
    const uint32_t nProofOfWorkLimit = bnPowLimit.GetCompact();
    const bool fPostUpgrade = pblock->nVersion >= CBlockHeader::FIRST_FORK_VERSION;
    if (pindexLast == nullptr)
        return nProofOfWorkLimit; // genesis block

    const CBlockIndex* pindexPrev = algo == -1 ? GetLastBlockIndex(pindexLast, fProofOfStake) : GetLastBlockIndexForAlgo(pindexLast, algo);
    if (pindexPrev->pprev == nullptr)
        return nProofOfWorkLimit; // first block

    const CBlockIndex* pindexPrevPrev = algo == -1 ? GetLastBlockIndex(pindexPrev->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindexPrev->pprev, algo);
    if (pindexPrevPrev->pprev == nullptr)
        return nProofOfWorkLimit; // second block

    int nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime(); // Difficulty for PoW and PoS are calculated separately

    // peercoin: target change every block
    // peercoin: retarget with exponential moving toward target spacing
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int nTargetSpacing = params.nPowTargetSpacing;
    uint32_t nTargetTimespan = params.nPowTargetTimespan;
    int nInterval = params.DifficultyAdjustmentInterval(); // alpha_reciprocal = (N(SMA) + 1) / 2 for same "center of mass" as SMA

    // Previous blunders with difficulty calculations follow...
    if (!fPostUpgrade) {
        nTargetSpacing = 60;
        nTargetTimespan = 40 * 60;
        nInterval = nTargetTimespan / nTargetSpacing;

        // Limiting the solvetime and how much the difficulty can rise here allows attackers to drop the difficulty to zero using timestamps in the past
        if (nActualSpacing < 0)
            nActualSpacing = 1;
    } else { // Now that we got that over with, we can do it the right way here
        //nTargetSpacing *= 2; // 160 second block time for PoW + 160 second block time for PoS = 80 second effective block time
        if (!fProofOfStake) {
            //nTargetSpacing *= (CBlockHeader::ALGO_COUNT - 1); // Multiply by the number of PoW algos
            nTargetSpacing = 10 * 60; // PoW spacing is 10 minutes
        }

        nInterval = nTargetTimespan / nTargetSpacing; // Update nInterval with new nTargetSpacing
    }

    // If nActualSpacing is very, very negative (close to but greater than -nTargetTimespan/2), it would cause the difficulty calculation to result
    // in zero or a negative number, so we have no choice but to limit the solvetime to the lowest value this method can handle. Ideally, this if
    // statement would become impossible to trigger by requiring sequential timestamps or MTP enforcement and a large enough nTargetTimespan. There
    // may be a way to extend this calculation to appropriately handle even lower negative numbers, but the difficulty already increases significantly
    // for small negative solvetimes and the next solvetime would have to be many times larger than this negative value in order to simply return
    // to the same difficulty as before, which can be modeled by the following equation where x is previous solvetime and f(x) is the next solvetime:
    // f(x) = ((nInterval + 1) * nTargetSpacing / 2)^2 / ((nInterval - 1) * nTargetSpacing / 2 + x) - ((nInterval - 1) * nTargetSpacing / 2)
    // The functionality of this if statement has been moved below directly into the calculation of the numerator with the call to std::max
    //if (nActualSpacing <= -((nInterval - 1) * nTargetSpacing / 2))
        //nActualSpacing = -((nInterval - 1) * nTargetSpacing / 2) + 1;

    // This is a linear equation used to adjust the next difficulty target based on the previous solvetime only (no averaging is used). On SPL, it
    // simplifies to f(x) = (x + 3560) / 3640 where x is nActualSpacing and bnNew is directly multiplied by f(x) to calculate the next difficulty
    // target. The equation is equal to 1 when x is 80 and the y-intercept of 3560 / 3640 is the result that we would arrive at from a solvetime of
    // zero, but the x-intercept at -3560 poses several problems for the difficulty calculation, as the target cannot be zero or a negative number.
    // This forces us to impose a restriction on solvetime such that x > -3560 which is an asymmetric limit on how much the difficulty can rise, but
    // enforcing sequential timestamps and our strict future time limit effectively mitigates the risk of this being exploited against us. The issue
    // is much more pronounced when using faster responding difficulty calculations, as the x-intercept for this function when we were using the 20
    // minute nTargetTimespan was only -560, and a block 10 minutes in the past would have already bumped into the solvetime limit and could be used
    // to lower the difficulty. Increasing nTargetTimespan or decreasing nTargetSpacing lowers the x-intercept farther in order to handle out of order
    // timestamps better, but this also slows the response time of the difficulty adjustment algorithm and makes it more stable. The first derivative
    // of f(x) (or simply the slope of this linear equation) is what determines how quickly the difficulty adjustment responds to changes in solvetimes,
    // with smaller derivatives corresponding to slower responding difficulty calculations. The derivative of our current equation is 1 / 3640 while
    // the previous equation with the 20 minute nTargetTimespan had a derivative of 1 / 640.
    const uint32_t numerator = std::max((nInterval - 1) * nTargetSpacing + 2 * nActualSpacing, 1);
    const uint32_t denominator = (nInterval + 1) * nTargetSpacing;

    // Keep in mind the order of operations and integer division here - this is why the *= operator cannot be used, as it could cause overflow or integer division to occur
    arith_uint512 bnNew512 = arith_uint512(bnNew) * numerator / denominator; // For peercoin: next_target = prev_target * (nInterval - 1 + 2 * prev_solvetime/target_solvetime) / (nInterval + 1)

    // Some algorithms were affected by the arith_uint256 overflow bug while calculating difficulty, so we need to use the old formula here
    //if (!fPostUpgrade && (algo == CBlockHeader::ALGO_POW_QUARK || algo == CBlockHeader::ALGO_POW_SCRYPT_SQUARED))
        //bnNew = bnNew * numerator / denominator;
    //else
        bnNew = bnNew512.trim256();

    if (bnNew > bnPowLimit || bnNew512 > arith_uint512(bnPowLimit) || bnNew == arith_uint256())
        bnNew = bnPowLimit;

    return fPostUpgrade ? bnNew.GetCompactRounded() : bnNew.GetCompact();
}

unsigned int AverageTargetASERT(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int algo = CBlockHeader::GetAlgo(pblock->nVersion);
    const bool fProofOfStake = pblock->IsProofOfStake();
    const arith_uint256 bnPowLimit = algo == -1 ? UintToArith256(params.powLimit[fProofOfStake ? CBlockHeader::ALGO_POS : CBlockHeader::ALGO_POW_XEVAN]) : UintToArith256(params.powLimit[algo]);
    const uint32_t nProofOfWorkLimit = bnPowLimit.GetCompact();
    uint32_t nTargetSpacing = params.nPowTargetSpacing;
    //nTargetSpacing *= 2; // 160 second block time for PoW + 160 second block time for PoS = 80 second effective block time
    if (!fProofOfStake) {
        //nTargetSpacing *= (CBlockHeader::ALGO_COUNT - 1); // Multiply by the number of PoW algos
        nTargetSpacing = 10 * 60; // PoW spacing is 10 minutes
    }

    if (pindexLast == nullptr)
        return nProofOfWorkLimit; // genesis block

    const CBlockIndex* pindexPrev = algo == -1 ? GetLastBlockIndex(pindexLast, fProofOfStake) : GetLastBlockIndexForAlgo(pindexLast, algo);
    if (pindexPrev->pprev == nullptr)
        return nProofOfWorkLimit; // first block

    const CBlockIndex* pindexPrevPrev = algo == -1 ? GetLastBlockIndex(pindexPrev->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindexPrev->pprev, algo);
    if (pindexPrevPrev->pprev == nullptr)
        return nProofOfWorkLimit; // second block

    const uint32_t nASERTStartHeight = params.nMandatoryUpgradeBlock + 10;
    // In the future, it may be a good idea to switch this from height based to a fixed time window
    const uint32_t nASERTBlockTargetsToAverage = 4 * params.nPowTargetTimespan / nTargetSpacing; // Average the past 2 days' worth of block targets

    const uint32_t nHeight = pindexLast->nHeight + 1;
    if (nHeight < nASERTStartHeight)
        return WeightedTargetExponentialMovingAverage(pindexLast, pblock, params);

    uint32_t nBlocksPassed = 0;
    const CBlockIndex* pindexReferenceBlock = GetASERTReferenceBlockAndHeightForAlgo(pindexPrev, nProofOfWorkLimit, nASERTStartHeight, algo, nBlocksPassed);
    const CBlockIndex* pindexReferenceBlockPrev = algo == -1 ? GetLastBlockIndex(pindexReferenceBlock->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindexReferenceBlock->pprev, algo);
    // Use reference block's parent block's timestamp unless it is the genesis (not using the prev timestamp here would put us permanently one block behind schedule)
    const int64_t nTimeDiff = pindexPrev->GetBlockTime() - (pindexReferenceBlockPrev ? pindexReferenceBlockPrev->GetBlockTime() : (pindexReferenceBlock->GetBlockTime() - nTargetSpacing));
    //LogPrintf("pindexReferenceBlock->GetBlockHash() = %s\n", pindexReferenceBlock->GetBlockHash().ToString().c_str());
    //LogPrintf("nBlocksPassed = %u\n", nBlocksPassed);
    //LogPrintf("pindexPrev->nHeight + 1 - pindexReferenceBlock->nHeight = %u\n", pindexPrev->nHeight + 1 - pindexReferenceBlock->nHeight);
    //assert(nBlocksPassed == (uint32_t)pindexPrev->nHeight + 1 - pindexReferenceBlock->nHeight);
    const uint32_t nHeightDiff = nBlocksPassed; //pindexPrev->nHeight + 1 - pindexReferenceBlock->nHeight;
    arith_uint256 refBlockTarget;

    // We don't want to recalculate the average of several days' worth of block targets here every single time, so instead we cache the average and start height
    const bool fUseCache = true;
    {
        LOCK(cs_target_cache);

        static arith_uint256 refBlockTargetCache;
        static int nTargetCacheHeight = -1;
        static int nTargetCacheAlgo = CBlockHeader::ALGO_COUNT;

        if (nASERTBlockTargetsToAverage > 0 && nHeight >= nASERTStartHeight + nASERTBlockTargetsToAverage && nHeightDiff >= nASERTBlockTargetsToAverage) {
            if (!fUseCache || nTargetCacheHeight != (int)(nHeightDiff / nASERTBlockTargetsToAverage) || nTargetCacheAlgo != algo || refBlockTargetCache == arith_uint256() || algo == -1) {
                const uint32_t nBlocksToSkip = nHeightDiff % nASERTBlockTargetsToAverage;
                const CBlockIndex* pindex = pindexPrev;
                //LogPrintf("nBlocksToSkip = %u\n", nBlocksToSkip);

                for (unsigned int i = 0; i < nBlocksToSkip; i++) {
                    pindex = algo == -1 ? GetLastBlockIndex(pindex->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindex->pprev, algo);
                }
                //LogPrintf("begin pindex->nHeight = %i\n", pindex->nHeight);

                //unsigned int nBlocksAveraged = 0;
                for (int i = 0; i < (int)nASERTBlockTargetsToAverage; i++) {
                    if (pindex->nBits != (nProofOfWorkLimit - 1) || !params.fPowAllowMinDifficultyBlocks) { // Don't add min difficulty targets to the average
                        arith_uint256 bnTarget = arith_uint256().SetCompact(pindex->nBits);
                        refBlockTarget += bnTarget / nASERTBlockTargetsToAverage;
                        //nBlocksAveraged++;
                        //if (pindex->GetBlockHash() == params.hashGenesisBlock)
                            //LogPrintf("Averaging genesis block target\n");
                    } else
                        i--; // Average one more block to make up for the one we skipped
                    pindex = algo == -1 ? GetLastBlockIndex(pindex->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindex->pprev, algo);
                    if (!pindex)
                        break;
                }
                //LogPrintf("nBlocksAveraged = %u\n", nBlocksAveraged);
                //assert(nBlocksAveraged == nASERTBlockTargetsToAverage);
                //if (pindex)
                    //LogPrintf("end pindex->nHeight = %i\n", pindex->nHeight);
                if (fUseCache) {
                    refBlockTargetCache = refBlockTarget;
                    nTargetCacheHeight = nHeightDiff / nASERTBlockTargetsToAverage;
                    nTargetCacheAlgo = algo;
                    //LogPrintf("Set average target cache at nHeight = %u with algo = %i\n", nHeight, algo);
                }
            } else {
                refBlockTarget = refBlockTargetCache;
                //LogPrintf("Using average target cache at nHeight = %u with algo = %i\n", nHeight, algo);
            }
        } else {
            if (fUseCache && algo != -1) {
                if (nTargetCacheHeight != -1 || nTargetCacheAlgo != algo || refBlockTargetCache == arith_uint256()) {
                    refBlockTargetCache = arith_uint256().SetCompact(pindexReferenceBlock->nBits);
                    nTargetCacheHeight = -1;
                    nTargetCacheAlgo = algo;
                    //LogPrintf("Set ref target cache at nHeight = %u with algo = %i\n", nHeight, algo);
                }
                refBlockTarget = refBlockTargetCache;
            } else
                refBlockTarget = arith_uint256().SetCompact(pindexReferenceBlock->nBits);
        }
    }

    //LogPrintf("nHeight = %u, algo = %i, refBlockTarget = %s\n", nHeight, algo, refBlockTarget.ToString().c_str());
    arith_uint256 bnNew(refBlockTarget);
    const int64_t dividend = nTimeDiff - nTargetSpacing * nHeightDiff;
    const bool fPositive = dividend >= 0;
    const uint32_t divisor = params.nPowTargetTimespan; // Must be positive
    const int exponent = dividend / divisor; // Note: this integer division rounds down positive and rounds up negative numbers via truncation, but the truncated fractional part is handled by the approximation below
    const uint32_t remainder = fPositive ? dividend % divisor : -dividend % divisor; // Must be positive
    // We are using uint512 rather than uint64_t here because a nPowTargetTimespan of more than 3 days in the divisor may cause the following cubic approximation to overflow a uint64_t
    arith_uint512 numerator = 1;
    arith_uint512 denominator = 1;
    // Alternatively, ensure that the nPowTargetTimespan (divisor) is small enough such that (2*2*2) * (4 + 11 + 35 + 50) * (divisor)^3 < (2^64 - 1) which is the uint64_t maximum value to leave some room and make overflow extremely unlikely
    //assert(divisor <= (3 * 24 * 60 * 60));
    // Keep in mind that a large exponent due to being extremely far ahead or behind schedule, especially in case of reviving an abandoned chain, will also lead to overflowing the numerator, so a less accurate approximation should be used in this case
    //uint64_t numerator = 1;
    //uint64_t denominator = 1;

    //LogPrintf("nHeight = %u, algo = %i is %s schedule, nTimeDiff = %li, ideal = %u, exponent = %i\n", nHeight, algo, fPositive ? "behind" : "ahead of", nTimeDiff, nTargetSpacing * nHeightDiff, exponent);
    if (fPositive) {
        for (int i = 0; i < exponent; i++)
            numerator *= 2;

        if (remainder != 0) { // Approximate 2^x with (4x^3+11x^2+35x+50)/50 for 0<x<1 (must be equal to 1 at x=0 and equal to 2 at x=1 to avoid discontinuities) - note: x+1 and (3x^2+7x+10)/10 are also decent and less complicated approximations
            //numerator *= remainder + divisor; // x+1
            //denominator *= divisor;

            const arith_uint512 bnDivisor(divisor);
            const arith_uint512 bnRemainder(remainder);
            numerator = numerator * ((4 * bnRemainder*bnRemainder*bnRemainder) + (11 * bnRemainder*bnRemainder * bnDivisor) + (35 * bnRemainder * bnDivisor*bnDivisor) + (50 * bnDivisor*bnDivisor*bnDivisor));
            denominator = denominator * (50 * bnDivisor*bnDivisor*bnDivisor);
            //numerator = numerator * ((4lu * remainder*remainder*remainder) + (11lu * remainder*remainder * divisor) + (35lu * remainder * divisor*divisor) + (50lu * divisor*divisor*divisor));
            //denominator = denominator * (50lu * divisor*divisor*divisor);
        }
    } else {
        for (int i = 0; i > exponent; i--)
            denominator *= 2;

        if (remainder != 0) { // Approximate 2^x with (4x^3+11x^2+35x+50)/50 for 0<x<1 (must be equal to 1 at x=0 and equal to 2 at x=1 to avoid discontinuities) - note: x+1 and (3x^2+7x+10)/10 are also decent and less complicated approximations
            //numerator *= divisor;
            //denominator *= remainder + divisor; // x+1

            const arith_uint512 bnDivisor(divisor);
            const arith_uint512 bnRemainder(remainder);
            numerator = numerator * (50 * bnDivisor*bnDivisor*bnDivisor);
            denominator = denominator * ((4 * bnRemainder*bnRemainder*bnRemainder) + (11 * bnRemainder*bnRemainder * bnDivisor) + (35 * bnRemainder * bnDivisor*bnDivisor) + (50 * bnDivisor*bnDivisor*bnDivisor));
            //numerator = numerator * (50lu * divisor*divisor*divisor);
            //denominator = denominator * ((4lu * remainder*remainder*remainder) + (11lu * remainder*remainder * divisor) + (35lu * remainder * divisor*divisor) + (50lu * divisor*divisor*divisor));
        }
    }

    // Keep in mind the order of operations and integer division here - this is why the *= operator cannot be used, as it could cause overflow or integer division to occur
    arith_uint512 bnNew512 = arith_uint512(bnNew) * arith_uint512(numerator) / arith_uint512(denominator);
    bnNew = bnNew512.trim256();

    //LogPrintf("numerator = %s\n", numerator.ToString().c_str());
    //LogPrintf("denominator = %s\n", denominator.ToString().c_str());
    //LogPrintf("numerator = %lu\n", numerator);
    //LogPrintf("denominator = %lu\n", denominator);
    //LogPrintf("10000 * 2^(%li/%u) = %s\n", dividend, divisor, arith_uint512((10000 * arith_uint512(numerator)) / arith_uint512(denominator)).trim256().ToString().c_str());
    if (bnNew512 > arith_uint512(bnPowLimit) || bnNew == arith_uint256())
        bnNew = bnPowLimit;

    return bnNew.GetCompactRounded();
}

unsigned int SimpleMovingAverageTarget(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const int algo = CBlockHeader::GetAlgo(pblock->nVersion);
    const bool fProofOfStake = pblock->IsProofOfStake();
    const arith_uint256 bnPowLimit = algo == -1 ? UintToArith256(params.powLimit[fProofOfStake ? CBlockHeader::ALGO_POS : CBlockHeader::ALGO_POW_XEVAN]) : UintToArith256(params.powLimit[algo]);
    const uint32_t nProofOfWorkLimit = bnPowLimit.GetCompact();
    const bool fPostUpgrade = pblock->nVersion >= CBlockHeader::FIRST_FORK_VERSION;
    uint32_t nTargetSpacing = fPostUpgrade ? params.nPowTargetSpacing : 60;
    //nTargetSpacing *= 2; // 160 second block time for PoW + 160 second block time for PoS = 80 second effective block time
    if (!fProofOfStake && fPostUpgrade) {
        //nTargetSpacing *= (CBlockHeader::ALGO_COUNT - 1); // Multiply by the number of PoW algos
        nTargetSpacing = 10 * 60; // PoW spacing is 10 minutes
    }

    bool fUseTempering = fPostUpgrade; // true = DigiShield, false = Dark Gravity Wave
    const uint32_t nTemperingFactor = 4;
    int nPastBlocks = fPostUpgrade ? params.nPowTargetTimespan / nTargetSpacing : 24; // DGW default of 24 - needs to be more than 6 for MTP=11 enforcement to ensure that nActualTimespan will always be positive (or require sequential timestamps)
    if (fUseTempering)
        nPastBlocks /= nTemperingFactor; // DigiShield averages fewer blocks in order to respond faster but uses tempering to maintain similar stability

    // nFirstWeightMultiplier can be calculated using the formula (nPastBlocks * x) / (1 - x) + 1 where x = 1/3 (configurable) in order to give 33.3% of the overall weight to the most recent target
    const uint32_t nFirstWeightMultiplier = fPostUpgrade ? 1 : 2; // DGW default of 2 to put double weight on the most recent element in the average - set equal to 1 for normal SMA behavior
    if (pindexLast == nullptr)
        return nProofOfWorkLimit; // genesis block

    const CBlockIndex* pindexPrev = algo == -1 ? GetLastBlockIndex(pindexLast, fProofOfStake) : GetLastBlockIndexForAlgo(pindexLast, algo);
    if (pindexPrev->pprev == nullptr)
        return nProofOfWorkLimit; // first block

    const CBlockIndex* pindexPrevPrev = algo == -1 ? GetLastBlockIndex(pindexPrev->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindexPrev->pprev, algo);
    if (pindexPrevPrev->pprev == nullptr)
        return nProofOfWorkLimit; // second block

    //nPastBlocks = std::min(nPastBlocks, pindexLast->nHeight);
    if (pindexLast->nHeight < nPastBlocks + (fPostUpgrade ? 2 : 0)) // We are adding 2 here to skip the first two blocks at nProofOfWorkLimit, but it isn't necessary to do this for the average to work
        return fPostUpgrade ? WeightedTargetExponentialMovingAverage(pindexLast, pblock, params) : nProofOfWorkLimit;

    const CBlockIndex* pindex = pindexPrev;
    arith_uint256 bnPastTargetAvg;

    // This is a simple moving average of difficulty targets with double weight for the most recent target by default (same as a harmonic SMA of difficulties)
    // (2 * T1 + T2 + T3 + T4 + ... + T24) / (24 + 1)
    for (int nCountBlocks = 1; nCountBlocks <= nPastBlocks; nCountBlocks++) {
        if (pindex->nBits != (nProofOfWorkLimit - 1) || !params.fPowAllowMinDifficultyBlocks) { // Don't add min difficulty targets to the average
            arith_uint256 bnTarget = arith_uint256().SetCompact(pindex->nBits);
            if (nCountBlocks == 1)
                bnTarget *= nFirstWeightMultiplier;
            bnPastTargetAvg += bnTarget / (nPastBlocks + nFirstWeightMultiplier - 1); // Number of elements added to average is nFirstWeightMultiplier - 1
        } else
            nCountBlocks--; // Average one more block to make up for the one we skipped

        const CBlockIndex* pprev = algo == -1 ? GetLastBlockIndex(pindex->pprev, fProofOfStake) : GetLastBlockIndexForAlgo(pindex->pprev, algo);
        // If we skip the last index here, it causes nActualTimespan to be calculated with one less timestamp than it is supposed to use
        if (pprev && pprev->nHeight != 0 && (fPostUpgrade || nCountBlocks != nPastBlocks))
            pindex = pprev;
        else
            break;
    }

    if (bnPastTargetAvg == arith_uint256())
        bnPastTargetAvg = bnPowLimit;
    arith_uint256 bnNew(bnPastTargetAvg);

    // If pprev was nullptr, nActualTimespan will use one less than nPastBlocks timestamps, which causes difficulty to be slightly higher than expected
    int nActualTimespan = pindexPrev->GetBlockTime() - pindex->GetBlockTime(); // Dark Gravity Wave
    int nTargetTimespan = nPastBlocks * nTargetSpacing;
    // Respond faster by avoiding tempering when the average solvetime is at least 15% too low or too high
    // WARNING: The following code will cause oscillations in difficulty if the max error percentage is set too low due to undershoot/overshoot in difficulty target
    const int nMaxSolvetimeErrorPercentage = 15;
    if (nActualTimespan <= (nTargetTimespan * (100 - nMaxSolvetimeErrorPercentage)) / 100 || nActualTimespan >= (nTargetTimespan * (100 + nMaxSolvetimeErrorPercentage)) / 100)
        fUseTempering = false;

    // Note we did not use MTP to calculate nActualTimespan here, which enables the time warp attack to drop the difficulty to zero using timestamps in the past due to the timespan limit below
    if (fUseTempering) { // DigiShield
        nActualTimespan += (nTemperingFactor - 1) * nTargetTimespan; // Temper nActualTimespan with the formula (3 * nTargetTimespan + nActualTimespan) / 4
        nTargetTimespan *= nTemperingFactor; // We multiply by 4 here in order to divide by 4 in the final calculation
    }

    // We have no choice but to limit the timespan here in case the calculation resulted in zero or a negative number, but it shouldn't be possible to reach this while requiring sequential timestamps or MTP enforcement
    if (nActualTimespan < 1)
        nActualTimespan = 1;

    // Keep in mind the order of operations and integer division here - this is why the *= operator cannot be used, as it could cause overflow or integer division to occur
    arith_uint512 bnNew512 = arith_uint512(bnNew) * nActualTimespan / nTargetTimespan; // next_target = avg(nPastBlocks prev_targets) * (nTemperingFactor - 1 + avg(nPastBlocks prev_solvetimes)/target_solvetime) / nTemperingFactor
    bnNew = bnNew512.trim256();

    if (bnNew512 > arith_uint512(bnPowLimit) || bnNew == arith_uint256())
        bnNew = bnPowLimit;

    return fPostUpgrade ? bnNew.GetCompactRounded() : bnNew.GetCompact();
}

bool CheckProofOfWork(const uint256& hash, const unsigned int& nBits, const int& algo, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || algo < -1 || algo == CBlockHeader::ALGO_POS || algo >= CBlockHeader::ALGO_COUNT || bnTarget > UintToArith256(params.powLimit[algo == -1 ? CBlockHeader::ALGO_POW_XEVAN : algo]))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

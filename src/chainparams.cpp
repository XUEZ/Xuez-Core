// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <arith_uint256.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h> // for signet block challenge hash
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    if (genesisReward > 0)
        txNew.vin[0].scriptSig = CScript() << OP_0 << nBits << OP_4 << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    else
        txNew.vin[0].scriptSig = CScript() << 0x1d00ffff << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    arith_uint256 hashTarget = arith_uint256().SetCompact(std::min(genesis.nBits, (unsigned)0x1f00ffff));
    /*while (true) {
        arith_uint256 hash = UintToArith256(genesis.GetPoWHash());
        if (hash <= hashTarget) {
            // Found a solution
            printf("genesis block found\n   hash: %s\n target: %s\n   bits: %08x\n  nonce: %u\n", hash.ToString().c_str(), hashTarget.ToString().c_str(), genesis.nBits, genesis.nNonce);
            break;
        }
        genesis.nNonce += 1;
        if ((genesis.nNonce & 0x1ffff) == 0)
            printf("testing nonce: %u\n", genesis.nNonce);
    }*/
    uint256 hash = genesis.GetPoWHash();
    assert(UintToArith256(hash) <= hashTarget);

    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Museum of Fine Arts, Boston MA, is one of the most comprehensive art museums";
    const CScript genesisOutputScript = genesisReward > 0 ? CScript() << OP_0 << ParseHex("1d25c8716a4513320d1c26b3fbd3d2f6dfe3fc37") : CScript() << ParseHex("041e2be616cdaadb196b3d15c9587dbf0f5803c04c1df97cc01699cbb7462b90d9efee0d4c4b45b63419d83013c33fa3710cd71f0e0b8f4232ba76877ecb08c847") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = 100000;
        consensus.nMandatoryUpgradeBlock = 1442800;
        consensus.nTreasuryPaymentsStartBlock = consensus.nMandatoryUpgradeBlock;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = consensus.nMandatoryUpgradeBlock;
        consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // TODO: set to hash of fork block (at BIP34Height aka nMandatoryUpgradeBlock) to bypass slow BIP30 checking on new blocks
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0; // segwit activation height + miner confirmation window
        consensus.powLimit[CBlockHeader::ALGO_POS] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000"); // 0x1e00ffff
        consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN] = uint256S("00000fffff000000000000000000000000000000000000000000000000000000"); // 0x1e0fffff
        consensus.nPowTargetTimespan = 12 * 60 * 60; // 12 hours
        consensus.nPowTargetSpacing = 80; // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0xf; // 16 second time slots
        consensus.nStakeMinDepth[0] = 10;
        consensus.nStakeMinDepth[1] = 600;
        consensus.nStakeMinAge[0] = 6 * 60 * 60; // previous min age was 6 hours
        consensus.nStakeMinAge[1] = 12 * 60 * 60; // current minimum age for coin age is 12 hours
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60; // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = (14 * 24 * 60 * 60 * 95) / (100 * consensus.nPowTargetSpacing); // 95% of the blocks in the past two weeks
        consensus.nMinerConfirmationWindow = 14 * 24 * 60 * 60 / consensus.nPowTargetSpacing; // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTreasuryPaymentsCycleBlocks = 1 * 24 * 60 * 60 / consensus.nPowTargetSpacing; // Once per day
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("b9b44fa969c0a95dfa89a553cd6adb7a59e49ad1"), 100); // 3% (full reward) for xz1qhx6yl2tfcz54m75f54fu66km0fv7fxk30pr3p0
        consensus.nTreasuryRewardPercentage = 3; // 3% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000253bbaf1090c8a9fc");
        consensus.defaultAssumeValid = uint256S("0x5b85bce8fa177da2ed744a3fbe35bb4e5e1ff319c110b17ec0715e6d3b1ca8de"); // 1400000

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xe5;
        pchMessageStart[1] = 0x5a;
        pchMessageStart[2] = 0xc4;
        pchMessageStart[3] = 0x54;
        nDefaultPort = 41798;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 5;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1523045620, 20710639, 0x1e0ffff0, 1, 0 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash mainnet: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash mainnet: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0x9df20f860896e0e7d9ecc5f490f2d5fd01715553db7000b2d1d3497322400be0"));
        assert(consensus.hashGenesisBlock == uint256S("0x000000e1febc39965b055e8e0117179a4d18e24e7aaa0c69864c4054b4f29445"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("xuez.donkeypool.com");
        vSeeds.emplace_back("seed1.xuezcoin.com");
        vSeeds.emplace_back("seed2.xuezcoin.com");
        vSeeds.emplace_back("seed3.xuezcoin.com");
        vSeeds.emplace_back("seed4.xuezcoin.com");
        vSeeds.emplace_back("seed5.xuezcoin.com");
        vSeeds.emplace_back("seed6.xuezcoin.com");
        vSeeds.emplace_back("seed7.xuezcoin.com");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,75);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,18);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,212);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "xz";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 0, uint256S("0x000000e1febc39965b055e8e0117179a4d18e24e7aaa0c69864c4054b4f29445")},
                { 50000, uint256S("0x90b0eedf028c5045c8332e979c87f820cf2aceb551103dcaa6b529fa7df35769")},
                { 100000, uint256S("0x258afd3da5d1aab779a0b1cc647947f07db8b42febc49e0304bdd7fa963b11c2")},
                { 150000, uint256S("0x48824baf791db8ef6cde3444fb8f5717f37181eeac9baffb80e6dcc81b03b2b5")},
                { 200000, uint256S("0x0a2f3295ea391f00d2f2b4d0aac05627b4ba5ec6df3b9822e42a4ee53ecb5a6f")},
                { 250000, uint256S("0xdf9d262a26ff1d6f7f566bfa1d0fddeeb3d2b9e3d805eb836b7b3e30b3e03406")},
                { 300000, uint256S("0xba713d739638803b8a051997018f8245555b2fcfe13df792b3e94f5004f1245f")},
                { 350000, uint256S("0x02975b21a74caafe3cc1212fa54c82ccd2360ff4c7c40ba183938b30d6f8adc9")},
                { 400000, uint256S("0xcbca0219d49932c2f56d52b7bc0b64c74a7c2aeeea77988cf4f6309d142e8a92")},
                { 450000, uint256S("0x565cde9423ef8450f3fa2b350de41cc9c504f8da516721c959e6b68252603203")},
                { 500000, uint256S("0xbf855f07b50393d79b1a8c53bae805cfe7f34b53c3d77879eda67844066bfff5")},
                { 550000, uint256S("0x9a85bd9bd45ceabba48ca619a9a877fbf5f6aca90b8b8b39873d710c54666124")},
                { 600000, uint256S("0x236808140cbab2492d2ea5bd916da435f06b8c607f8b2339bc86e09e3b70a303")},
                { 650000, uint256S("0x2fdddce264c53c088bd095d3b7f2d538632b54c5ff82994500a470460404e655")},
                { 700000, uint256S("0x9f04fa89d0e39da9c3e99438d43b4861aba6014a28c7f69d973415b66020c69b")},
                { 750000, uint256S("0x1e46d2d631855e1ed84441deb0e2197148a748fd7d468b4e3627083e1cea89f5")},
                { 800000, uint256S("0xfe56466ed0fdbda570cd5b3be1b80d45f328860ca62975968a7aff78adbf99e2")},
                { 850000, uint256S("0x84a8973d9b2e9b72e297beeec9e8059f1b875d3a03f352ce1a3bd478fd5d1bc0")},
                { 900000, uint256S("0xbdaf79da0977c9e59b442548c4f32ef3f9e975eabe7efa2a953a334e644cff81")},
                { 950000, uint256S("0xc49309cd4d08b998497cdf8e42e1daa4f0a0fc8c9e3d798eaeb2c9e0445a8a32")},
                { 1000000, uint256S("0x27d83e0c20a0067c970aaab76c96b73c1a8abac3961a0e9c5d181659ad5d9632")},
                { 1050000, uint256S("0x704d0ee7f3a8140109cac08df95ffece028e4654555ad9f2891b3dcab6f68b61")},
                { 1100000, uint256S("0x6db222f870de768ec72414e67ec758a92163b930e16ed1e417e8ad57c2277f25")},
                { 1150000, uint256S("0xb5367136b20f79d551cc6794946cc95ba5ea9d3e0ac46dfdda9cd9dc30265eb0")},
                { 1200000, uint256S("0xbc391ef1e260130e4b1ed05e7ccf5438000478c933d7a747bf326af9d1c8b118")},
                { 1250000, uint256S("0x7a83fed51cb60f1547246a40a042f7f3ea0291f40d09a28d0b9368cb41345279")},
                { 1300000, uint256S("0xccb0ccb4f2d160b7f51332da641a6c0f5d76f4dadb1d47d6c5fac73d29aa9fd6")},
                { 1350000, uint256S("0x7562e5c43ff39a6993557d4f0f9e49690ba9d07a6b054c7313a35606c5eaf814")},
                { 1400000, uint256S("0x5b85bce8fa177da2ed744a3fbe35bb4e5e1ff319c110b17ec0715e6d3b1ca8de")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 30720 0000000000000000000b9d2ec5a352ecba0592946514a92f14319dc2b367fc72
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = std::numeric_limits<int>::max();
        consensus.nMandatoryUpgradeBlock = 0;
        consensus.nTreasuryPaymentsStartBlock = 200;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000002179077fe51cd42f32e9133bb4d792a267b1068f554bbf15bd7122fabeb");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0; // segwit activation height + miner confirmation window
        consensus.powLimit[CBlockHeader::ALGO_POS] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000"); // 0x1e00ffff
        consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN] = uint256S("00000fffff000000000000000000000000000000000000000000000000000000"); // 0x1e0fffff
        consensus.nPowTargetTimespan = 12 * 60 * 60; // 12 hours
        consensus.nPowTargetSpacing = 80; // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0xf; // 16 second time slots
        consensus.nStakeMinDepth[0] = 100;
        consensus.nStakeMinDepth[1] = 100;
        consensus.nStakeMinAge[0] = 2 * 60 * 60;
        consensus.nStakeMinAge[1] = 2 * 60 * 60; // testnet min age is 2 hours
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60; // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = (14 * 24 * 60 * 60 * 75) / (100 * consensus.nPowTargetSpacing); // 75% for testchains
        consensus.nMinerConfirmationWindow = 14 * 24 * 60 * 60 / consensus.nPowTargetSpacing; // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTreasuryPaymentsCycleBlocks = 24 * 6 * 60 / consensus.nPowTargetSpacing; // Ten times per day
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("1d25c8716a4513320d1c26b3fbd3d2f6dfe3fc37"), 100); // 3% (full reward) for p2wpkh
        consensus.nTreasuryRewardPercentage = 3; // 3% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
        consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // 1864000

        pchMessageStart[0] = 0x85;
        pchMessageStart[1] = 0x7c;
        pchMessageStart[2] = 0x6d;
        pchMessageStart[3] = 0x86;
        nDefaultPort = 27192;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 5;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1612170000, 1383744, UintToArith256(consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN]).GetCompact(), 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash testnet: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash testnet: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0xa7555f3b033a3079b467fc9eb8128ddf2b29a6c9695e820610b1b99ce02f2a3e"));
        assert(consensus.hashGenesisBlock == uint256S("0x000002179077fe51cd42f32e9133bb4d792a267b1068f554bbf15bd7122fabeb"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("xuez.donkeypool.com");
        vSeeds.emplace_back("seed1.xuezcoin.com");
        vSeeds.emplace_back("seed2.xuezcoin.com");
        vSeeds.emplace_back("seed3.xuezcoin.com");
        vSeeds.emplace_back("seed4.xuezcoin.com");
        vSeeds.emplace_back("seed5.xuezcoin.com");
        vSeeds.emplace_back("seed6.xuezcoin.com");
        vSeeds.emplace_back("seed7.xuezcoin.com");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,139);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "xzt";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {0, uint256S("0x000002179077fe51cd42f32e9133bb4d792a267b1068f554bbf15bd7122fabeb")},
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 000000000000006433d1efec504c53ca332b64963c425395515b01977bd7b3b0
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0,
        };
    }
};

/**
 * Signet
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("xuez.donkeypool.com");
            vSeeds.emplace_back("seed1.xuezcoin.com");
            vSeeds.emplace_back("seed2.xuezcoin.com");
            vSeeds.emplace_back("seed3.xuezcoin.com");
            vSeeds.emplace_back("seed4.xuezcoin.com");
            vSeeds.emplace_back("seed5.xuezcoin.com");
            vSeeds.emplace_back("seed6.xuezcoin.com");
            vSeeds.emplace_back("seed7.xuezcoin.com");

            consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
            consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000"); // 9434
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 30720 0000002a1de0f46379358c1fd09906f7ac59adf3712323ed90eb59e4c183c020
                /* nTime    */ 0,
                /* nTxCount */ 0,
                /* dTxRate  */ 0,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = std::numeric_limits<int>::max();
        consensus.nMandatoryUpgradeBlock = 0;
        consensus.nTreasuryPaymentsStartBlock = 200;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 12 * 60 * 60; // 12 hours
        consensus.nPowTargetSpacing = 80; // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0xf; // 16 second time slots
        consensus.nStakeMinDepth[0] = 10;
        consensus.nStakeMinDepth[1] = 600;
        consensus.nStakeMinAge[0] = 6 * 60 * 60; // previous min age was 6 hours
        consensus.nStakeMinAge[1] = 12 * 60 * 60; // current minimum age for coin age is 12 hours
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60; // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = (14 * 24 * 60 * 60 * 95) / (100 * consensus.nPowTargetSpacing); // 95% of the blocks in the past two weeks
        consensus.nMinerConfirmationWindow = 14 * 24 * 60 * 60 / consensus.nPowTargetSpacing; // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTreasuryPaymentsCycleBlocks = 1 * 24 * 60 * 60 / consensus.nPowTargetSpacing; // Once per day
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit[CBlockHeader::ALGO_POS] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000"); // 0x1e00ffff
        consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN] = uint256S("00000377ae000000000000000000000000000000000000000000000000000000"); // 0x1e0377ae
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("1d25c8716a4513320d1c26b3fbd3d2f6dfe3fc37"), 100); // 3% (full reward) for p2wpkh
        consensus.nTreasuryRewardPercentage = 3; // 3% of block reward goes to treasury

        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 37393;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1612170000, 8913562, UintToArith256(consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN]).GetCompact(), 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash signet: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash signet: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0x28bbad827cbecb021628bfdf85db568320498545b2e907023b35775d5e6bae89"));
        assert(consensus.hashGenesisBlock == uint256S("0x000003027d8c5c4e4a697ae9389fb899619fa23b8d6a6313361c9c162367fe70"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,139);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "xzt";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = std::numeric_limits<int>::max();
        consensus.nMandatoryUpgradeBlock = 0;
        consensus.nTreasuryPaymentsStartBlock = 30;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit[CBlockHeader::ALGO_POS] = uint256S("7fffff0000000000000000000000000000000000000000000000000000000000"); // 0x207fffff
        consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN] = uint256S("7fffff0000000000000000000000000000000000000000000000000000000000"); // 0x207fffff
        consensus.nPowTargetTimespan = 1 * 60 * 60; // 1 hour
        consensus.nPowTargetSpacing = 80; // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0x3; // 4 second time slots
        consensus.nStakeMinDepth[0] = 0;
        consensus.nStakeMinDepth[1] = 0;
        consensus.nStakeMinAge[0] = 1 * 60;
        consensus.nStakeMinAge[1] = 1 * 60; // regtest min age is 1 minute
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60; // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = (24 * 60 * 60 * 75) / (100 * consensus.nPowTargetSpacing); // 75% for testchains
        consensus.nMinerConfirmationWindow = 24 * 60 * 60 / consensus.nPowTargetSpacing; // Faster than normal for regtest (one day instead of two weeks)
        consensus.nTreasuryPaymentsCycleBlocks = 20;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("1d25c8716a4513320d1c26b3fbd3d2f6dfe3fc37"), 100); // 3% (full reward) for p2wpkh
        consensus.nTreasuryRewardPercentage = 3; // 3% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xd5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1612170000, 15987, UintToArith256(consensus.powLimit[CBlockHeader::ALGO_POW_XEVAN]).GetCompact(), 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash regtest: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash regtest: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0xef0fff04651cfb240fae33d129ec25d67452f11e2f3d621ef5621d7995099be2"));
        assert(consensus.hashGenesisBlock == uint256S("0x0000ee24cde738fbfe9677c6395247ab0b1225c0354c4c4284b3225d9913d8a0"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("0x0000ee24cde738fbfe9677c6395247ab0b1225c0354c4c4284b3225d9913d8a0")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,139);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "xzr";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}

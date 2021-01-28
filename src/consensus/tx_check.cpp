// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_check.h>

#include <primitives/transaction.h>
#include <consensus/validation.h>

static inline bool ValidZerocoinSpend(const CTransaction& tx)
{
    const uint256 &txHash = tx.GetHash();

    static const uint256 validHashes[117] = {
        uint256S("a0f67175abc727a4275a4202e03d14bceab701ece23c430eed0f19f67d3e3acf"),
        uint256S("9148f73ab15823c6046645f4fea0a874a9932db67a8de003a88c386bc1558771"),
        uint256S("81d2064e9f6f799edad9c8588c78e3b99c07c27eff7563d42b14cc93ca6c122e"),
        uint256S("be824943ffa648f9a926e9728a3e7a49c868d05ae48e8b11710875b3da6bd70a"),
        uint256S("cbf6dafcdab42be08972139d68ccf4edc69850af4812ff79ab3e0c35bb0f0364"),
        uint256S("9650e3efba89ab84e94deddeb1d6bfa061627574001162866f405d35da60b108"),
        uint256S("4a5d425446b2168a6469257cad43014b4ce45eed1d574d76c2c0925420968076"),
        uint256S("3f1b6aebf057fc23983e9a35afacdacbce1b48823c61d0936c32359c453e3574"),
        uint256S("3f4fa0306b0fc1446b179837b48ad87458fe9b35baab7a1dd22254151f2710f2"),
        uint256S("d01b228f00e4eb542c28bb6d7f2f2d737f1ce84523486457df16ac6ad6325e65"),
        uint256S("703eeb1fde1e7095f8ab9ec18ec2797b92d2d23920f003c5c00516847d8b2939"),
        uint256S("3433304aa37d86a8de254f0e6029142b8a32e4ca899d7af4791887d5e386b518"),
        uint256S("f2a9d418153d84c6a8b8ec96d8f9898f8790dfc5b2c89024f4bf22c5c6cb3f0e"),
        uint256S("88bd7df7299956c5df8039a3e073a915d97c17030d62bb68df3a1877ce719cf2"),
        uint256S("fae105e427e719e826f742832a224d57894de7a67c3590c5f02c92c1fb6d9b3d"),
        uint256S("e2fae035b80ea46cb99c3be69b86dded919cb21558ac3f002a412448c9bc2b69"),
        uint256S("7d4c5b4d71bec2d9a0051848f3f79b81aba4ea12c26dac8e5e432ffe3a271ee4"),
        uint256S("07d23457c830e3ba711f15ea0bedb19f507da8dacb6daec7f290b8a154c882f7"),
        uint256S("9b2d94596d267c8407d54319158a4a92168efbfe2130b18c93320428c752e760"),
        uint256S("e186824e8335943b5239f6655a20b614089d4347be8d850aac6aba510c069ca4"),
        uint256S("d9a3d2688cff7075484d22622a5f5670507aad24f22545c05adb18ec653488e0"),
        uint256S("e5030abcfe2a30b036fa4e317c640fbb411dece7cc5317db26494eb0dae4bcf4"),
        uint256S("a8115f927292381a226df6012225b1616bd22f0e515909143de0b3f88d594762"),
        uint256S("92f1b44a714e0d08d7f6f1af859b38f6fafdfb9ca05a3e86b24e373f0bbe135c"),
        uint256S("211aa69ddc92c6afaeae00af15e088c5ac38f31c35f1d7f1e3676cc2b5c40568"),
        uint256S("d077a438f0a5b6d5ae1ea66367f0cdffb45fddbb3a67944b9825d1c016862f94"),
        uint256S("10ece7399e11af7b5ad61dbf6f6ea7f27027352c3c324a0dc2f63a6ccaa1c2ff"),
        uint256S("bc1d338aedee4b18fe4f7187d9a384b4938b580638d9840f80e32a528753382c"),
        uint256S("159d09ddcbd36e8deb5c5947fa7695bbc7848cc8706473594a2a4787b9408e28"),
        uint256S("f3030ead2ea8c6d48842889ce22bbc5cacc575b2468ae4089a085262536271de"),
        uint256S("e8d641c274d16742423a82bb6ae5f60a7a83e1acf76b91e28f2f004e831a872d"),
        uint256S("ecf5a7c97ba1400fc682fbd7a28f3cd9f6f832b8c421739931158f016ae3d6f0"),
        uint256S("a2779f1177ba0228a26369b07980e1d5c9c14586a118b1864e53ba7ff6832bf0"),
        uint256S("0ac90eabbb801dce539e9055b29749f49ebea6fbc5461ac267321c52df9968f0"),
        uint256S("ee954f74ad6235305c42644fa320fe84b66364e8405de96e4590a1985d8ba8c3"),
        uint256S("0885bcc7899857c4fadddde0b4fe85da48f47e96947da4a8c6976858f0fccaad"),
        uint256S("799c3cd5973ce6b617b1f534c9bc776889b8f020dc4433a472f0d698ffc80f96"),
        uint256S("68378425394a95c9526a2bf4786e3e4f61f252cb703c25997d0bc986cd90a146"),
        uint256S("6f511fbb5c8d4f30dc3cace184fc2ee094e38bc7fb985d7addf46d6892bc538a"),
        uint256S("802ea65e804816120504ae93b32d67b4a58c4502a114f8c82377ff36f6ab27bb"),
        uint256S("bedd0528e888f51a293ea91d1268c7554b7d5de4845e4cd3f8ca1b8eedc02bc9"),
        uint256S("3d8f88b0ce128c370a5022159012376d9994544eba60c19dd89ce10b6a5896b2"),
        uint256S("3b3610d7463d46acf06aa299476624d89cd171687f4e03e6c17b741c2aca01e7"),
        uint256S("3e6c4de82d2360dbbbb083b878d25cd8dc6c8e97c6eed27b74335a0f133d242b"),
        uint256S("458eea47c8f8f62156f774a3269e98d487dd064a970bf0070e45913bae5744fe"),
        uint256S("6470f8ffe313b7dacfaa36aba46aecb638d73ea69971210fd1729732e646563d"),
        uint256S("60da74862c87487c39a6c8eb6861eec653e59f03b1fd7ee79f1f797b4654e509"),
        uint256S("491f076016a8f4ab6cd7a235be28046c56aeb2bc6b8961253a7cff2ebe534bd7"),
        uint256S("fe1ae1ac6065efaebd88cb2a6ce47e78963106d2c4a30be5af8029d2f0e85115"),
        uint256S("7aabc7082176fa9b11b11e135dfacf0ab9912a678a57ad944aac74267e9fc558"),
        uint256S("e3b1ed62c3f9c2c04a3e7dfde040f66a6b4a937aa1d421507e1afd32c4ac22e1"),
        uint256S("0341623fe5b32212b1fe383015d2b53e7065f67816c26c34f7eaa98f861d20fe"),
        uint256S("dcb8fcb1c178c1f70d2b0dac41dbde3921c07c9ee9f87550a1526a8cb6562e41"),
        uint256S("3e90b6d48ac5426d21b0f3101657c7745b0d840a3be137b1a52449602255c926"),
        uint256S("d6eba8d4168f9723083d80b718d9460457204aab2cd5adefca941167846fffaa"),
        uint256S("456186bdb49ceb42c70ce87038ad29ff59fad95051c162bcdaa3acf3e46c4637"),
        uint256S("5b2d0acbe8fe46d2a1ae2d1d0fc013bfade249eeabb6fdcd8d94ac53d4a2a229"),
        uint256S("2c4699a3c94a89bb00ca26e06e67f2623b235c51073595d3320d26e5c6efcdbc"),
        uint256S("6ca3430b25733f1084c4936129d93c759409a7a837b532055b759adfde96aac3"),
        uint256S("4e0efa66277dce6d1f33927a3f710aea681240e7e137fc4c5cf0de5de9808bf5"),
        uint256S("7dc8369a3bd35ff61af83a10a8e4b2e33460eefc9d32c95fc75e605893237784"),
        uint256S("ffb8629ebf70218ff1d961dec447c04205fb620bd3a49fbbc654c6e7983f2bc1"),
        uint256S("a2c61c6ae363e061c29eeddd9aaf612d242bb34cb9e11b7969d8ecad80200f10"),
        uint256S("6ef7f3ddd8ead4eb631388c056e3c93dbad73e0f019045e1635d199845aab88b"),
        uint256S("3c02be7a293312bfce78ef211bbde69522fe85ffdb7e63891d8f6351f0de0645"),
        uint256S("11dd91ea619d35d2c60995d7cd143ac66e8e4fd4ee256071c4f7d29fee803583"),
        uint256S("39be6b6a8faf1b6808aba447a13ce1315227d7be93fefd434b110ae32f8f2dc7"),
        uint256S("6663a05fbf2676f9e0e5250700772f54103776e566fbdd62bc8a4c2805388607"),
        uint256S("09172e32a3ab41f15eb44c0415437c8ec9bd85d53a9cd1f5d914389d0d3040c4"),
        uint256S("fa1968f6ca08969550915d6d7c57a0a5482c96607e7ea8908b245485a69feaee"),
        uint256S("d8efea4e60f7217736f867be00c2d17286661b26ff0f8300c66ca2650509c6fb"),
        uint256S("87d09faec3eb8e6b807b6cc920b65a8080e5bfbdd47ce81258da012a39e14ea4"),
        uint256S("85707dadd4499f5053c598c7f1ddd1db249a557525c1091ffbc947aa90eb5fae"),
        uint256S("82c111dada30ad8e3b2ff69b18944cf81d02f9a49a684d834e25c007c248e8d7"),
        uint256S("a95488981510c3b67aded294dea69300181887a17c2e487f6778ca9f68da32c7"),
        uint256S("4ac256acb7cc54eba0d8e83e1bbb811aad880b32de3eb2b5aec213a08b2d8e07"),
        uint256S("9a74dd73836eb1878a0a04854e8a10f189443d3b32ce5f9dc751e62c1ca89ce2"),
        uint256S("5a918435f4df7cf077060920e3a4626ac635ca89e2d56f25bac7398229946536"),
        uint256S("5438d91e395ea783dac8787808761df53160cca59285730e36f8bd2ed782b43e"),
        uint256S("364d489d3adf272e4670223a6b90964c1cb591efdb64928422aafbd759c1020c"),
        uint256S("04144a04c48504eb63a33250ad4c02c031f32bf4629c69bc002c9a1c24f49667"),
        uint256S("bbc7434dab0d0b3d8beec35055af1737d5cca15b2c98ca9f9226645c42f3d654"),
        uint256S("d6adcbef25e1206173f1c6d2ce017c6a3d3bcd45698607fab81feae8b784d6cb"),
        uint256S("a17426a8a04280493b84c26d531a64c02b077df8214d4e9b57e54eac88a83b4c"),
        uint256S("73ff3a23d011d48a510b079f2213a5b693382296c2c8e85b0980ded3c1281fa8"),
        uint256S("52cee468c72d9d7fc7e62ad351f65b0b9c1deee9ee2f05246f4621e433bc7818"),
        uint256S("1a519fb2f7fae1f9c1fae42fcf2f48009bab9f4e0ceb35d64ef05297345e699b"),
        uint256S("ee84147a4d046cd1cf8e7c600016d705e372a6ce3a0465e0e06669b962fbbf60"),
        uint256S("c0f54e520da80b5cad1b93cb440acb22d8d5b11b46579bc9ba31ee2db008cada"),
        uint256S("f4d3a026736bc63931232e5ffafef4bc9f361d1259fde3acd286bc65c17a0269"),
        uint256S("d7ee8fdaf45cd3737b818f659b93617a4ca6bdd3f9059dc83479518c8ed4f72d"),
        uint256S("4fee710da74d42af706e9a9177cbbd8ffaaa11b9288f2f5e88faa2f6a7f3d801"),
        uint256S("1630e5fa1b8fd4bf2d4b773fb0442dc363438cf2ad5eab129fc1cb3c57216199"),
        uint256S("532e2f10b155b2b9a2bd0b84ff40d80783d282f0d399f983d9d51a9dad125c6d"),
        uint256S("483a00758e9256103822b4af3e3d371b3b34532faab2933c25a890ad703373d7"),
        uint256S("5dd5f8b5b6e5dc7eb6c2d232267c994cd340c0932c717d2293461aeb67dab7ec"),
        uint256S("ccec40642b0d24bb22be84afe5546aaea4cc8f75bf77c00ab5cdf6903563ae18"),
        uint256S("07812c99e56a3161db7a4c76c0db46a803ef378e6f2eafb2f60a42d92400197f"),
        uint256S("b78ca957af5a8dca76f79e528228d6edde029c910993a145bdd795a5b54cac47"),
        uint256S("d1724da154202161bef84eb49a15bdac91add09428ce17685f3286cfcbde6263"),
        uint256S("19b9ed7df176d26e4fa19eb072d40d2940eaef1928dcc6cde7504ef4e5c588d4"),
        uint256S("3d01eefe12234fcf25f69580231c5e094f23ee378ab55e78cfd0181d7331952d"),
        uint256S("feb2ba977656beaf0ea59fda1d3aaf598b2ca51bf04e652b9ba6daf282f435bf"),
        uint256S("48af3a8e49ab729770615a726c8f5f76f0ea6cd40b73e2b6936cb054932b23c5"),
        uint256S("21c33f5a66d4fd4d4c092a93c508156d2b85ac81a89c97ae6c73ee6ac8f821a3"),
        uint256S("cd5a0384e48eea8370162fe5ad306cec3117de463cb6236d1a7fcad6eb880ad7"),
        uint256S("0de39edce6e366963373a08c84c893cc7006dcc7730c0d92bf7e9616e74f87cb"),
        uint256S("3f5aafaa12710823f19dd8b1043c36ada5809500c07aad300659ab169a9d4c8b"),
        uint256S("2d712c6f2df38ad6f4f4d225c1deb361839ba06c00ea4dcf931dfb95f08f3d31"),
        uint256S("8f3bbe96821624ab712d91e5e8aa6290b25aa8b20bf62696dd4861961ca0ed73"),
        uint256S("61f14efa1efdd63b1c9f71d60c05bd8d17bfa9d8276ca66cf97a8d48f9a6b06d"),
        uint256S("f02968b25b77974ea308cbc1f939903a91a64d69e20407f2be18a0758573cb9a"),
        uint256S("237fb3503b089f88fdaab4c9a3d2810d74c72e28a642bd4c4f7a28cc932d1b84"),
        uint256S("a71dee9e25518deecf01d7905565992d591344262d4519acdee5473702ef7f97"),
        uint256S("99389577cb29bbece71b99540c44fa186f5c402500f3f39268056c642a459622"),
        uint256S("d4c6ae78b15dbfdbad2d2d151dbb646ba2e9810e366e33995611a579aab9433f"),
        uint256S("4c69c2f89c591f1b7eb8ab5139ab6dac22ad0c5367e826031c96ec9b98d9bc60"),
    };

    for (const uint256& hash : validHashes) {
        if (txHash == hash)
            return true;
    }
    return false;
}

bool CheckTransaction(const CTransaction& tx, TxValidationState& state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    const unsigned int size = ::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    if (size * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-oversize");
    // 64-byte transactions are rejected to mitigate CVE-2017-12842
    if (size == 64)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "tx-size-small");

    // Check for negative or overflow output values (see CVE-2010-5139)
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.nValue < 0)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs (see CVE-2018-17144)
    // While Consensus::CheckTxInputs does check if all inputs of a tx are available, and UpdateCoins marks all inputs
    // of a tx as spent, it does not check if the tx has duplicate inputs.
    // Failure to run this check will result in either a crash or an inflation bug, depending on the implementation of
    // the underlying coins database.
    std::set<COutPoint> vInOutPoints;
    for (const auto& txin : tx.vin) {
        if (!vInOutPoints.insert(txin.prevout).second && (tx.nVersion != 1 || !txin.IsZerocoinSpend()))
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-inputs-duplicate");
    }

    const bool fZerocoinSpend = tx.nVersion == 1 && tx.HasZerocoinSpendInputs();
    if (fZerocoinSpend && !ValidZerocoinSpend(tx))
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-invalid-zerocoin");

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-cb-length");
    }
    else if (!fZerocoinSpend)
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-txns-prevout-null");
    }

    return true;
}

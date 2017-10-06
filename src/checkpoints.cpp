// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (       0, uint256("0x00000c31cbfa287f2bc7c6c5634475883af72c6dd47cd3d27341bc668f731c81"))
		(       1, uint256("0x00000e42c6e6ec223410e7916d11d9483e24933594aed7d326338cd32381f334"))
		(    4700, uint256("0x00000000bd96f25c5fe68b003e665445a94a050182e23c37022438f9caffe472"))
		(   31124, uint256("0x000000001f3316fd17ecb40019bfae299a5e7f40c8cea57bd3e34237c4c04638"))
		(   53233, uint256("0x00000000fbcda674f094486c1684ca0cc99f537576b4d3445babe6ad21a23db2"))
		(   66437, uint256("0x00000000b445027f5b4f117f5d2e76d3352cff67375ecf265dc1d5d9f157c239"))
		(   71621, uint256("0x000000007c35a5ce3ef7c77f7aa535a88e1ed03b9793be4d46d9c462504e2aa1"))
		(   92490, uint256("0x000000003050d117a6d410057be32506be8ad02a96c27e08dd3f7a41b8671ce7"))
        (  150000, uint256("0x00000001b5e05ebcc219012b7c7832d28d86d3249e3387d8593e6c7148bb3547"))
        (  200000, uint256("0x000000012d9d0aba3f4af54bbc2788efeb28d28d4b23a9923dec67d25d454394"))
        (  250000, uint256("0x000000002c704d9fac463bcde288626d94e610ee288e21574b9919650f87d8c0"))
        (  300000, uint256("0x000000086b931f20226dc9759ed7e6ea479fceec308812ecba1463be11837e4b"))
        (  350000, uint256("0x000000023fc820dec0cd6c35fcf239e1c99145cd19c6a985199cf0bbc3ae07fd"))
        (  400000, uint256("0x0000000088b3390955e9de9a802e8399e44290d16fd72f787c6f0af1b8d46899"))
        (  450000, uint256("0x000002392b10bda48faddaf4b855ba51bcde527bf16c91131fa390ae8022fb7c"))
        (  500000, uint256("0x000000c8d4e43f5579c728e870198ea236daddae7f6bea62003e993ccb657ac9"))

        ;

    static const CCheckpointData data = {
        &mapCheckpoints,
        
        1507133860, // * UNIX timestamp of last checkpoint block
        //125908,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        1000.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet = 
        boost::assign::map_list_of
        (   0, uint256("0x332865499df77f269f1fa1c640075275abc3b452c21619bfe05f757a65a46c48"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1394545201,
        0,
        100
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL; // Testnet has no checkpoints
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
	
	uint256 GetLatestHardenedCheckpoint()
    {
        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
        return (checkpoints.rbegin()->second);
    }
}

#pragma once
// This file and the associated implementation has been placed in the public domain, waiving all copyright. No restrictions are placed on its use. 
#include <array>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Common/Timer.h>
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h"
#include "libOTe/TwoChooseOne/IknpOtExtReceiver.h"
#include "Poly/polyNTL.h"
#include "PsiDefines.h"
#include <NTL/ZZ_p.h>
#include <NTL/vec_ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/ZZ.h>
#include "Tools/BalancedIndex.h"
#include "PrtyMOtReceiver.h"

using namespace NTL;

namespace osuCrypto
{

    class PrtyMPsiReceiver : public TimerAdapter
    {
    public:
     
		
		bool mHasBase;
		bool mIsMalicious;

		u64 mMyInputSize, mTheirInputSize, mMaskLength, mPsiSecParam, mNumBin, mSigma, mNumOTs, mMalNumOTs;
		std::vector<block> mS;
		std::pair<u64, u64> mCuckooItemLength;

		PrtyMOtReceiver mPrytOtRecv; //
		
		PRNG mPrng;

		std::vector<u64> mIntersection; //index

		void init(u64 myInputSize, u64 theirInputSize, u64 psiSecParam, PRNG& prng, span<Channel> chls, bool isMalicious=false);
		void output(span<block> inputs, span<Channel> chls);

		
    };

}

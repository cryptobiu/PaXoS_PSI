#pragma once
// This file and the associated implementation has been placed in the public domain, waiving all copyright. No restrictions are placed on its use.  
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include "libOTe/TwoChooseOne/IknpOtExtReceiver.h"
#include "PsiDefines.h"
#include "PrtyMOtSender.h"


#include <array>
namespace osuCrypto {

	class PrtyMPsiSender :public TimerAdapter
	{
	public:

		bool mHasBase;

		u64 mMyInputSize, mTheirInputSize, mMaskLength, mPsiSecParam, mNumBin, mSigma, mNumOTs;
		PrtyMOtSender mPrytOtSender; //
		bool mIsMalicious;

		std::pair<u64, u64>  mCuckooItemLength; //plaintext/encoding lenght

		PRNG mPrng;
		BitVector mOtChoices;

		void init(u64 myInputSize, u64 theirInputSize, u64 psiSecParam, PRNG& prng, span<Channel> chls, bool isMalicious=false);
		void output(span<block> inputs, span<Channel> chls);
		
	
	};
}


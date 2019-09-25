//
// Created by moriya on 7/24/19.
//

#ifndef OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#define OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H
#include <cryptoTools/Network/IOService.h>
#include <libPSI/PsiDefines.h>
#include <libPSI/PRTY2/PrtyMOtReceiver.h>
#include <libPSI/PRTY2/PrtyMOtSender.h>
#include <libPSI/Tools/mx_132_by_583.h>
#include <libOTe/Tools/Tools.h>
#include <libOTe/Tools/LinearCode.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Common/Log.h>

#include <libscapi/include/comm/MPCCommunication.hpp>
#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/Measurement.hpp>


#include <string.h>
#include <thread>
#include <bitset>



using namespace osuCrypto;

#include "ObliviousDictionary.h"

class ProtocolParty : public Protocol {

protected:


    int partyId;
    int times; //number of times to run the run function
    int iteration; //number of the current iteration
    int numOTs, tableRealSize, hashSize, fieldSize, fieldSizeBytes;
    int reportStatistics = 0;
    int gamma;

    PrgFromOpenSSLAES prg;
    EVP_CIPHER_CTX* aes;

    vector<uint64_t> keys;
    vector<byte> vals;
    ObliviousDictionary* dic;

    Measurement *timer;
    shared_ptr<ProtocolPartyData> otherParty;
    boost::asio::io_service io_service;

public:

    ProtocolParty(int argc, char *argv[]);
    ~ProtocolParty();

    void run() override;

    bool hasOffline() override {
        return false;
    }


    bool hasOnline() override {
        return true;
    }

    void runOffline() override {}


};

class Receiver : public ProtocolParty {
private :
    PrtyMOtReceiver recv;
    unordered_set<uint64_t> xorsSet;

    vector<byte> createDictionary();

    void runOT(vector<byte> & sigma);

    void runOOS(vector<byte> & sigma);

    void computeXors();
    void checkVariables(vector<byte> & variables);

    void receiveSenderXors();
public:

    Receiver(int argc, char *argv[]);
    ~Receiver();

    void runOnline() override;
};

class Sender : public ProtocolParty {
private :
    PrtyMOtSender sender;
    BitVector baseChoice;
    LinearCode code;

    vector<uint64_t> xors;

    void runOT();
    void runOOS();

    void computeXors();

    void sendXors();
public:

    Sender(int argc, char *argv[]);
    ~Sender();
    void runOnline() override;


};


#endif //OBLIVIOUSDICTIONARY_PROTOCOLPARTY_H

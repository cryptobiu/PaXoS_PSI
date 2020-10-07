//
// Created by moriya on 7/24/19.
//

#include "ProtocolParty.h"
#include "cryptoTools/Crypto/RandomOracle.h"
#include "libOTe/Base/naor-pinkas.h"

ProtocolParty::ProtocolParty(int argc, char* argv[]) : Protocol("PSI", argc, argv)
{

    partyId = stoi(this->getParser().getValueByKey(arguments, "partyID"));

    this->times = stoi(this->getParser().getValueByKey(arguments, "internalIterationsNumber"));
    this->hashSize = stoi(this->getParser().getValueByKey(arguments, "hashSize"));
    this->fieldSize = stoi(this->getParser().getValueByKey(arguments, "fieldSize"));
    fieldSizeBytes = fieldSize % 8 == 0 ? fieldSize/8 : fieldSize/8 + 1;
    int zeroBits = 8 - fieldSize % 8;
    unsigned char zeroMask = 0xff >> zeroBits;

    gamma = 60;
    vector<string> subTaskNames{"Online", "CreateDictionary", "OT", "ComputeXors", "ReceiveAndCalc"};
    timer = new Measurement(*this, subTaskNames);

    MPCCommunication comm;
    string partiesFile = this->getParser().getValueByKey(arguments, "partiesFile");

    isMalicious = stoi(this->getParser().getValueByKey(arguments, "malicious")) == 0 ? false : true;
    otherParty = comm.setCommunication(io_service, partyId, 2, partiesFile)[0];

    if (isMalicious)
    {
        bytesPerHash = 32 / 8;
    }
    else
    {
        bytesPerHash = (40 + 2 * std::log2(hashSize) + 7 ) / 8;

        // some logic requires at least 8 bytes.
        if (bytesPerHash < 8)
            bytesPerHash = 8;
    }

    ConfigFile cf(partiesFile);

    string portString, ipString;
    vector<int> ports(2);
    vector<string> ips(2);

    for (int i = 0; i < 2; i++) {
        portString = "party_" + to_string(i) + "_port";
        ipString = "party_" + to_string(i) + "_ip";

        //get partys IPs and ports data
        ports[i] = stoi(cf.Value("", portString));
        ips[i] = cf.Value("", ipString);
    }
    //osuCrypto::IOService ios(0);
    addressForOT = ips[1];
    portForOT = ports[1] + 1;

    cout<<"addressForOT = "<<addressForOT<<endl;
    cout<<"portForOT = "<<portForOT<<endl;

    string tmp = "init times";
    //cout<<"before sending any data"<<endl;
    byte tmpBytes[20];
    if (otherParty->getID() < partyId) {
        otherParty->getChannel()->write(tmp);
        otherParty->getChannel()->read(tmpBytes, tmp.size());
    } else {
        otherParty->getChannel()->read(tmpBytes, tmp.size());
        otherParty->getChannel()->write(tmp);
    }

    dic = new ObliviousDictionary(hashSize, fieldSize, gamma);

    hashSize = dic->getHashSize();
    tableRealSize = dic->getTableSize();
    numOTs = tableRealSize*2 + gamma;
    keys.resize(hashSize);
    vals.resize(hashSize*fieldSizeBytes);


    //    auto key = prg.generateKey(128);
    prg = PrgFromOpenSSLAES(hashSize*fieldSizeBytes*2);
    vector<byte> seed(128, 0);
    SecretKey key(seed, "");
    prg.setKey(key);

    for (int i=0; i < hashSize; i++){
        keys[i] = i;//prg.getRandom64();
    }
    prg.getPRGBytes(vals, 0, hashSize*fieldSizeBytes);

    //masking the values so they will not have random in the padding bits
    for (int i=0; i<hashSize; i++){
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;

        vals[(i+1)*fieldSizeBytes-1] = vals[(i+1)*fieldSizeBytes-1]  >> zeroBits;
//
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;
    }

    aes = EVP_CIPHER_CTX_new();
    const EVP_CIPHER* cipher = EVP_aes_128_ecb();

    vector<byte> fixedKey(128);
    prg.getPRGBytes(fixedKey, 0, 128);
    EVP_EncryptInit(aes, cipher, fixedKey.data(), NULL);

    switch(fieldSize){
        case 65:
            code.load(mx65by448, sizeof(mx65by448));
            cout<<"load mx65by448"<<endl;
            break;
        case 72:
            code.load(mx72by462, sizeof(mx72by462));
            cout<<"load mx72by462"<<endl;
            break;
        case 84:
            code.load(mx84by495, sizeof(mx84by495));
            cout<<"load mx84by495"<<endl;
            break;
        case 90:
            code.load(mx90by495, sizeof(mx90by495));
            cout<<"load mx90by495"<<endl;
            break;
        case 132:
            code.load(mx132by583, sizeof(mx132by583));
            cout<<"load mx132by583"<<endl;
            break;
        case 138:
            code.load(mx138by594, sizeof(mx138by594));
            cout<<"load mx138by594"<<endl;
            break;
        case 144:
            code.load(mx144by605, sizeof(mx144by605));
            cout<<"load mx144by605"<<endl;
            break;
        case 150:
            code.load(mx150by616, sizeof(mx150by616));
            cout<<"load mx150by616"<<endl;
            break;
        case 156:
            code.load(mx156by627, sizeof(mx156by627));
            cout<<"load mx156by627"<<endl;
            break;
        case 162:
            code.load(mx162by638, sizeof(mx162by638));
            cout<<"load mx162by638"<<endl;
            break;
        case 168:
            code.load(mx168by649, sizeof(mx168by649));
            cout<<"load mx168by649"<<endl;
            break;
        case 174:
            code.load(mx174by660, sizeof(mx174by660));
            cout<<"load mx174by660"<<endl;
            break;
        case 210:
            code.load(mx210by732, sizeof(mx210by732));
            cout<<"load mx210by732"<<endl;
            break;
        case 217:
            code.load(mx217by744, sizeof(mx217by744));
            cout<<"load mx217by744"<<endl;
            break;
        case 231:
            code.load(mx231by768, sizeof(mx231by768));
            cout<<"load mx231by768"<<endl;
            break;
        case 238:
            code.load(mx238by776, sizeof(mx238by776));
            cout<<"load mx238by776"<<endl;
            break;
    }

}

ProtocolParty::~ProtocolParty(){

    delete timer;
    delete dic;
}

void ProtocolParty::run() {

    for (iteration=0; iteration<times; iteration++){

        auto t1start = high_resolution_clock::now();
        timer->startSubTask("Online", iteration);
        runOnline();
        timer->endSubTask("Online", iteration);

        auto t2end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(t2end-t1start).count();

        cout << "time in milliseconds for protocol: " << duration << endl;
    }


}

Receiver::Receiver(int argc, char *argv[]): ProtocolParty(argc, argv){

    dic->setKeysAndVals(keys, vals);
    xorsSet = unordered_set<uint64_t>(hashSize);

}

Receiver::~Receiver(){
}

void Receiver::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    timer->startSubTask("CreateDictionary", iteration);
    createDictionary();
    timer->endSubTask("CreateDictionary", iteration);
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "createDictionary took in milliseconds: " << duration << endl;

//    cout<<"before check"<<endl;
//    vector<byte> sigma;
//    dic->getVariables(sigma);
//    checkVariables(sigma);
//    cout<<"after check"<<endl;
    t1 = high_resolution_clock::now();

    timer->startSubTask("OT", iteration);
    runOOS();
    timer->endSubTask("OT", iteration);
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "run OOS took in milliseconds: " << duration << endl;


    t1 = high_resolution_clock::now();
    timer->startSubTask("ComputeXors", iteration);
    computeXors();
    timer->endSubTask("ComputeXors", iteration);
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "computeXors took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();
    timer->startSubTask("ReceiveAndCalc", iteration);
    receiveSenderXors();
    timer->endSubTask("ReceiveAndCalc", iteration);
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "receiveXors took in milliseconds: " << duration << endl;
    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end - start).count();
    cout << "all PSI protocol took in milliseconds: " << duration << endl;
}

void Receiver::createDictionary(){
    dic->init();

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();

    dic->fillTables();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "fillTables took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();
    dic->peeling();

    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "peeling took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();
    dic->generateExternalToolValues();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "calc equations took in milliseconds: " << duration << endl;


    t1 = high_resolution_clock::now();
    dic->unpeeling();

    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2 - t1).count();
    cout << "unpeeling took in milliseconds: " << duration << endl;

    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end - start).count();
    cout << "all dictionary protocol took in milliseconds: " << duration << endl;

//        dic->checkOutput();


}

void Receiver::runOOS(){


    std::string name = "n";
    IOService ios(0);
    Session ep1(ios, addressForOT, portForOT, SessionMode::Client, name);
    auto recvChl = ep1.addChannel(name, name);

    recv.configure(isMalicious, 40, fieldSize);

    //Base OT - simulated
    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
    u64 baseCount = recv.getBaseOTCount();
    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    BitVector baseChoice(baseCount);
    baseChoice.randomize(prng0);
    //prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    //for (u64 i = 0; i < baseCount; ++i)
    //{
    //    baseRecv[i] = baseSend[i][baseChoice[i]];
    //}

    oc::NaorPinkas base;
    base.send(baseSend, prng0, recvChl);
    recv.setBaseOts(baseSend);

    vector<byte> sigma;
    dic->getVariables(sigma);
    //OT
    PRNG prng1(_mm_set_epi32(4253465, 3434565, 234435, 23987025));
    recv.init(numOTs, prng1, recvChl);

    for (u64 i = 0; i < numOTs; i += stepSize)
    {
        auto curStepSize = std::min<u64>(stepSize, numOTs - i);
        for (u64 k = 0; k < curStepSize; ++k)
        {
            recv.otCorrection(i + k, &sigma[fieldSizeBytes*(k + i)]);
        }
        recv.sendCorrection(recvChl, curStepSize);
    }

    std::cout << "sent OT matrix, " << recv.mT1.size() << " = " << recv.mT1.rows() << " x " << recv.mT1.stride() << std::endl;
}

void Receiver::computeXors(){

    u64 baseCount = recv.getBaseOTCount();
    int blockSize = baseCount/128;

    vector<block> output(blockSize);
    vector<byte> temp(xors2.stride());
    int size;



    vector<int> indices(gamma+2);
    int indicesSize;
    oc::RandomOracle ro(xors2.stride());

//    cout<<"Xors:"<<endl;
    for (int i=0; i<hashSize; i++){

        indicesSize = dic->dec(keys[i], indices);

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(*(recv.mT0.data(indices[0]) + j),*(recv.mT0.data(tableRealSize + indices[1]) + j));
        }

        for (int k=2; k<indicesSize; k++) {
            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], *(recv.mT0.data(2*tableRealSize + indices[k]) + j));
            }
        }


        //EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);
        ro.Reset();
        ro.Update((byte*)output.data(), blockSize * 16);
        ro.Final(temp.data());

        xorsSet.insert(((uint64_t*)temp.data())[0]);

//        for (int j=0; j<20;j++){
//            cout<<(int)(((byte*)output.data())[j])<<" ";
//        }
//        cout<<endl;
//        cout<<((uint64_t*)temp.data())[0]<<endl;

    }


}

std::vector<u64> Receiver::receiveSenderXors(){

    uint64_t size, bytesPer;
    otherParty->getChannel()->read((byte*)&size, 8);
    otherParty->getChannel()->read((byte*)&bytesPer, 8);


    //vector<uint64_t> senderOutputs(size);
    oc::Matrix<byte> senderOutputs(size, bytesPer)
    otherParty->getChannel()->read((byte*)senderOutputs.data(), senderOutputs.size());

    std::vector<u64> ret;
    uint64_t count = 0;
    for (int i=0; i<hashSize; i++){
        //auto val = 
        uint64_t vv = *(uint64_t*)senderOutputs[i].data();
        if (xorsSet.find(vv) != xorsSet.end()){
            count++;
            ret.push_back(i);
//            cout<<"element "<<i<<" is in the intersection"<<endl;
        }
    }

    cout<<"found "<<count<<" matches"<<endl;

    return ret;
}



void Receiver::checkVariables(vector<byte> & variables){
    recv.configure(isMalicious, 40, fieldSize);
    u64 baseCount = recv.getBaseOTCount();
    int blockSize = baseCount/128;

    vector<vector<block>> outputs(keys.size(), vector<block>(blockSize));

    cout<<"baseCount = "<<baseCount<<endl;
    cout<<"Xors:"<<endl;

    vector<int> indices(gamma+2);
    int indicesSize;

    bool error = false;
    vector<byte> check(fieldSizeBytes);
    for (int i=0; i<hashSize; i++){

        indicesSize = dic->dec(keys[i], indices);

        for (int j=0; j<fieldSizeBytes; j++) {
            check[j] = variables[indices[0]*fieldSizeBytes + j] ^ variables[tableRealSize*fieldSizeBytes + indices[1]*fieldSizeBytes + j];
        }

        for (int k=2; k<indicesSize; k++) {
            for (int j=0; j<fieldSizeBytes; j++) {
                check[j] = check[j] ^ variables[2*tableRealSize*fieldSizeBytes + indices[k]*fieldSizeBytes + j];
            }
        }

        for (int j=0; j<fieldSizeBytes; j++) {
            if (check[j] != vals[i*fieldSizeBytes + j]){
                error = true;
                cout<<"error in check! xor of D values is not equal to val"<<endl;
            }
        }

    }

    if (error ==  false){
        cout<<"success!!! all D values equal to val"<<endl;
    }

}


Sender::Sender(int argc, char *argv[]) : ProtocolParty(argc, argv){
    xors2.resize(hashSize, bytePerHash);
}

Sender::~Sender() {

}



void Sender::runOnline() {

    auto start = high_resolution_clock::now();
    auto t1 = high_resolution_clock::now();
    runOOS();
    auto t2 = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "run OT took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();

    computeXors();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "computeXors took in milliseconds: " << duration << endl;

    t1 = high_resolution_clock::now();

    sendXors();
    t2 = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(t2-t1).count();
    cout << "sendXors took in milliseconds: " << duration << endl;
    auto end = high_resolution_clock::now();

    duration = duration_cast<milliseconds>(end-start).count();
    cout << "all protocol took in milliseconds: " << duration << endl;

}

void Sender::runOOS(){

    std::string name = "n";
    IOService ios(0);
    Session ep0(ios, addressForOT, portForOT, SessionMode::Server, name);
    auto sendChl = ep0.addChannel(name, name);

    sender.configure(isMalicious, 40, fieldSize);

    //Base OT - simulated
    PRNG prng0(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
    u64 baseCount = sender.getBaseOTCount();

    std::vector<block> baseRecv(baseCount);
    std::vector<std::array<block, 2>> baseSend(baseCount);
    baseChoice.resize(baseCount);
    baseChoice.randomize(prng0);
    //prng0.get((u8*)baseSend.data()->data(), sizeof(block) * 2 * baseSend.size());
    //for (u64 i = 0; i < baseCount; ++i)
    //{
    //    baseRecv[i] = baseSend[i][baseChoice[i]];
    //}


    oc::NaorPinkas base;
    base.receive(baseChoice, baseRecv, prng0, sendChl);

    sender.setBaseOts(baseRecv, baseChoice);

    //OT
    sender.init(numOTs, prng0, sendChl);

    // Get the random OT messages
    for (u64 i = 0; i < numOTs; i += stepSize)
    {
        auto curStepSize = std::min<u64>(stepSize, numOTs - i);
        sender.recvCorrection(sendChl, curStepSize);
        for (u64 k = 0; k < curStepSize; ++k)
        {
            sender.otCorrection(i + k);
        }
    }

}

void Sender::computeXors(){

    u64 baseCount = sender.getBaseOTCount();
    int blockSize = baseCount/128;
    vector<block> output(blockSize);

    vector<byte> temp(blockSize*16);
    int size;
    vector<int> indices(gamma+2);
    int indicesSize;
    oc::RandomOracle ro(xors2.stride());
    for (int i=0; i<hashSize; i++){

        indicesSize = dic->dec(keys[i], indices);

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(*(sender.mT.data(indices[0]) + j),*(sender.mT.data(tableRealSize + indices[1]) + j));
        }

        for (int k=2; k<indicesSize; k++) {
            for (int j = 0; j < blockSize; j++) {
                output[j] = _mm_xor_si128(output[j], *(sender.mT.data(2*tableRealSize + indices[k]) + j));
            }
        }

        std::array<block, 10> codeword = { ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock, ZeroBlock };
        memcpy(codeword.data(), vals.data() + i*fieldSizeBytes, fieldSizeBytes);
        code.encode((u8*)codeword.data(), (u8*)codeword.data());

        for (int j = 0; j < blockSize; j++) {
            codeword[j] = _mm_and_si128(codeword[j], ((block*)baseChoice.data())[j]);
        }

        for (int j=0; j<blockSize; j++) {
            output[j] = _mm_xor_si128(output[j], codeword[j]);
        }

        //EVP_EncryptUpdate(aes, temp.data(), &size, (byte*)output.data(), blockSize*16);
        ro.Reset();
        ro.Update((byte*)output.data(), blockSize * 16);
        ro.Final(xors2[i].data());
        //xors[i] = ((uint64_t*)temp.data())[0];
        //memcpy(xors2[i].data(), temp.data(), blockSize * 16);
//        for (int j=0; j<20;j++){
//            cout<<(int)(((byte*)output.data())[j])<<" ";
//        }
//        cout<<endl;
//
//        cout<<xors[i]<<endl;
    }


}

void Sender::sendXors(){

    int64_t size = xors2.rows();
    int64_t bytesPer = xors2.stride();

    otherParty->getChannel()->write((byte*)&size, 8);
    otherParty->getChannel()->write((byte*)&bytesPer, 8);
    otherParty->getChannel()->write((byte*)xors2.data(), xors2.size());

    std::cout << "sending final encoding, " << xors2.size() << " = " << size << " x " << bytesPer << std::endl;
}



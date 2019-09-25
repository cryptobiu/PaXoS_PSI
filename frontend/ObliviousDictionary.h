//
// Created by moriya on 7/17/19.
//

#ifndef BENNYPROJECT_OBLIVIOUSDICTIONARY_H
#define BENNYPROJECT_OBLIVIOUSDICTIONARY_H

#include <unordered_set>
#include <unordered_map>
#include <libscapi/include/primitives/Prg.hpp>
#include <libscapi/include/comm/MPCCommunication.hpp>
#include "Hasher.h"
#include <NTL/mat_GF2E.h>
#include <NTL/GF2E.h>
#include <NTL/GF2X.h>
#include <NTL/GF2XFactoring.h>
#include "gf2e_mat_solve.h"

#include <chrono>

using namespace std::chrono;

using namespace std;
using namespace NTL;


class ObliviousDictionary {
protected:

    int hashSize;
    int tableRealSize;
    int fieldSize, fieldSizeBytes;
    int gamma;

    PrgFromOpenSSLAES prg;
    vector<uint64_t> keys;
    vector<byte> values;

    uint64_t firstSeed, secondSeed, dhSeed;

    unordered_map<uint64_t, GF2E> vals;

    vector<uint64_t> peelingVector;
    int peelingCounter;

    int reportStatistics=0;
    ofstream statisticsFile;

    unordered_set<uint64_t, Hasher> first;
    unordered_set<uint64_t, Hasher> second;

    Hasher DH;

    GF2EVector variables;
    vector<byte> sign;


    uint64_t getDHBits(uint64_t key);

public:

    ObliviousDictionary(int hashSize, int fieldSize, int gamma);

     ~ObliviousDictionary(){
         if (reportStatistics == 1) {

             statisticsFile.close();
         }
    }

    void setKeysAndVals(vector<uint64_t>& keys, vector<byte>& values){
         this->keys = keys;
         this->values = values;

//        for (int i=0; i<hashSize; i++){
//            cout << "key = " << keys[i] << " val = ";
//
//            for (int j=0; j<fieldSizeBytes; j++){
//                cout<<(int)(values[i*fieldSizeBytes + j])<<" ";
//            }
//            cout<<endl;
//        }

     }

    vector<uint64_t> dec(uint64_t key);

    void createSets();

    void fillTables();

    void peeling();

    void generateExternalToolValues();

    void unpeeling();

    void checkOutput();

    bool hasLoop();

    void setReportStatstics(int flag){
        reportStatistics = flag;
        if (reportStatistics == 1) {

            cout<<"statistics file created"<<endl;
            statisticsFile.open("statistics.csv");
            statisticsFile << "-------------Statistics-----------.\n";
        }};

    void init();

    vector<byte> getVariables() {
        vector<byte> sigma(variables.size()*fieldSizeBytes);
        for (int i=0; i<variables.size(); i++){
//            cout<<"variables["<<i<<"] = "<<variables[i]<<endl;
            BytesFromGF2X(sigma.data() + i*fieldSizeBytes, rep(variables[i]), fieldSizeBytes);
//            for (int j=0; j<fieldSizeBytes; j++){
//                cout<<(int)*(sigma.data() + i*fieldSizeBytes + j)<< " ";
//            }
//            cout<<endl;
        }

        return sigma;
    }

    int getHashSize(){return hashSize;}
    int getTableSize(){return tableRealSize;}


};



#endif //BENNYPROJECT_OBLIVIOUSDICTIONARY_H

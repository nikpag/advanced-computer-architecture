// custom predictors implemented for 4.5

#ifndef CUSTOM_PREDICTORS_H
#define CUSTOM_PREDICTORS_H

#include "branch_predictor.h"
#include <cassert>

class StaticTakenPredictor : public BranchPredictor
{
public:
    StaticTakenPredictor() {}
    ~StaticTakenPredictor() {}

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        return true;
    }

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        updateCounters(predicted, actual);
    };

    virtual string getName() {
        return "Static Taken";
    }
};

class BTFNTPredictor : public BranchPredictor
{
public:
    BTFNTPredictor() {}
    ~BTFNTPredictor() {}

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        return ip > target;
    }

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        updateCounters(predicted, actual);
    };

    virtual string getName() {
        return "Static BTFNT";
    }
};

class TournamentPredictor : public BranchPredictor
{
public:
    TournamentPredictor(int _entries, BranchPredictor* A, BranchPredictor* B)
    : BranchPredictor(), entries(_entries)
    {
        PREDICTOR[0] = A;
        PREDICTOR[1] = B;

        // whatever, initial values are never used anyway
        prediction[0] = true;
        prediction[1] = true;

        // which one to use first -- arbitrary
        counter = new int[entries];
        memset(counter, 0, entries * sizeof(*counter));
    }

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        int cnt = counter[ip % entries];

        prediction[0] = PREDICTOR[0]->predict(ip, target);
        prediction[1] = PREDICTOR[1]->predict(ip, target);

        // 0,1 -> prediction[0] --- 2,3 -> prediction[1]
        if (cnt < 2)
            return prediction[0];
        else if (cnt < 4)
            return prediction[1];
        else {
            cerr << "ERROR, SOMETHING WENT WRONG, TRUST NOTHING BEYOND THIS LINE" << endl;
            return false;
        }
    }

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        updateCounters(predicted, actual);
        int index = ip % entries;

        // update the actual predictors
        PREDICTOR[0]->update(prediction[0], actual, ip, target);
        PREDICTOR[1]->update(prediction[1], actual, ip, target);

        if (prediction[0] == prediction[1])
            return;

        if ((prediction[0] == actual) && (counter[index] > 0))
            counter[index] --; // favour p0 for this entry
        else if (counter[ip % entries] < 3)
            counter[index] ++; // favour p1 for this entry
    }

    virtual string getName() {
        std::ostringstream stream;
        stream << "Tournament-" << entries << "entries-(" << PREDICTOR[0]->getName() << "," << PREDICTOR[1]->getName() << ")";
        return stream.str();
    }

    ~TournamentPredictor()
    {
        delete counter;
    }

private:
    BranchPredictor *PREDICTOR[2];
    bool prediction[2];
    
    int entries;
    int *counter;
};

// local history predictor
class LocalHistoryPredictor : public BranchPredictor
{
public:
    LocalHistoryPredictor(int bht_entries_, int bht_bits_, int index_bits_, int nbits)
    : BranchPredictor(), pht_entries(1 << (index_bits_+bht_bits_)), pht_bits(nbits), bht_entries(bht_entries_), bht_bits(bht_bits_) 
    {
        bhrmax = 1 << bht_bits;

        // create empty tables
        BHT = new int[bht_entries];
        for (int i = 0; i < bhrmax; i++) {
            PHT.push_back(new NbitPredictor(index_bits_, pht_bits));
        }
    }
    
    ~LocalHistoryPredictor() {
        PHT.clear();
        delete BHT;
    }

    // use the correct predictor using the local history
    virtual bool predict(ADDRINT ip, ADDRINT target) {
        int bhr = BHT[ip % bht_entries];
        return PHT[bhr]->predict(ip, target);
    }

    // update
    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        updateCounters(predicted, actual);

        // update predictor
        int & bhr = BHT[ip % bht_entries];
        PHT[bhr]->update(predicted, actual, ip, target);

        // update local history
        bhr = (bhr << 1) % bhrmax;      // shift left and drop MSB
        if (actual && bht_bits) bhr++;  // set LSB to 1 if branch was taken
    }

    virtual string getName() {
        ostringstream st;
        int size = pht_entries * pht_bits + bht_entries*bht_bits;
        size = size >> 10;
        st << "LocalHistory-Size:" << size << "K-PHT(" << pht_entries << "," << pht_bits << ")-BHT(" << bht_entries << "," << bht_bits << ")";
        return st.str();
    }

protected:
    int pht_entries, pht_bits;  // entries and bits per entry
    int bht_entries, bht_bits;  // entries and bits per entry

private:
    int *BHT;                   // use BHT[ip % bht_entries]
    vector<NbitPredictor*> PHT;  // use PHT[BHT[ip % bht_entries]]

    int bhrmax;                 // max entry for BHT
};

class GlobalHistoryPredictor : public LocalHistoryPredictor
{
public:
    GlobalHistoryPredictor(int _bhr_bits, int index_bits, int nbits)
    : LocalHistoryPredictor(1, _bhr_bits, index_bits, nbits)
    {}

    virtual string getName() {
        ostringstream st;
        int size = pht_entries * pht_bits + bht_entries*bht_bits;
        size = size >> 10;
        st << "GlobalHistory-Size:" << size << "K-PHT(" << pht_entries << "," << pht_bits << ")-BHR(" << bht_bits << ")";
        return st.str();
    }
};

#endif /* CUSTOM_PREDICTORS_H */

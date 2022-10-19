#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include <sstream> // std::ostringstream
#include <cmath>   // pow()
#include <cstring> // memset()
#include <vector>

using std::vector;

/**
 * A generic BranchPredictor base class.
 * All predictors can be subclasses with overloaded predict() and update()
 * methods.
 **/
class BranchPredictor
{
public:
    BranchPredictor() : correct_predictions(0), incorrect_predictions(0) {};
    ~BranchPredictor() {};

    virtual bool predict(ADDRINT ip, ADDRINT target) = 0;
    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) = 0;
    virtual string getName() = 0;

    UINT64 getNumCorrectPredictions() { return correct_predictions; }
    UINT64 getNumIncorrectPredictions() { return incorrect_predictions; }

    void resetCounters() { correct_predictions = incorrect_predictions = 0; };

protected:
    void updateCounters(bool predicted, bool actual) {
        if (predicted == actual)
            correct_predictions++;
        else
            incorrect_predictions++;
    };

private:
    UINT64 correct_predictions;
    UINT64 incorrect_predictions;
};

class NbitPredictor : public BranchPredictor
{
public:
    NbitPredictor(unsigned index_bits_, unsigned cntr_bits_)
        : BranchPredictor(), index_bits(index_bits_), cntr_bits(cntr_bits_) {
        table_entries = 1 << index_bits;
        TABLE = new unsigned long long[table_entries];
        memset(TABLE, 0, table_entries * sizeof(*TABLE));

        COUNTER_MAX = (1 << cntr_bits) - 1;
    };
    ~NbitPredictor() { delete TABLE; };

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        unsigned int ip_table_index = ip % table_entries;
        unsigned long long ip_table_value = TABLE[ip_table_index];
        unsigned long long prediction = ip_table_value >> (cntr_bits - 1);
        return (prediction != 0);
    };

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        unsigned int ip_table_index = ip % table_entries;
        if (actual) {
            if (TABLE[ip_table_index] < COUNTER_MAX)
                TABLE[ip_table_index]++;
        } else {
            if (TABLE[ip_table_index] > 0)
                TABLE[ip_table_index]--;
        }

        updateCounters(predicted, actual);
    };

    virtual string getName() {
        std::ostringstream stream;
        stream << "Nbit-" << pow(2.0,double(index_bits)) / 1024.0 << "K-" << cntr_bits;
        return stream.str();
    }

private:
    unsigned int index_bits, cntr_bits;
    unsigned int COUNTER_MAX;

    /* Make this unsigned long long so as to support big numbers of cntr_bits. */
    unsigned long long *TABLE;
    unsigned int table_entries;
};

//EDITED
// Implementation points:
// * predict taken for entries found in BTB, otherwise not taken
// * replace LRU address in case of conflict
// * vector used for sets with (max size == associativity)
// * vector head == LRU, vector tail == MRU
// * CorrectTargetPredictions = branch was taken and target in BTB was correct
//                            OR branch not taken and no entry in BTB
// * CorrectPredictions : prediction == actual
// * IncorrectPredictions : prediction != actual
// * The *ip* stored in the BTB need not contain the index bits. We keep them
//      anyway because it makes the implementation easier
class BTBPredictor : public BranchPredictor
{
public:
    BTBPredictor(UINT32 btb_lines, UINT32 btb_assoc)
         : table_lines(btb_lines), table_assoc(btb_assoc)
    {
        table = new BTB_SET[table_lines];
        correct_target_predictions = 0;
    }

    ~BTBPredictor() {
        delete table;
    }

    // predict taken if IP exists in the BTB
    virtual bool predict(ADDRINT ip, ADDRINT target) {
        int index = ip % table_lines;
        BTB_SET & set = table[index];

        bool found = false;
        for (BTB_SET::iterator it = set.begin(); !found && it != set.end(); it++) {
            found = (ip == (*it).ip);
        }

        return found;
    }

    // we predict taken for entries in the BTB, so predicted == entry exists
    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        // find set
        int index = ip % table_lines;
        BTB_SET & set = table[index];

        // count correct/incorrect predictions
        updateCounters(predicted, actual);

        // branch was taken and there was no entry in BTB, add it as MRU
        if (!predicted && actual) {
            // add entry as MRU
            set.push_back({ip, target});
            if (set.size() > table_assoc)
                set.erase(set.begin());
        }

        // branch was taken and there was an entry in BTB
        else if (predicted && actual) {
            // find entry
            BTB_SET::iterator entry;
            for (entry = set.begin(); entry != set.end(); entry++) {
                if ((*entry).ip == ip)
                    break;
            }

            // was target correct?
            if ((*entry).target == target) {
                correct_target_predictions++;
            }

            // make the (corrected) entry MRU
            set.erase(entry);
            set.push_back({ip, target});
        }

        // branch was not taken and there was an entry in BTB
        else if (predicted && !actual) {
            // find entry
            BTB_SET::iterator entry;
            for (entry = set.begin(); entry != set.end(); entry++) {
                if ((*entry).ip == ip)
                    break;
            }

            // remove it
            set.erase(entry);
        }

        // branch was not taken and there was no entry in BTB
        else {
            // prediction was correct, does it count as correct target prediction?
            correct_target_predictions++;
        }
    }

    virtual string getName() {
        std::ostringstream stream;
        stream << "BTB-" << table_lines << "-" << table_assoc;
        return stream.str();
    }

    UINT64 getNumCorrectTargetPredictions() {
        return correct_target_predictions;
    }

private:
    struct addr_pair {
        UINT64 ip;
        UINT64 target;
    };
    typedef vector<addr_pair> BTB_SET;
    UINT32 table_lines, table_assoc;
    BTB_SET *table;
    UINT64 correct_target_predictions;
};


#endif

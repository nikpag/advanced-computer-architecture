#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H
#define KILO 1024

#include <sstream> // std::ostringstream
#include <cmath>   // pow()
#include <cstring> // memset()

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


class TwoBitFSMPredictor: public BranchPredictor
{
public:
    TwoBitFSMPredictor(unsigned index_bits_)
        : BranchPredictor(), index_bits(index_bits_), cntr_bits(2) {
        table_entries = 1 << index_bits;
        TABLE = new unsigned long long[table_entries];
        memset(TABLE, 0, table_entries * sizeof(*TABLE));

        COUNTER_MAX = (1 << cntr_bits) - 1;
    };
    ~TwoBitFSMPredictor() { delete TABLE; };

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        unsigned int ip_table_index = ip % table_entries;
        unsigned long long ip_table_value = TABLE[ip_table_index];
        unsigned long long prediction = ip_table_value >> (cntr_bits - 1);
        return (prediction != 0);
    };

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        unsigned int ip_table_index = ip % table_entries;
        if (actual) {
            if (TABLE[ip_table_index] == 1) // State no 1 transits to 3
                TABLE[ip_table_index] = 3;
            else if (TABLE[ip_table_index] < COUNTER_MAX)
                TABLE[ip_table_index]++;
        } else {
            if (TABLE[ip_table_index] == 2) // State no 1 transits to 0
                TABLE[ip_table_index] = 0;
            else if (TABLE[ip_table_index] > 0)
                TABLE[ip_table_index]--;
        }

        updateCounters(predicted, actual);
    };

    virtual string getName() {
        std::ostringstream stream;
        stream << "2bitFSM-" << pow(2.0,double(index_bits)) / 1024.0 << "K-" << cntr_bits;
        return stream.str();
    }

private:
    unsigned int index_bits, cntr_bits;
    unsigned int COUNTER_MAX;

    /* Make this unsigned long long so as to support big numbers of cntr_bits. */
    unsigned long long *TABLE;
    unsigned int table_entries;
};


class BTBEntry
{
public:
    ADDRINT ip;
    ADDRINT target;
    UINT64 LRUCounter;
    BTBEntry()
        : ip(0), target(0), LRUCounter(0) {}
    BTBEntry(ADDRINT i, ADDRINT t, UINT64 cnt)
        : ip(i), target(t), LRUCounter(cnt) {}
};


class BTBPredictor : public BranchPredictor
{
private:
	unsigned int table_lines, table_assoc;
	unsigned int correct_target_predictions, incorrect_target_predictions;

	UINT64 timestamp;
	BTBEntry* TABLE;

	BTBEntry* find(ADDRINT ip) {
		unsigned int ip_table_index = ip % table_lines;
		for (unsigned int i = 0; i < table_assoc; i++) {
			BTBEntry* entry = &TABLE[ip_table_index * table_assoc + i];
			if (entry->ip == ip) {
				return entry;
			}
		}
		return NULL;
	};

	BTBEntry* replace(ADDRINT ip) {
		unsigned int ip_table_index = ip % table_lines;

		BTBEntry *entry;
		BTBEntry *LRUEntry = &TABLE[ip_table_index * table_assoc];
		for (unsigned int i = 0; i < table_assoc; i++) {
			entry = &TABLE[ip_table_index*table_assoc + i];

			if (LRUEntry->LRUCounter > entry->LRUCounter) {
				LRUEntry = entry;
			}
		}
		return LRUEntry;
	};

public:
	BTBPredictor(unsigned btb_lines, unsigned btb_assoc)
	    : BranchPredictor(), table_lines(btb_lines), table_assoc(btb_assoc),
        correct_target_predictions(0), incorrect_target_predictions(0), timestamp(0) {
        TABLE = new BTBEntry[table_lines*table_assoc];
	};

	~BTBPredictor() {
        delete TABLE;
	};

	virtual bool predict(ADDRINT ip, ADDRINT target) {
	    BTBEntry* entry = find(ip);
	    if (entry) {
	    	entry->LRUCounter = timestamp++;
	    	return true;
	    }
        return false;
	};

	virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
		if (actual && predicted) {
			BTBEntry* entry = find(ip);
			if (entry) {
				if (entry->target == target)
					correct_target_predictions++;
				else {
					incorrect_target_predictions++;
					entry->target = target;
				}
			}
			else {
                perror("BTB: Entry is not present although the branch is predicted taken");
			}
		}
		else if (actual && (!predicted)) {

			BTBEntry* entry = replace(ip);
			entry->ip = ip;
			entry->target = target;
			entry->LRUCounter = timestamp++;
		}
		else if ((!actual) && predicted) {

			BTBEntry* entry = find(ip);
			entry->ip = 0;
			entry->target = 0;
			entry->LRUCounter = 0;
		}

		updateCounters(predicted, actual);
	};

	virtual string getName() {
	std::ostringstream stream;
		stream << "BTB-" << table_lines << "-" << table_assoc;
		return stream.str();
	};

	virtual UINT64 getNumCorrectTargetPredictions() {
		return correct_target_predictions;
	};

	virtual UINT64 getNumIncorrectTargetPredictions() {
		return incorrect_target_predictions;
	};

};


/* Static Taken Predictor */
class StaticTakenPredictor : public BranchPredictor
{
public:
	StaticTakenPredictor():BranchPredictor() {};
	~StaticTakenPredictor() {};

	virtual bool predict(ADDRINT ip, ADDRINT target) {
		return true;
	};

	virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
		updateCounters(predicted, actual);
	};

	virtual string getName() {
		std::ostringstream stream;
		stream << "StaticTakenPredictor";
		return stream.str();
	};

};

/* Static Back Taken Forward not takes */
class BTFNTPredictor : public BranchPredictor {
public:
  BTFNTPredictor ():BranchPredictor() {};
  ~BTFNTPredictor() {};

	virtual bool predict(ADDRINT ip, ADDRINT target) {
		return target < ip;
	};

	virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
		updateCounters(predicted, actual);
	};

  	virtual string getName() {
		std::ostringstream stream;
		stream << "BTFNTPredictor";
		return stream.str();
	};

};

/* This is a parametrized tournament predictor */
class TournamentPredictor : public BranchPredictor {
public:
    TournamentPredictor(int _entries, BranchPredictor* A, BranchPredictor* B)
        : BranchPredictor(), entries(_entries) {
        PREDICTOR[0] = A;
        PREDICTOR[1] = B;

        // whatever, initial values are never used anyway
        prediction[0] = true;
        prediction[1] = true;

        counter = new int[entries]; // BHR
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

        PREDICTOR[0]->update(prediction[0], actual, ip, target);
        PREDICTOR[1]->update(prediction[1], actual, ip, target);

        if (prediction[0] == prediction[1])
            return;

        // If predictors do not agree
        if ((prediction[0] == actual) && (counter[index] > 0))
            counter[index] --; // favour p0 for this entry
        else if ((prediction[1] == actual) && (counter[index] < 3))
            counter[index] ++; // favour p1 for this entry
    }

    virtual string getName() {
        std::ostringstream stream;
        stream << "Tournament-" << entries/KILO << "K-Entries-(" << PREDICTOR[0]->getName() << "," << PREDICTOR[1]->getName() << ")";
        return stream.str();
    }

    ~TournamentPredictor() {
        delete counter;
    }

private:
    BranchPredictor *PREDICTOR[2];
    bool prediction[2];

    int entries;
    int *counter;
};


/* This is a local history predictor */
class LocalHistoryPredictor : public BranchPredictor
{
public:
    LocalHistoryPredictor(int bht_entries_, int bht_bits_, int index_bits_, int nbits)
        : BranchPredictor(), pht_entries(1 << (index_bits_+bht_bits_)), pht_bits(nbits),
        bht_entries(bht_entries_), bht_bits(bht_bits_) {

        // Branch History Register Max value (BHT means BHR)
        bhrmax = 1 << bht_bits;

        BHT = new int[bht_entries];
        for (int i = 0; i < bhrmax; i++) {
            // Construct each predictor
            PHT.push_back(new NbitPredictor(index_bits_, pht_bits));
        }
    }

    ~LocalHistoryPredictor() {
        PHT.clear();
        delete BHT;
    }

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        int bhr = BHT[ip % bht_entries];
        return PHT[bhr]->predict(ip, target);
    }

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
        updateCounters(predicted, actual);

        int & bhr = BHT[ip % bht_entries];
        PHT[bhr]->update(predicted, actual, ip, target);

        bhr = (bhr << 1) % bhrmax;      // shift left and drop MSB
        if (actual && bht_bits) bhr++;  // set LSB to 1 if branch was taken
    }

    virtual string getName() {
        ostringstream st;
        st << "LocalHistory-PHT(" << (pht_entries/KILO) << "K," << pht_bits << ")-BHT(" << (bht_entries/KILO) << "K," << bht_bits << ")";
        return st.str();
    }


protected:
    int pht_entries, pht_bits;
    int bht_entries, bht_bits;
    int *BHT;                   // use BHT[ip % bht_entries]

private:
    vector<NbitPredictor*> PHT;  // use PHT[BHT[ip % bht_entries]]
    int bhrmax;                 // max entry for BHT
};

// This is a global History Predictor
class GlobalHistoryPredictor : public LocalHistoryPredictor {
public:
    GlobalHistoryPredictor(int _bhr_bits, int index_bits, int nbits)
    : LocalHistoryPredictor(1, _bhr_bits, index_bits, nbits)
    {}

    virtual string getName() {
        ostringstream st;
        int pht_entries_kilo = pht_entries / KILO;
        st << "GlobalHistory-PHT(" << pht_entries_kilo << "K," << pht_bits << ")-BHR(" << bht_bits << ")";
        return st.str();
    }

		virtual int getBHRegister() {
			return BHT[0];
		}
};

class AlphaPredictor : public BranchPredictor
{
public:
    AlphaPredictor() : BranchPredictor() {
		choice_entries = 4*KILO; // How many entries for choice predictor
        BranchPredictor* p0 = new LocalHistoryPredictor(KILO, 10, 0, 3);
        BranchPredictor* p1 = new GlobalHistoryPredictor(12, 0, 2); // 4K 2bit proedctors
        PREDICTOR[0] = p0;
        PREDICTOR[1] = p1;

        // whatever, initial values are never used anyway
        prediction[0] = true;
        prediction[1] = true;

        counter = new int[choice_entries]; // BHR
        memset(counter, 0, choice_entries * sizeof(*counter));
    }

    virtual bool predict(ADDRINT ip, ADDRINT target) {
		int global_history =  ((GlobalHistoryPredictor*) PREDICTOR[1])->getBHRegister();
        int cnt = counter[global_history % choice_entries]; // Indexed by global history

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
		int global_history =  ((GlobalHistoryPredictor*) PREDICTOR[1])->getBHRegister();
        int index = global_history % choice_entries;

        PREDICTOR[0]->update(prediction[0], actual, ip, target);
        PREDICTOR[1]->update(prediction[1], actual, ip, target);

        if (prediction[0] == prediction[1])
            return;

        // If predictors do not agree
        if ((prediction[0] == actual) && (counter[index] > 0))
            counter[index] --; // favour p0 for this entry
        else if ((prediction[1] == actual) && (counter[index] < 3))
            counter[index] ++; // favour p1 for this entry
    }

    virtual string getName() {
        std::ostringstream stream;
        stream << "ALPHA-PREDICTOR" << PREDICTOR[0]->getName() << "," << PREDICTOR[1]->getName() << ")";
        return stream.str();
    }

    ~AlphaPredictor() {
        delete counter;
    }

private:
    BranchPredictor *PREDICTOR[2];
    bool prediction[2];
    int *counter;
    int choice_entries;
};

// BTBPredictorApostApost
// LRU replacement policy
class BTBPredictorApost : public BTBPredictor
{
public:
	BTBPredictorApost(int btb_lines, int btb_assoc)
	     : BTBPredictor(1,1), table_lines(btb_lines), table_assoc(btb_assoc)
	{
		// space allocation
		table = new std::vector<way> [btb_lines];
		// utilize also correct target prediction
		correct_target_predictions = 0;
		wrong_target_predictions = 0;
		takenBranches = 0;
	}

	~BTBPredictorApost() {
		delete[] table;
	}

    virtual bool predict(ADDRINT ip, ADDRINT target) {
    	std::vector<way>::iterator assocc_it;
		unsigned int long long ip_table_index = ip % table_lines;
		bool PC_found = false;

		// look up for PC of instruction to fetch
		for(assocc_it = table[ip_table_index].begin(); assocc_it != table[ip_table_index].end(); ++assocc_it) {
			if (ip == assocc_it->Current_PC){
				PC_found = true;
			}
		}
		// if PC found then take the branch
		return PC_found;
	}

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
    	std::vector<way>::iterator assoc_it;
    	unsigned long long int ip_table_index = ip % table_lines;


 		if ( actual ){
 			// Count total Taken Branches
 			takenBranches++;
    	 	if ( !predicted){
    			// remove the least recently used if full
    			// also we control the size of the vector (associativity)
	    		if ( ((int) table[ip_table_index].size()) == table_assoc){
					table[ip_table_index].erase(table[ip_table_index].begin() );
				}
	    		// put the taken branch in the branch (push_back so it is MRU)
	    		way new_field;
				new_field.Current_PC = ip;
				new_field.Predicted_PC = target;
				table[ip_table_index].push_back(new_field);
    			// update Counters normally
    			updateCounters(predicted, actual);
    		} else{			//if (predicted){
    			for(assoc_it = table[ip_table_index].begin(); assoc_it != table[ip_table_index].end(); ++assoc_it) {
    				if (ip == assoc_it->Current_PC){
						// check the targets
						if (target == assoc_it->Predicted_PC){
							// update correct target predictions
							correct_target_predictions++;
						} else {
							// NO // if the targets don't match make predicted false
							// so the update counters function below count this branch as a misprediction
							wrong_target_predictions++;
						}
						// make this field MRU
    					// or if the target is not the same change it to thet correct one automatically
						way new_field;
						new_field.Current_PC = ip;
						new_field.Predicted_PC = target;
						table[ip_table_index].erase(assoc_it);
						table[ip_table_index].push_back(new_field);
						// if found the field and make it MRU and make the changes so we break
						break;
    				}
    			}
				// update Counters
				//  NO! // DIRECTION prediction hit only with target match
				updateCounters(predicted, actual);
    		}
    	} else { 		// if ((!actual))

    		if (predicted){
    			// just remove the specific branch
    			for(assoc_it = table[ip_table_index].begin(); assoc_it != table[ip_table_index].end(); ++assoc_it) {
					if (ip == assoc_it->Current_PC){
    					table[ip_table_index].erase(assoc_it);
 					    break;
    				}
    			}
				// update Counters normally
    			updateCounters(predicted, actual);
    		} else {
				// update Counters normally
    			updateCounters(predicted, actual);
    		}
		}
	}

    virtual string getName() {
		stream << "Apost-BTB-" << table_lines << "-" << table_assoc <<"-Apost";
		return stream.str();
	}

    virtual UINT64 getNumCorrectTargetPredictions() {
		return correct_target_predictions;
	}
	virtual UINT64 getNumWrongTargetPredictions() {
		return 	wrong_target_predictions;
	}
	UINT64 getTakenBranches() {
		return takenBranches;
	}
private:
	 long long int table_lines, table_assoc;

	struct way {
		unsigned long long int Current_PC;
		unsigned long long int Predicted_PC;
	};
	typedef struct way way;
	std::vector<way> *table;
	UINT64 correct_target_predictions, wrong_target_predictions;
	UINT64 takenBranches;
	std::ostringstream stream;
};



#endif
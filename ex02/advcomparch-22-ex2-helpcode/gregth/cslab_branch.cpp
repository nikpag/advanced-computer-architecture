#include "pin.H"

#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;

#include "branch_predictor.h"
#include "pentium_m_predictor/pentium_m_branch_predictor.h"
#include "ras.h"

/* ===================================================================== */
/* Commandline Switches                                                  */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "cslab_branch.out", "specify output file name");
/* ===================================================================== */

/* ===================================================================== */
/* Global Variables                                                      */
/* ===================================================================== */
std::vector<BranchPredictor *> branch_predictors;
typedef std::vector<BranchPredictor *>::iterator bp_iterator_t;

//> BTBs have slightly different interface (they also have target predictions)
//  so we need to have different vector for them.
std::vector<BTBPredictor *> btb_predictors;
typedef std::vector<BTBPredictor *>::iterator btb_iterator_t;

std::vector<RAS *> ras_vec;
typedef std::vector<RAS *>::iterator ras_vec_iterator_t;

UINT64 total_instructions;
std::ofstream outFile;

/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool simulates various branch predictors.\n\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

/* ===================================================================== */

VOID count_instruction()
{
    total_instructions++;
}

VOID call_instruction(ADDRINT ip, ADDRINT target, UINT32 ins_size)
{
    ras_vec_iterator_t ras_it;

    for (ras_it = ras_vec.begin(); ras_it != ras_vec.end(); ++ras_it) {
        RAS *ras = *ras_it;
        ras->push_addr(ip + ins_size);
    }
}

VOID ret_instruction(ADDRINT ip, ADDRINT target)
{
    ras_vec_iterator_t ras_it;

    for (ras_it = ras_vec.begin(); ras_it != ras_vec.end(); ++ras_it) {
        RAS *ras = *ras_it;
        ras->pop_addr(target);
    }
}

VOID cond_branch_instruction(ADDRINT ip, ADDRINT target, BOOL taken)
{
    bp_iterator_t bp_it;
    BOOL pred;

    for (bp_it = branch_predictors.begin(); bp_it != branch_predictors.end(); ++bp_it) {
        BranchPredictor *curr_predictor = *bp_it;
        pred = curr_predictor->predict(ip, target);
        curr_predictor->update(pred, taken, ip, target);
    }
}

VOID branch_instruction(ADDRINT ip, ADDRINT target, BOOL taken)
{
    btb_iterator_t btb_it;
    BOOL pred;

    for (btb_it = btb_predictors.begin(); btb_it != btb_predictors.end(); ++btb_it) {
        BTBPredictor *curr_predictor = *btb_it;
        pred = curr_predictor->predict(ip, target);
        curr_predictor->update(pred, taken, ip, target);
    }
}

VOID Instruction(INS ins, void * v)
{
    if (INS_Category(ins) == XED_CATEGORY_COND_BR)
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)cond_branch_instruction,
                       IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                       IARG_END);
    else if (INS_IsCall(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)call_instruction,
                       IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR,
                       IARG_UINT32, INS_Size(ins), IARG_END);
    else if (INS_IsRet(ins))
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ret_instruction,
                       IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_END);

    // For BTB we instrument all branches except returns
    if (INS_IsBranch(ins) && !INS_IsRet(ins))
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)branch_instruction,
                   IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                   IARG_END);

    // Count each and every instruction
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)count_instruction, IARG_END);
}

/* ===================================================================== */

VOID Fini(int code, VOID * v)
{
    bp_iterator_t bp_it;
    btb_iterator_t btb_it;
    ras_vec_iterator_t ras_it;

    // Report total instructions and total cycles
    outFile << "Total Instructions: " << total_instructions << "\n";
    outFile << "\n";

    outFile <<"RAS: (Correct - Incorrect)\n";
    for (ras_it = ras_vec.begin(); ras_it != ras_vec.end(); ++ras_it) {
        RAS *ras = *ras_it;
        outFile << ras->getNameAndStats() << "\n";
    }
    outFile << "\n";

    outFile <<"Branch Predictors: (Name - Correct - Incorrect)\n";
    for (bp_it = branch_predictors.begin(); bp_it != branch_predictors.end(); ++bp_it) {
        BranchPredictor *curr_predictor = *bp_it;
        outFile << "  " << curr_predictor->getName() << ": "
                << curr_predictor->getNumCorrectPredictions() << " "
                << curr_predictor->getNumIncorrectPredictions() << "\n";
    }
    outFile << "\n";

    outFile <<"BTB Predictors: (Name - Correct - Incorrect - TargetCorrect - TargetIncorrect)\n";
    for (btb_it = btb_predictors.begin(); btb_it != btb_predictors.end(); ++btb_it) {
        BTBPredictor *curr_predictor = *btb_it;
        outFile << "  " << curr_predictor->getName() << ": "
                << curr_predictor->getNumCorrectPredictions() << " "
                << curr_predictor->getNumIncorrectPredictions() << " "
                << curr_predictor->getNumCorrectTargetPredictions() << " "
                << curr_predictor->getNumIncorrectTargetPredictions() << "\n";
    }

    outFile.close();
}

/* ===================================================================== */

VOID InitPredictors()
{
    // N-bit predictors
    for (int i=1; i <= 4; i++) {
        NbitPredictor *nbitPred = new NbitPredictor(14, i);
        branch_predictors.push_back(nbitPred);
    }
    TwoBitFSMPredictor *twoBitFSMPred = new TwoBitFSMPredictor(14);
    branch_predictors.push_back(twoBitFSMPred);

    NbitPredictor *nbitPred = new NbitPredictor(15, 1);
    branch_predictors.push_back(nbitPred);

    nbitPred = new NbitPredictor(13, 4);
    branch_predictors.push_back(nbitPred);


    // Pentium-M predictor
    PentiumMBranchPredictor *pentiumPredictor = new PentiumMBranchPredictor();
    branch_predictors.push_back(pentiumPredictor);


    /**** 4-3: BTB  *********/
    BTBPredictor *btb = new BTBPredictor(512, 1);
    btb_predictors.push_back(btb);

    btb = new BTBPredictor(256, 2);
    btb_predictors.push_back(btb);

    btb = new BTBPredictor(128, 2);
    btb_predictors.push_back(btb);

    btb = new BTBPredictor(64, 4);
    btb_predictors.push_back(btb);

    btb = new BTBPredictor(32, 4);
    btb_predictors.push_back(btb);

    btb = new BTBPredictor(8, 8);
    btb_predictors.push_back(btb);

    /********* 4.5 *************/
    BranchPredictor* bp = new StaticTakenPredictor();
    branch_predictors.push_back(bp);

    bp = new BTFNTPredictor();
    branch_predictors.push_back(bp);

    bp = new PentiumMBranchPredictor();
    branch_predictors.push_back(bp);

    bp = new NbitPredictor(13, 4);
    branch_predictors.push_back(bp);


    // PHT Hardware is fixed. 16K. Another 16K for BHT
    int pht_bits = 2;
    int bht_entries = 2*KILO; // 2048 entries
    int bht_bits = 8;
    int pht_index = 5;
    bp = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);


    /* Another 16K hardware */
    pht_bits = 2;
    bht_entries = 4*KILO;
    bht_bits = 4;
    pht_index = 9;
    bp = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);


    /* GLOBAL PREDICTOR - PHT Hardware = 32 K*/
    bht_bits = 5;

    pht_bits = 2;
    pht_index = 9;
    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    pht_bits = 4;
    pht_index = 8;
    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    bht_bits = 10;
    pht_bits = 4;
    pht_index = 3;
    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    pht_bits = 2;
    pht_index = 4;
    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    BranchPredictor *p0 = new NbitPredictor(13, 2); // 16K Overhead
    BranchPredictor *p1 = new NbitPredictor(12, 4); // 16K Overhead
    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    p0 = new GlobalHistoryPredictor(2, 10,4);
    p1 = new NbitPredictor(13, 2);
    bp = new TournamentPredictor(2048, p0, p1);
    branch_predictors.push_back(bp);

    p0 = new GlobalHistoryPredictor(2, 11, 2); // 4* 4K = 16K Overhead
    p1 = new LocalHistoryPredictor(4*KILO, 2, 9, 4); // 8K BHT, 4*2K=8K PHT
    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    p0 = new NbitPredictor(13, 2); // 16K Overhead
    p1 = new GlobalHistoryPredictor(5, 8, 2); // 8K BHT, 4*2K=8K PHT
    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    p0 = new NbitPredictor(13, 2); // 16K Overhead
    p1 = new GlobalHistoryPredictor(5, 8, 2); // 8K BHT, 4*2K=8K PHT
    bp = new TournamentPredictor(2048, p0, p1);
    branch_predictors.push_back(bp);

    p0 = new NbitPredictor(11, 8); // 16K Overhead
    p1 = new LocalHistoryPredictor(4*KILO, 2, 9, 4); // 8K BHT, 4*2K=8K PHT
    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    bp = new AlphaPredictor();
    branch_predictors.push_back(bp);

}

VOID InitRas()
{
    for (UINT32 i = 4; i <= 64; i*=2);
        //ras_vec.push_back(new RAS(i));
}

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if(PIN_Init(argc,argv))
        return Usage();

    // Open output file
    outFile.open(KnobOutputFile.Value().c_str());

    // Initialize predictors and RAS vector
    InitPredictors();
    InitRas();

    // Instrument function calls in order to catch __parsec_roi_{begin,end}
    INS_AddInstrumentFunction(Instruction, 0);

    // Called when the instrumented application finishes its execution
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
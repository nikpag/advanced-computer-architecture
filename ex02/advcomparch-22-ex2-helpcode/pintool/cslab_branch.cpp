#include "pin.H"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

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

// This is used to suppress "unused variable" warnings
VOID doNothing(void *ptr) {
    return;
}

VOID doNothing(int x) {
    return;
}

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

    outFile <<"BTB Predictors: (Name - Correct - Incorrect - TargetCorrect)\n";
    for (btb_it = btb_predictors.begin(); btb_it != btb_predictors.end(); ++btb_it) {
        BTBPredictor *curr_predictor = *btb_it;
        outFile << "  " << curr_predictor->getName() << ": "
                << curr_predictor->getNumCorrectPredictions() << " "
                << curr_predictor->getNumIncorrectPredictions() << " "
                << curr_predictor->getNumCorrectTargetPredictions() << "\n";
    }

    outFile.close();
}

/* ===================================================================== */

VOID InitPredictors()
{
    //// Declarations ////
    NbitPredictor *nbitPred = nullptr;
    AlternateFSMPredictor *altFsmPred = nullptr;
    BTBPredictor *btb = nullptr;
    BranchPredictor *bp = nullptr;
    BranchPredictor *p0 = nullptr, *p1 = nullptr;

    doNothing(nbitPred);
    doNothing(altFsmPred);
    doNothing(btb);
    doNothing(bp);
    doNothing(p0);
    doNothing(p1);

    int pht_bits = 0;
    int pht_index = 0;
    int bht_entries = 0;
    int bht_bits = 0;

    doNothing(pht_bits);
    doNothing(pht_index);
    doNothing(bht_entries);
    doNothing(bht_bits);

    //// 4.2 ////

    // i //

    // // BHT entries = 16K, N = 1
    // nbitPred = new NbitPredictor(14, 1);
    // branch_predictors.push_back(nbitPred);

    // // BHT entries = 16K, N = 2
    // nbitPred = new NbitPredictor(14, 2);
    // branch_predictors.push_back(nbitPred);

    // // BHT entries = 16K, N = 2b (alternate FSM)
    // altFsmPred = new AlternateFSMPredictor(14);
    // branch_predictors.push_back(altFsmPred);

    // // BHT entries = 16K, N = 3
    // nbitPred = new NbitPredictor(14, 3);
    // branch_predictors.push_back(nbitPred);

    // // BHT entries = 16K, N = 4
    // nbitPred = new NbitPredictor(14, 4);
    // branch_predictors.push_back(nbitPred);

    // ii //

    // // BHT entries = 32K, N = 1
    // nbitPred = new NbitPredictor(15, 1);
    // branch_predictors.push_back(nbitPred);

    // // BHT entries = 16K, N = 2
    // nbitPred = new NbitPredictor(14, 2);
    // branch_predictors.push_back(nbitPred);

    // // BHT entries = 16K, N = 2b (alternate FSM)
    // altFsmPred = new AlternateFSMPredictor(14);
    // branch_predictors.push_back(altFsmPred);

    // // BHT entries = 8K,  N = 4
    // nbitPred = new NbitPredictor(13, 4);
    // branch_predictors.push_back(nbitPred);

    //// 4.3 ////

    // // BTB entries = 512, BTB associativity = 1
    // btb = new BTBPredictor(512, 1);
    // btb_predictors.push_back(btb);

    // // BTB entries = 512, BTB associativity = 2
    // btb = new BTBPredictor(512, 2);
    // btb_predictors.push_back(btb);

    // // BTB entries = 256, BTB associativity = 2
    // btb = new BTBPredictor(256, 2);
    // btb_predictors.push_back(btb);

    // // BTB entries = 256, BTB associativity = 4
    // btb = new BTBPredictor(256, 4);
    // btb_predictors.push_back(btb);

    // // BTB entries = 128, BTB associativity = 2
    // btb = new BTBPredictor(128, 2);
    // btb_predictors.push_back(btb);

    // // BTB entries = 128, BTB associativity = 4
    // btb = new BTBPredictor(128, 4);
    // btb_predictors.push_back(btb);

    // // BTB entries = 64, BTB associativity = 4
    // btb = new BTBPredictor(64, 4);
    // btb_predictors.push_back(btb);

    // // BTB entries = 64, BTB associativity = 8
    // btb = new BTBPredictor(64, 8);
    // btb_predictors.push_back(btb);

    //// 4.5 ////

    // Static AlwaysTaken
    bp = new StaticTakenPredictor();
    branch_predictors.push_back(bp);

    // Static BTFNT
    bp = new BTFNTPredictor();
    branch_predictors.push_back(bp);

    // Nbit-8K-4bit
    bp = new NbitPredictor(13, 4);
    branch_predictors.push_back(bp);

    // Pentium M
    bp = new PentiumMBranchPredictor();
    branch_predictors.push_back(bp);

    // Local history predictor #1
        // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 4 = 9
        // PHT N-bit counter length = 2
        // BHT entries = 4K
        // BHT entry length = 4
    pht_index = 9;
    pht_bits = 2;
    bht_entries = 4*KILO;
    bht_bits = 4;

    bp = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Local history predictor #2
        // PHT entries = 8K  --> pht_index = log(pht entries) - bht entry length = log(2^13) - 2 = 11
        // PHT N-bit counter length = 2
        // BHT entries = 8K
        // BHT entry length = 2
    pht_index = 11;
    pht_bits = 2;
    bht_entries = 8*KILO;
    bht_bits = 2;

    bp = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Global history predictor #1
        // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 4 = 9
        // PHT N-bit counter length = 2
        // BHR length = 4
    pht_index = 9;
    pht_bits = 2;
    bht_bits = 4;

    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Global history predictor #2
        // PHT entries = 16K --> pht_index = log(pht entries) - bht entry length = log(2^14) - 8 = 6
        // PHT N-bit counter length = 2
        // BHR length = 8
    pht_index = 6;
    pht_bits = 2;
    bht_bits = 8;

    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Global history predictor #3
        // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 4 = 9
        // PHT N-bit counter length = 4
        // BHR length = 4
    pht_index = 9;
    pht_bits = 4;
    bht_bits = 4;

    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Global history predictor #4
        // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 8 = 5
        // PHT N-bit counter length = 4
        // BHR length = 8
    pht_index = 5;
    pht_bits = 4;
    bht_bits = 8;

    bp = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);
    branch_predictors.push_back(bp);

    // Alpha 21264 predictor
    bp = new AlphaPredictor();
    branch_predictors.push_back(bp);

    // Tournament Hybrid predictor #1
        // meta predictor M is a 2-bit predictor with 1024 entries
        // P0 is an N-bit predictor with:
            // BHT entries = 8K
            // N = 2
        // P1 is also an N-bit predictor with:
            // BHT entries = 4K
            // N = 4

    p0 = new NbitPredictor(13, 2);

    p1 = new NbitPredictor(12, 4);

    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    // Tournament Hybrid predictor #2
        // meta predictor M is a 2-bit predictor with 2048 entries
        // P0 is a local history predictor with:
            // PHT entries = 4K --> pht_index = log(pht entries) - bht entry length = log(2^12) - 2 = 10
            // PHT N-bit counter length = 2 bit
            // BHT entries = 4K
            // BHT entry length = 2 bit
        // P1 is an N-bit predictor with:
            // BHT entries = 8K
            // N = 2

    pht_index = 10;
    pht_bits = 2;
    bht_entries = 4*KILO;
    bht_bits = 2;

    p0 = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);

    p1 = new NbitPredictor(13, 2);

    bp = new TournamentPredictor(2048, p0, p1);
    branch_predictors.push_back(bp);

    // Tournament Hybrid predictor #3
        // meta predictor M is a 2-bit predictor with 1024 entries
        // P0 is a global history predictor with:
            // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 8 = 5
            // PHT N-bit counter length = 2
            // BHR length = 8
        // P1 is an N-bit predictor with:
            // BHT entries = 4K
            // N = 4

    pht_index = 5;
    pht_bits = 2;
    bht_bits = 8;

    p0 = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);

    p1 = new NbitPredictor(12, 4);

    bp = new TournamentPredictor(1024, p0, p1);
    branch_predictors.push_back(bp);

    // Tournament Hybrid predictor #4
        // meta predictor M is a 2-bit predictor with 2048 entries
        // P0 is a local history predictor with:
            // PHT entries = 8K --> pht_index = log(pht entries) - bht entry length = log(2^13) - 2 = 11
            // PHT N-bit counter length = 1
            // BHT entries = 4K
            // BHT entry length = 2
        // P1 is a global history predictor with:
            // PHT entries = 4K --> pht_index = log(pht entries) - bht entry length = log(2^12) - 4 = 8
            // PHT N-bit counter length = 4
            // BHR length = 4

    pht_index = 11;
    pht_bits = 1;
    bht_entries = 4*KILO;
    bht_bits = 2;

    p0 = new LocalHistoryPredictor(bht_entries, bht_bits, pht_index, pht_bits);

    pht_index = 8;
    pht_bits = 4;
    bht_bits = 4;

    p1 = new GlobalHistoryPredictor(bht_bits, pht_index, pht_bits);

    bp = new TournamentPredictor(2048, p0, p1);
    branch_predictors.push_back(bp);

    //// Helpcode version ////

    // N-bit predictors
    // for (int i = 1; i <= 7; i++) {
    //     NbitPredictor *nbitPred = new NbitPredictor(14, i);
    //     branch_predictors.push_back(nbitPred);
    // }
    // NbitPredictor *nbitPred = new NbitPredictor(15, 1);
    // branch_predictors.push_back(nbitPred);
    // nbitPred = new NbitPredictor(13, 4);
    // branch_predictors.push_back(nbitPred);

    // // Pentium-M predictor
    // PentiumMBranchPredictor *pentiumPredictor = new PentiumMBranchPredictor();
    // branch_predictors.push_back(pentiumPredictor);
}

VOID InitRas()
{
    //// 4.4 ////
    // ras_vec.push_back(new RAS(4));
    // ras_vec.push_back(new RAS(8));
    // ras_vec.push_back(new RAS(16));
    // ras_vec.push_back(new RAS(32));
    // ras_vec.push_back(new RAS(48));
    // ras_vec.push_back(new RAS(64));
}

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if(PIN_Init(argc,argv))
        return Usage();

    // Open output file
    outFile.open(KnobOutputFile.Value().c_str());

    // Initialize predictors and RAS vector

    //// 4.2, 4.3, 4.5 ////
    InitPredictors();

    //// 4.4 ////
    // InitRas();

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

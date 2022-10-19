#include "pin.H"

#include <iostream>
#include <fstream>
#include <cassert>

#include "branch_predictor.h"
#include "custom_predictors.h"
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
    //FIXME
    /*
    ras_vec_iterator_t ras_it;

    for (ras_it = ras_vec.begin(); ras_it != ras_vec.end(); ++ras_it) {
        RAS *ras = *ras_it;
        ras->push_addr(ip + ins_size);
    }
    */
}

VOID ret_instruction(ADDRINT ip, ADDRINT target)
{
    //FIXME
    /*
    ras_vec_iterator_t ras_it;

    for (ras_it = ras_vec.begin(); ras_it != ras_vec.end(); ++ras_it) {
        RAS *ras = *ras_it;
        ras->pop_addr(target);
    }
    */
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
    /*
    btb_iterator_t btb_it;
    BOOL pred;

    for (btb_it = btb_predictors.begin(); btb_it != btb_predictors.end(); ++btb_it) {
        BTBPredictor *curr_predictor = *btb_it;
        pred = curr_predictor->predict(ip, target);
        curr_predictor->update(pred, taken, ip, target);
    }
    */
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
    else if (INS_IsRet(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ret_instruction,
                       IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_END);
    }

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
    //EDITED
    // predictors for 4.5d (tournament with local and global history)
    // add them as separate as well to cross check results
    branch_predictors.push_back(new GlobalHistoryPredictor(4, 9, 2));
    branch_predictors.push_back(new LocalHistoryPredictor(2048, 4, 8, 2));

    BranchPredictor *a = new GlobalHistoryPredictor(4, 9, 2);
    BranchPredictor *b = new LocalHistoryPredictor(2048, 4, 8, 2);
    branch_predictors.push_back(new TournamentPredictor(512, a, b));

    /*
    // predictors for 4.5c (local history)
    branch_predictors.push_back(new LocalHistoryPredictor(2048, 8, 5, 2));
    branch_predictors.push_back(new LocalHistoryPredictor(4096, 4, 9, 2));

    // correct predictors for 4.5b (using correct history predictors)
    branch_predictors.push_back(new GlobalHistoryPredictor(4, 10, 2));
    branch_predictors.push_back(new GlobalHistoryPredictor(4, 9, 4));
    branch_predictors.push_back(new GlobalHistoryPredictor(8, 6, 2));
    branch_predictors.push_back(new GlobalHistoryPredictor(8, 5, 4));

    // tournament predictor with nbit and globalhistory
    // add them as separate predictors as well, to make sure tournament works better
    branch_predictors.push_back(new NbitPredictor(14,1));
    branch_predictors.push_back(new GlobalHistoryPredictor(4, 9, 2));

    BranchPredictor * a = new NbitPredictor(14,1);
    BranchPredictor * b = new GlobalHistoryPredictor(4, 9, 2);
    branch_predictors.push_back(new TournamentPredictor(512, a, b));
    //Predictors for 4.5a
    branch_predictors.push_back(new StaticTakenPredictor());
    branch_predictors.push_back(new BTFNTPredictor());

    // N-bit predictors
    for (int i=1; i <= 7; i++) {
        NbitPredictor *nbitPred = new NbitPredictor(14, i);
        branch_predictors.push_back(nbitPred);
    }
    NbitPredictor *nbitPred = new NbitPredictor(15, 1);
    branch_predictors.push_back(nbitPred);
    nbitPred = new NbitPredictor(13, 4);
    branch_predictors.push_back(nbitPred);

    // Pentium-M predictor
    PentiumMBranchPredictor *pentiumPredictor = new PentiumMBranchPredictor();
    branch_predictors.push_back(pentiumPredictor);
    */
}

VOID InitRas()
{
    //EDITED
    // changed sizes to the ones needed for 4.4
    for (UINT32 i = 1; i <= 128; i*=2) {
        if (i == 32 || i == 8) continue;
        ras_vec.push_back(new RAS(i));
    }
}

VOID InitBTB()
{
    //EDITED
    // create BTBs for 4.3
    for (UINT32 assoc = 1; assoc <= 8; assoc *= 2) {
        btb_predictors.push_back(new BTBPredictor(512/assoc, assoc));
    }
}

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if(PIN_Init(argc,argv))
        return Usage();

    // Open output file
    outFile.open(KnobOutputFile.Value().c_str());

    //FIXME
    //EDITED
    // Initialize predictors and RAS vector
    InitPredictors();
    //InitRas();

    // Initialize BTB
    //InitBTB();

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

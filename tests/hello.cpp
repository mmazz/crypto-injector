#include "pin.H"
#include <iostream>

VOID Instruction(INS ins, VOID *v)
{
    std::cerr << "INS @ " << std::hex << INS_Address(ins) << std::endl;
}

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return 1;

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_StartProgram();
    return 0;
}


#include <iostream>
#include "pin.H"

using namespace std;

VOID Fini(INT32 code, VOID *v) {
    cout << "=== Pin tool finished ===" << endl;
}

INT32 Usage() {
    cerr << "This is a basic Pin tool example" << endl;
    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc, char *argv[]) {
    if (PIN_Init(argc, argv)) return Usage();

    cout << "=== Pin tool started ===" << endl;

    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();

    return 0;
}

#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

using std::string;
using std::map;
using std::set;
using std::vector;
using std::pair;

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

// Tipos de instrucciones aritméticas
enum ArithType {
    ARITH_ADD,
    ARITH_SUB,
    ARITH_MUL,
    ARITH_DIV,
    ARITH_INC,
    ARITH_DEC,
    ARITH_NEG,
    ARITH_IMUL,
    ARITH_IDIV,
    // SIMD Integer
    ARITH_SIMD_ADD,
    ARITH_SIMD_SUB,
    ARITH_SIMD_MUL,
    // AVX/SSE Floating Point
    ARITH_SSE_ADD,
    ARITH_SSE_SUB,
    ARITH_SSE_MUL,
    ARITH_SSE_DIV,
    ARITH_AVX_ADD,
    ARITH_AVX_SUB,
    ARITH_AVX_MUL,
    ARITH_AVX_DIV,
    // FPU
    ARITH_FPU_ADD,
    ARITH_FPU_SUB,
    ARITH_FPU_MUL,
    ARITH_FPU_DIV,
    ARITH_UNKNOWN
};

// Mapa de nombres para cada tipo
const char* ArithTypeNames[] = {
    "ADD", "SUB", "MUL", "DIV", "INC", "DEC", "NEG", "IMUL", "IDIV",
    "SIMD_ADD", "SIMD_SUB", "SIMD_MUL",
    "SSE_ADD", "SSE_SUB", "SSE_MUL", "SSE_DIV",
    "AVX_ADD", "AVX_SUB", "AVX_MUL", "AVX_DIV",
    "FPU_ADD", "FPU_SUB", "FPU_MUL", "FPU_DIV",
    "UNKNOWN"
};

// Contadores por función
struct FunctionStats {
    string name;
    ADDRINT address;
    map<ArithType, UINT64> arithCounts;
    UINT64 totalArithInstructions;
    bool isInlined;

    FunctionStats() : address(0), totalArithInstructions(0), isInlined(false) {}
};

// Contexto de llamada para rastrear jerarquías
struct CallContext {
    string functionName;
    ADDRINT callSite;
    UINT32 depth;
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Mapa de funciones: address -> FunctionStats
map<ADDRINT, FunctionStats> functionStatsMap;

// Conjunto de funciones de interés (filtro)
set<string> functionsOfInterest;

// Mapa de nombres de funciones: address -> name
map<ADDRINT, string> functionNames;

// Stack de llamadas para rastrear jerarquía
vector<CallContext> callStack;

// Mapa para rastrear funciones inline
map<ADDRINT, set<string>> inlinedFunctionsMap;

// Archivo de salida
std::ofstream outFile;

// Opciones de línea de comandos
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "arithmetic_profile.txt", "Archivo de salida");

KNOB<string> KnobFunctionFilter(KNOB_MODE_APPEND, "pintool",
    "f", "", "Función a instrumentar (puede especificarse múltiples veces)");

KNOB<BOOL> KnobVerbose(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "Modo verbose para debugging");

KNOB<BOOL> KnobTrackCallHierarchy(KNOB_MODE_WRITEONCE, "pintool",
    "track", "1", "Rastrear jerarquía de llamadas");

KNOB<BOOL> KnobIncludeLibraries(KNOB_MODE_WRITEONCE, "pintool",
    "l", "0", "Incluir bibliotecas dinámicas en la instrumentación");

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

// Clasificar una instrucción aritmética
ArithType ClassifyArithmeticInstruction(INS ins) {
    OPCODE opcode = INS_Opcode(ins);

    // Instrucciones enteras básicas
    if (opcode == XED_ICLASS_ADD) return ARITH_ADD;
    if (opcode == XED_ICLASS_SUB) return ARITH_SUB;
    if (opcode == XED_ICLASS_MUL) return ARITH_MUL;
    if (opcode == XED_ICLASS_DIV) return ARITH_DIV;
    if (opcode == XED_ICLASS_INC) return ARITH_INC;
    if (opcode == XED_ICLASS_DEC) return ARITH_DEC;
    if (opcode == XED_ICLASS_NEG) return ARITH_NEG;
    if (opcode == XED_ICLASS_IMUL) return ARITH_IMUL;
    if (opcode == XED_ICLASS_IDIV) return ARITH_IDIV;

    // SIMD Integer (SSE2/AVX2)
    if (opcode == XED_ICLASS_PADDB || opcode == XED_ICLASS_PADDW ||
        opcode == XED_ICLASS_PADDD || opcode == XED_ICLASS_PADDQ ||
        opcode == XED_ICLASS_VPADDB || opcode == XED_ICLASS_VPADDW ||
        opcode == XED_ICLASS_VPADDD || opcode == XED_ICLASS_VPADDQ)
        return ARITH_SIMD_ADD;

    if (opcode == XED_ICLASS_PSUBB || opcode == XED_ICLASS_PSUBW ||
        opcode == XED_ICLASS_PSUBD || opcode == XED_ICLASS_PSUBQ ||
        opcode == XED_ICLASS_VPSUBB || opcode == XED_ICLASS_VPSUBW ||
        opcode == XED_ICLASS_VPSUBD || opcode == XED_ICLASS_VPSUBQ)
        return ARITH_SIMD_SUB;

    if (opcode == XED_ICLASS_PMULLW || opcode == XED_ICLASS_PMULLD ||
        opcode == XED_ICLASS_VPMULLW || opcode == XED_ICLASS_VPMULLD ||
        opcode == XED_ICLASS_PMULUDQ || opcode == XED_ICLASS_VPMULUDQ)
        return ARITH_SIMD_MUL;

    // SSE Floating Point
    if (opcode == XED_ICLASS_ADDSS || opcode == XED_ICLASS_ADDSD ||
        opcode == XED_ICLASS_ADDPS || opcode == XED_ICLASS_ADDPD)
        return ARITH_SSE_ADD;

    if (opcode == XED_ICLASS_SUBSS || opcode == XED_ICLASS_SUBSD ||
        opcode == XED_ICLASS_SUBPS || opcode == XED_ICLASS_SUBPD)
        return ARITH_SSE_SUB;

    if (opcode == XED_ICLASS_MULSS || opcode == XED_ICLASS_MULSD ||
        opcode == XED_ICLASS_MULPS || opcode == XED_ICLASS_MULPD)
        return ARITH_SSE_MUL;

    if (opcode == XED_ICLASS_DIVSS || opcode == XED_ICLASS_DIVSD ||
        opcode == XED_ICLASS_DIVPS || opcode == XED_ICLASS_DIVPD)
        return ARITH_SSE_DIV;

    // AVX Floating Point
    if (opcode == XED_ICLASS_VADDSS || opcode == XED_ICLASS_VADDSD ||
        opcode == XED_ICLASS_VADDPS || opcode == XED_ICLASS_VADDPD)
        return ARITH_AVX_ADD;

    if (opcode == XED_ICLASS_VSUBSS || opcode == XED_ICLASS_VSUBSD ||
        opcode == XED_ICLASS_VSUBPS || opcode == XED_ICLASS_VSUBPD)
        return ARITH_AVX_SUB;

    if (opcode == XED_ICLASS_VMULSS || opcode == XED_ICLASS_VMULSD ||
        opcode == XED_ICLASS_VMULPS || opcode == XED_ICLASS_VMULPD)
        return ARITH_AVX_MUL;

    if (opcode == XED_ICLASS_VDIVSS || opcode == XED_ICLASS_VDIVSD ||
        opcode == XED_ICLASS_VDIVPS || opcode == XED_ICLASS_VDIVPD)
        return ARITH_AVX_DIV;

    // FPU x87
    if (opcode == XED_ICLASS_FADD || opcode == XED_ICLASS_FADDP ||
        opcode == XED_ICLASS_FIADD)
        return ARITH_FPU_ADD;

    if (opcode == XED_ICLASS_FSUB || opcode == XED_ICLASS_FSUBP ||
        opcode == XED_ICLASS_FISUB || opcode == XED_ICLASS_FSUBR ||
        opcode == XED_ICLASS_FSUBRP)
        return ARITH_FPU_SUB;

    if (opcode == XED_ICLASS_FMUL || opcode == XED_ICLASS_FMULP ||
        opcode == XED_ICLASS_FIMUL)
        return ARITH_FPU_MUL;

    if (opcode == XED_ICLASS_FDIV || opcode == XED_ICLASS_FDIVP ||
        opcode == XED_ICLASS_FIDIV || opcode == XED_ICLASS_FDIVR ||
        opcode == XED_ICLASS_FDIVRP)
        return ARITH_FPU_DIV;

    return ARITH_UNKNOWN;
}

// Determinar si una instrucción es aritmética
bool IsArithmeticInstruction(INS ins) {
    return ClassifyArithmeticInstruction(ins) != ARITH_UNKNOWN;
}

// Obtener el nombre demangled de una función
string GetDemangledName(const string& mangledName) {
    string demangled = mangledName;

    #if defined(TARGET_LINUX) || defined(TARGET_MAC)
    // Intentar demangle para C++
    PIN_UndecorateSymbolName(mangledName, UNDECORATION_COMPLETE);
    #endif

    return demangled;
}

// Verificar si una función está en el conjunto de interés
bool IsFunctionOfInterest(const string& funcName) {
    if (functionsOfInterest.empty()) {
        return true; // Sin filtro, todas las funciones son de interés
    }

    // Búsqueda exacta
    if (functionsOfInterest.find(funcName) != functionsOfInterest.end()) {
        return true;
    }

    // Búsqueda por substring (para funciones con namespaces)
    for (const auto& target : functionsOfInterest) {
        if (funcName.find(target) != string::npos) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// FUNCIONES DE ANÁLISIS (CALLBACKS)
// ============================================================================

// Callback para contar instrucciones aritméticas
VOID CountArithmeticInstruction(ADDRINT funcAddr, ArithType type) {
    FunctionStats& stats = functionStatsMap[funcAddr];
    stats.arithCounts[type]++;
    stats.totalArithInstructions++;
}

// Callback para entrada de función
VOID FunctionEntry(ADDRINT funcAddr, ADDRINT callSite) {
    if (KnobTrackCallHierarchy.Value()) {
        CallContext ctx;
        ctx.functionName = functionNames[funcAddr];
        ctx.callSite = callSite;
        ctx.depth = callStack.size();
        callStack.push_back(ctx);

        if (KnobVerbose.Value()) {
            std::cerr << string(ctx.depth * 2, ' ')
                      << "-> " << ctx.functionName
                      << " @ 0x" << std::hex << funcAddr << std::dec
                      << std::endl;
        }
    }
}

// Callback para salida de función
VOID FunctionExit(ADDRINT funcAddr) {
    if (KnobTrackCallHierarchy.Value() && !callStack.empty()) {
        callStack.pop_back();
    }
}

// ============================================================================
// INSTRUMENTACIÓN
// ============================================================================

// Instrumentar una rutina (función)
VOID InstrumentRoutine(RTN rtn, VOID *v) {
    RTN_Open(rtn);

    string rtnName = RTN_Name(rtn);
    ADDRINT rtnAddr = RTN_Address(rtn);
    IMG img = SEC_Img(RTN_Sec(rtn));
    string imgName = IMG_Name(img);

    // Filtrar bibliotecas si es necesario
    if (!KnobIncludeLibraries.Value() && IMG_Type(img) == IMG_TYPE_SHAREDLIB) {
        RTN_Close(rtn);
        return;
    }

    // Filtrar por funciones de interés
    if (!IsFunctionOfInterest(rtnName)) {
        RTN_Close(rtn);
        return;
    }

    // Inicializar estadísticas de la función
    if (functionStatsMap.find(rtnAddr) == functionStatsMap.end()) {
        FunctionStats stats;
        stats.name = rtnName;
        stats.address = rtnAddr;
        functionStatsMap[rtnAddr] = stats;
        functionNames[rtnAddr] = rtnName;

        if (KnobVerbose.Value()) {
            std::cerr << "Instrumentando función: " << rtnName
                      << " @ 0x" << std::hex << rtnAddr << std::dec
                      << " en " << imgName << std::endl;
        }
    }

    // Instrumentar entrada y salida de función
    if (KnobTrackCallHierarchy.Value()) {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)FunctionEntry,
                      IARG_ADDRINT, rtnAddr,
                      IARG_RETURN_IP,
                      IARG_END);

        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)FunctionExit,
                      IARG_ADDRINT, rtnAddr,
                      IARG_END);
    }

    // Instrumentar cada instrucción en la rutina
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
        if (IsArithmeticInstruction(ins)) {
            ArithType type = ClassifyArithmeticInstruction(ins);

            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountArithmeticInstruction,
                          IARG_ADDRINT, rtnAddr,
                          IARG_UINT32, type,
                          IARG_END);
        }
    }

    RTN_Close(rtn);
}

// Instrumentar imágenes cargadas (para manejar bibliotecas dinámicas)
VOID ImageLoad(IMG img, VOID *v) {
    if (KnobVerbose.Value()) {
        std::cerr << "Cargando imagen: " << IMG_Name(img)
                  << " @ 0x" << std::hex << IMG_LowAddress(img) << std::dec
                  << std::endl;
    }

    // Instrumentar todas las rutinas en la imagen
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
            InstrumentRoutine(rtn, v);
        }
    }
}

// Manejar llamadas indirectas (punteros a función, tablas virtuales)
VOID InstrumentTrace(TRACE trace, VOID *v) {
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
            // Detectar llamadas indirectas
            if (INS_IsIndirectControlFlow(ins)) {
                if (KnobVerbose.Value()) {
                    std::cerr << "Llamada indirecta detectada @ 0x"
                              << std::hex << INS_Address(ins) << std::dec
                              << std::endl;
                }
            }
        }
    }
}

// ============================================================================
// SALIDA DE RESULTADOS
// ============================================================================

// Comparador para ordenar funciones por total de instrucciones aritméticas
bool CompareFunctionStats(const pair<ADDRINT, FunctionStats>& a,
                         const pair<ADDRINT, FunctionStats>& b) {
    return a.second.totalArithInstructions > b.second.totalArithInstructions;
}

// Generar reporte
VOID GenerateReport() {
    outFile << "========================================" << std::endl;
    outFile << "  REPORTE DE INSTRUCCIONES ARITMÉTICAS  " << std::endl;
    outFile << "========================================" << std::endl;
    outFile << std::endl;

    // Convertir a vector para ordenar
    vector<pair<ADDRINT, FunctionStats>> sortedStats(
        functionStatsMap.begin(), functionStatsMap.end());

    std::sort(sortedStats.begin(), sortedStats.end(), CompareFunctionStats);

    UINT64 grandTotal = 0;

    for (const auto& entry : sortedStats) {
        const FunctionStats& stats = entry.second;

        if (stats.totalArithInstructions == 0) {
            continue;
        }

        grandTotal += stats.totalArithInstructions;

        outFile << "----------------------------------------" << std::endl;
        outFile << "Función: " << stats.name << std::endl;
        outFile << "Dirección: 0x" << std::hex << stats.address << std::dec << std::endl;
        outFile << "Total instrucciones aritméticas: " << stats.totalArithInstructions << std::endl;

        if (stats.isInlined) {
            outFile << "NOTA: Esta función puede estar inline" << std::endl;
        }

        outFile << std::endl;
        outFile << "Desglose por tipo:" << std::endl;
        outFile << std::setw(20) << "Tipo" << std::setw(15) << "Conteo"
                << std::setw(15) << "Porcentaje" << std::endl;
        outFile << string(50, '-') << std::endl;

        for (const auto& countEntry : stats.arithCounts) {
            if (countEntry.second > 0) {
                double percentage = (100.0 * countEntry.second) / stats.totalArithInstructions;
                outFile << std::setw(20) << ArithTypeNames[countEntry.first]
                        << std::setw(15) << countEntry.second
                        << std::setw(14) << std::fixed << std::setprecision(2)
                        << percentage << "%" << std::endl;
            }
        }

        outFile << std::endl;
    }

    outFile << "========================================" << std::endl;
    outFile << "RESUMEN GLOBAL" << std::endl;
    outFile << "========================================" << std::endl;
    outFile << "Total funciones instrumentadas: " << sortedStats.size() << std::endl;
    outFile << "Total instrucciones aritméticas: " << grandTotal << std::endl;
    outFile << std::endl;

    // Resumen por categoría
    map<ArithType, UINT64> globalCounts;
    for (const auto& entry : functionStatsMap) {
        for (const auto& countEntry : entry.second.arithCounts) {
            globalCounts[countEntry.first] += countEntry.second;
        }
    }

    outFile << "Distribución global por tipo:" << std::endl;
    outFile << std::setw(20) << "Tipo" << std::setw(15) << "Conteo"
            << std::setw(15) << "Porcentaje" << std::endl;
    outFile << string(50, '-') << std::endl;

    for (const auto& countEntry : globalCounts) {
        if (countEntry.second > 0) {
            double percentage = (100.0 * countEntry.second) / grandTotal;
            outFile << std::setw(20) << ArithTypeNames[countEntry.first]
                    << std::setw(15) << countEntry.second
                    << std::setw(14) << std::fixed << std::setprecision(2)
                    << percentage << "%" << std::endl;
        }
    }
}

// Callback al finalizar
VOID Fini(INT32 code, VOID *v) {
    GenerateReport();
    outFile.close();

    std::cerr << "Análisis completado. Resultados en: "
              << KnobOutputFile.Value() << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================

INT32 Usage() {
    std::cerr << "Esta pintool analiza y clasifica instrucciones aritméticas" << std::endl;
    std::cerr << "excluyendo aritmética de infraestructura (punteros, contadores, etc.)" << std::endl;
    std::cerr << std::endl;
    std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;
    std::cerr << std::endl;
    std::cerr << "Opciones de filtrado:" << std::endl;
    std::cerr << "  -s 1     Modo estricto (default): solo aritmética del dominio" << std::endl;
    std::cerr << "  -s 0     Modo permisivo: incluye toda aritmética" << std::endl;
    std::cerr << "  -p 1     Incluir aritmética de punteros (ej: add rax, 8)" << std::endl;
    std::cerr << "  -c 1     Incluir contadores de loop (ej: inc rcx)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Otras opciones:" << std::endl;
    std::cerr << "  -track 0/1  Rastrear jerarquía de llamadas (default: 1)" << std::endl;
    std::cerr << "  -l 0/1      Incluir bibliotecas dinámicas (default: 0)" << std::endl;
    std::cerr << "  -v 0/1      Modo verbose (default: 0)" << std::endl;
    std::cerr << "  -f <func>   Filtrar función específica (repetible)" << std::endl;
    std::cerr << "  -o <file>   Archivo de salida (default: arithmetic_profile.txt)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Ejemplos de uso:" << std::endl;
    std::cerr << "  # Modo estricto (solo aritmética real):" << std::endl;
    std::cerr << "  pin -t inst_counter.so -s 1 -- ./programa" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Incluir todo (modo legacy):" << std::endl;
    std::cerr << "  pin -t inst_counter.so -s 0 -- ./programa" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Modo estricto con contadores (medir overhead):" << std::endl;
    std::cerr << "  pin -t inst_counter.so -s 1 -c 1 -- ./programa" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Análisis de funciones específicas:" << std::endl;
    std::cerr << "  pin -t inst_counter.so -f main -f calculate -- ./programa" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  # Sin rastreo de jerarquía (más rápido):" << std::endl;
    std::cerr << "  pin -t inst_counter.so -track 0 -- ./programa" << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {
    // Inicializar Pin
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    // Abrir archivo de salida
    outFile.open(KnobOutputFile.Value().c_str());
    if (!outFile.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de salida: "
                  << KnobOutputFile.Value() << std::endl;
        return -1;
    }

    // Procesar funciones de interés
    for (UINT32 i = 0; i < KnobFunctionFilter.NumberOfValues(); i++) {
        functionsOfInterest.insert(KnobFunctionFilter.Value(i));
        std::cerr << "Filtrando función: " << KnobFunctionFilter.Value(i) << std::endl;
    }

    if (functionsOfInterest.empty()) {
        std::cerr << "Sin filtro de funciones. Instrumentando todas las funciones." << std::endl;
    }

    // Registrar callbacks
    IMG_AddInstrumentFunction(ImageLoad, 0);
    TRACE_AddInstrumentFunction(InstrumentTrace, 0);
    PIN_AddFiniFunction(Fini, 0);

    std::cerr << "Iniciando instrumentación..." << std::endl;

    // Iniciar el programa
    PIN_StartProgram();

    return 0;
}

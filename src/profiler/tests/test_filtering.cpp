#include <iostream>
#include <vector>
#include <cmath>
#include <immintrin.h>

// Función con operaciones aritméticas básicas
int basicArithmetic(int a, int b) {
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = (b != 0) ? a / b : 0;

    sum++;
    diff--;

    return sum + diff + prod + quot;
}

// Función con operaciones SIMD (SSE)
float sseOperations(float* data, int size) {
    float result = 0.0f;

    #ifdef __SSE__
    for (int i = 0; i < size - 3; i += 4) {
        __m128 vec1 = _mm_loadu_ps(&data[i]);
        __m128 vec2 = _mm_set1_ps(2.0f);

        // Operaciones SSE
        __m128 add_result = _mm_add_ps(vec1, vec2);
        __m128 mul_result = _mm_mul_ps(add_result, vec2);
        __m128 sub_result = _mm_sub_ps(mul_result, vec1);
        __m128 div_result = _mm_div_ps(sub_result, vec2);

        // Acumular resultado
        float temp[4];
        _mm_storeu_ps(temp, div_result);
        result += temp[0] + temp[1] + temp[2] + temp[3];
    }
    #endif

    return result;
}

// Función con operaciones AVX
double avxOperations(double* data, int size) {
    double result = 0.0;

    #ifdef __AVX__
    for (int i = 0; i < size - 3; i += 4) {
        __m256d vec1 = _mm256_loadu_pd(&data[i]);
        __m256d vec2 = _mm256_set1_pd(3.0);

        // Operaciones AVX
        __m256d add_result = _mm256_add_pd(vec1, vec2);
        __m256d mul_result = _mm256_mul_pd(add_result, vec2);
        __m256d sub_result = _mm256_sub_pd(mul_result, vec1);
        __m256d div_result = _mm256_div_pd(sub_result, vec2);

        // Acumular resultado
        double temp[4];
        _mm256_storeu_pd(temp, div_result);
        result += temp[0] + temp[1] + temp[2] + temp[3];
    }
    #endif

    return result;
}

// Función inline (el compilador puede expandirla)
inline int inlineMultiply(int x, int y) {
    return x * y + x - y;
}

// Función con llamadas inline
int useInlineFunction(int a, int b, int c) {
    int result = inlineMultiply(a, b);
    result += inlineMultiply(b, c);
    result += inlineMultiply(a, c);
    return result;
}

// Clase con función virtual (para indirección)
class Calculator {
public:
    virtual int calculate(int a, int b) = 0;
    virtual ~Calculator() {}
};

class Adder : public Calculator {
public:
    int calculate(int a, int b) override {
        return a + b + (a * b) - (a / (b + 1));
    }
};

class Multiplier : public Calculator {
public:
    int calculate(int a, int b) override {
        return (a * b) + (a - b) + (b / (a + 1));
    }
};

// Función que usa punteros a función (indirección)
typedef int (*MathOperation)(int, int);

int applyOperation(int a, int b, MathOperation op) {
    return op(a, b);
}

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// Función recursiva con aritmética
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Función con operaciones FPU
double fpuOperations(double x, double y) {
    double result = 0.0;

    result += x + y;
    result *= x * y;
    result -= x - y;
    result /= (y != 0) ? x / y : 1.0;

    result += std::sin(x) * std::cos(y);

    return result;
}

// Función principal
int main() {
    std::cout << "=== Programa de prueba para Arithmetic Profiler ===" << std::endl;

    // Test 1: Operaciones básicas
    std::cout << "\n1. Operaciones aritméticas básicas:" << std::endl;
    int result1 = basicArithmetic(42, 7);
    std::cout << "   Resultado: " << result1 << std::endl;

    // Test 2: Operaciones SSE
    std::cout << "\n2. Operaciones SIMD (SSE):" << std::endl;
    std::vector<float> floatData(100, 1.5f);
    float result2 = sseOperations(floatData.data(), floatData.size());
    std::cout << "   Resultado: " << result2 << std::endl;

    // Test 3: Operaciones AVX
    std::cout << "\n3. Operaciones SIMD (AVX):" << std::endl;
    std::vector<double> doubleData(100, 2.5);
    double result3 = avxOperations(doubleData.data(), doubleData.size());
    std::cout << "   Resultado: " << result3 << std::endl;

    // Test 4: Funciones inline
    std::cout << "\n4. Funciones inline:" << std::endl;
    int result4 = useInlineFunction(10, 20, 30);
    std::cout << "   Resultado: " << result4 << std::endl;

    // Test 5: Indirección virtual
    std::cout << "\n5. Llamadas virtuales (indirección):" << std::endl;
    Calculator* calc1 = new Adder();
    Calculator* calc2 = new Multiplier();
    int result5a = calc1->calculate(15, 3);
    int result5b = calc2->calculate(15, 3);
    std::cout << "   Adder: " << result5a << std::endl;
    std::cout << "   Multiplier: " << result5b << std::endl;
    delete calc1;
    delete calc2;

    // Test 6: Punteros a función
    std::cout << "\n6. Punteros a función:" << std::endl;
    int result6a = applyOperation(8, 4, add);
    int result6b = applyOperation(8, 4, multiply);
    std::cout << "   Add: " << result6a << std::endl;
    std::cout << "   Multiply: " << result6b << std::endl;

    // Test 7: Recursión
    std::cout << "\n7. Función recursiva (Fibonacci):" << std::endl;
    int result7 = fibonacci(10);
    std::cout << "   Fibonacci(10): " << result7 << std::endl;

    // Test 8: Operaciones FPU
    std::cout << "\n8. Operaciones FPU:" << std::endl;
    double result8 = fpuOperations(3.14, 2.71);
    std::cout << "   Resultado: " << result8 << std::endl;

    std::cout << "\n=== Fin del programa ===" << std::endl;

    return 0;
}

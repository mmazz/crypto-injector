#include "openfhe.h"
#include <random>
#include <iostream>

int random_int(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

using namespace lbcrypto;

bool compareCoefficients(const Ciphertext<DCRTPoly>& c1,
                         const Ciphertext<DCRTPoly>& c2,
                         int numSamples = 10) {
    const auto& elem1 = c1->GetElements()[0].GetAllElements();
    const auto& elem2 = c2->GetElements()[0].GetAllElements();

    int numLimbs = elem1.size();
    if (numLimbs == 0) return false;

    int ringDimension = elem1[0].GetLength();

    // Verify multiple random coefficients
    for (int i = 0; i < numSamples; i++) {
        int limb = random_int(0, numLimbs - 1);
        int coeff = random_int(0, ringDimension - 1);

        if (elem1[limb][coeff] != elem2[limb][coeff]) {
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    uint64_t seed = 1;
    uint32_t multDepth = 3;
    uint32_t ringDim = 1 << 4;
    uint32_t firstMod = 60;
    uint32_t scaleMod = 59;
    uint32_t batchSize = 4;
    ScalingTechnique rescaleTech = FIXEDMANUAL;

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleMod);
    parameters.SetFirstModSize(firstMod);
    parameters.SetBatchSize(batchSize);
    parameters.SetRingDim(ringDim);
    parameters.SetScalingTechnique(rescaleTech);
    parameters.SetSecurityLevel(HEStd_NotSet);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
    cc->Enable(PKE);
    cc->Enable(LEVELEDSHE);

    auto keys = cc->KeyGen();
    PRNG& prng = PseudoRandomNumberGenerator::GetPRNG();

    std::vector<double> input = {0, 0.25, 0.75, 1};
    Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(input);

    // Test 1: SetSeed and ResetToSeed should produce identical ciphertexts
    prng.SetSeed(seed);
    auto c_1 = cc->Encrypt(keys.publicKey, ptxt1);

    prng.ResetToSeed();
    auto c_2 = cc->Encrypt(keys.publicKey, ptxt1);

    bool test1 = compareCoefficients(c_1, c_2, 20);
    std::cout << "Test 1 (SetSeed == ResetToSeed): "
              << (test1 ? "PASS" : "FAIL") << std::endl;

    // Test 2: Without reset, next ciphertext should be different
    auto c_3 = cc->Encrypt(keys.publicKey, ptxt1);
    bool test2 = !compareCoefficients(c_1, c_3, 20);
    std::cout << "Test 2 (next ciphertext different): "
              << (test2 ? "PASS" : "FAIL") << std::endl;

    // Test 3: Reset again should return to same sequence
    prng.ResetToSeed();
    auto c_4 = cc->Encrypt(keys.publicKey, ptxt1);
    bool test3 = compareCoefficients(c_1, c_4, 20);
    std::cout << "Test 3 (reset returns to start): "
              << (test3 ? "PASS" : "FAIL") << std::endl;

    // Test 4: Changing seed produces different ciphertexts
    prng.SetSeed(seed + 42);
    auto c_5 = cc->Encrypt(keys.publicKey, ptxt1);
    bool test4 = !compareCoefficients(c_1, c_5, 20);
    std::cout << "Test 4 (different seed): "
              << (test4 ? "PASS" : "FAIL") << std::endl;

    // Test 5: Reset to new seed works
    prng.ResetToSeed();
    auto c_6 = cc->Encrypt(keys.publicKey, ptxt1);
    bool test5 = compareCoefficients(c_5, c_6, 20);
    std::cout << "Test 5 (reset to new seed): "
              << (test5 ? "PASS" : "FAIL") << std::endl;

    // Test 6: Return to original seed
    prng.SetSeed(seed);
    auto c_7 = cc->Encrypt(keys.publicKey, ptxt1);
    bool test6 = compareCoefficients(c_1, c_7, 20);
    std::cout << "Test 6 (return to original seed): "
              << (test6 ? "PASS" : "FAIL") << std::endl;

    bool allPassed = test1 && test2 && test3 && test4 && test5 && test6;

    std::cout << "\n=================================\n";
    if (allPassed) {
        std::cout << "✓ ALL TESTS PASSED" << std::endl;
    } else {
        std::cout << "✗ SOME TESTS FAILED" << std::endl;
    }
    std::cout << "=================================\n";

    return allPassed ? 0 : 1;
}

#include "openfhe.h"

using namespace lbcrypto;
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
    prng.SetSeed(seed);
    std::vector<double> input = {0, 0.25, 0.75, 1};
    Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(input);
    auto c_1 = cc->Encrypt(keys.publicKey, ptxt1);
    prng.ResetToSeed();
    auto c_2 = cc->Encrypt(keys.publicKey, ptxt1);
    auto c_3 = cc->Encrypt(keys.publicKey, ptxt1);
    prng.ResetToSeed();
    auto c_4 = cc->Encrypt(keys.publicKey, ptxt1);
    prng.SetSeed(seed+3);
    auto c_5 = cc->Encrypt(keys.publicKey, ptxt1);
    prng.ResetToSeed();
    auto c_6 = cc->Encrypt(keys.publicKey, ptxt1);


    std::cout << "c_1 with seed, c_2 with seed and reset it, c_3 with seed without rest, c_4 with seed and resetit"<<
                " c_5 with new seed, c_6 with reset neww seed" << std::endl;
    std::cout << "Are all the same?" << std::endl;
    std::cout << c_1->GetElements()[0].GetAllElements()[0][0] << ", " <<
                 c_2->GetElements()[0].GetAllElements()[0][0] << ", " <<
                 c_3->GetElements()[0].GetAllElements()[0][0] << ", " <<
                 c_4->GetElements()[0].GetAllElements()[0][0] << ", " <<
                 c_5->GetElements()[0].GetAllElements()[0][0] << ", " <<
                 c_6->GetElements()[0].GetAllElements()[0][0] << std::endl;

bool a = c_1->GetElements()[0].GetAllElements()[0][0] == c_2->GetElements()[0].GetAllElements()[0][0];
bool b =c_1->GetElements()[0].GetAllElements()[0][0] == c_4->GetElements()[0].GetAllElements()[0][0];
bool c =c_2->GetElements()[0].GetAllElements()[0][0] != c_3->GetElements()[0].GetAllElements()[0][0];
bool d =c_5->GetElements()[0].GetAllElements()[0][0] == c_6->GetElements()[0].GetAllElements()[0][0];

std::cout << a << " " << b << " " << c << " " << d << std::endl;
    return 0;
}

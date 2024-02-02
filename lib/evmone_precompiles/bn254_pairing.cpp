// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "bn254.hpp"
#include <iostream>

namespace evmmax::bn254
{
namespace
{

const ModArith<uint256> Fp{FieldPrime};

struct Fq2Config
{
    using ValueT = uint256;
    static constexpr auto DEGREE = 2;
    static constexpr std::array<std::pair<uint8_t, uint256>, 2> COEFFS = {{{1, 1_u256}}};
};

template <typename FieldConfigT>
struct FieldElem
{
    using CoeffArrT = std::array<Fq2Config::ValueT, Fq2Config::DEGREE>;
    CoeffArrT coeffs;

    explicit constexpr FieldElem(CoeffArrT cs) noexcept : coeffs(cs) {}

    static inline constexpr FieldElem mul(const FieldElem& e, const Fq2Config::ValueT& s) noexcept
    {
        CoeffArrT res_arr = e.coeffs;
        for (auto& c : res_arr)
            c *= s;
        return FieldElem(std::move(res_arr));
    }

    static inline constexpr FieldElem mul(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        std::array<Fq2Config::ValueT, Fq2Config::DEGREE * 2 - 1> res = {};

        // Multiply
        for (size_t j = 0; j < Fq2Config::DEGREE; ++j)
        {
            for (size_t i = 0; i < Fq2Config::DEGREE; ++i)
                res[i + j] = Fp.add(res[i + j], Fp.mul(e1.coeffs[i], e2.coeffs[j]));
        }

        // Reduce
        for (size_t i = 0; i < res.size() - Fq2Config::DEGREE; ++i)
        {
            for (const auto& mc : Fq2Config::COEFFS)
                res[i + 1 + mc.first] = Fp.sub(res[i + 1 + mc.first], Fp.mul(res[i], mc.second));
        }

        CoeffArrT ret;
        for (size_t i = res.size() - Fq2Config::DEGREE; i < res.size(); ++i)
            ret[i - res.size() + Fq2Config::DEGREE] = res[i];

        return FieldElem(ret);
    }
};

[[maybe_unused]]static auto m =
    FieldElem<Fq2Config>::mul(FieldElem<Fq2Config>({3, 4}), FieldElem<Fq2Config>({2, 5}));

}  // namespace

bool pairing([[maybe_unused]] const std::vector<Point>& pG1, [[maybe_unused]] const std::vector<Point>& pG2) noexcept
{
    return false;
}


}  // namespace evmmax::bn254

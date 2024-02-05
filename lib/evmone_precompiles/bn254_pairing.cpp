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
    typedef uint256 ValueT;
    [[maybe_unused]] static constexpr auto DEGREE = 2;
    [[maybe_unused]] static constexpr std::array<std::pair<uint8_t, uint256>, 1> COEFFS = {
        {{0, 0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256}}};
};

struct Fq6Config
{
    typedef uint256 ValueT;
    [[maybe_unused]] static constexpr uint8_t DEGREE = 6;
    // Polynomial modulus FQ2 (x^6 -18x^3 + 82). Coefficients in Montgomery form.
    [[maybe_unused]] static constexpr std::pair<uint8_t, uint256> COEFFS[2] = {
        /* 82 in mont */ {
            0, 0x26574fb11b10196f403a164ef43989b2be1ac00e5788671d4cf30d5bd4979ae9_u256},
        /* (-18 == mod - 18) in mont */
        {3, 0x259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa0_u256}};
    // Implied + [1 in mont form]
};

struct Fq12Config
{
    typedef uint256 ValueT;
    [[maybe_unused]] static constexpr uint8_t DEGREE = 12;
    // Polynomial modulus FQ2 (x^12 -18x^6 + 82). Coefficients in Montgomery form.
    [[maybe_unused]] static constexpr std::pair<uint8_t, uint256> COEFFS[2] = {
        /* 82 in mont */ {
            0, 0x26574fb11b10196f403a164ef43989b2be1ac00e5788671d4cf30d5bd4979ae9_u256},
        /* (-18 == mod - 18) in mont */
        {6, 0x259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa0_u256}};
    // Implied + [1 in mont form]
};

template <typename FieldConfigT>
struct FieldElem
{
    using CoeffArrT = std::array<typename FieldConfigT::ValueT, FieldConfigT::DEGREE>;
    CoeffArrT coeffs;

    explicit constexpr FieldElem() noexcept {}

    explicit constexpr FieldElem(CoeffArrT cs) noexcept : coeffs(cs)
    {
        (void)cs;
        static_assert(cs.size() == FieldConfigT::DEGREE);
    }

    static inline constexpr FieldElem add(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        CoeffArrT res = e1.coeffs;
        for (size_t i = 0; i < FieldConfigT::DEGREE; ++i)
            res[i] = Fp.add(res[i], e2.coeffs[i]);

        return FieldElem(std::move(res));
    }

    static inline constexpr FieldElem sub(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        CoeffArrT res = e1.coeffs;
        for (size_t i = 0; i < FieldConfigT::DEGREE; ++i)
            res[i] = Fp.sub(res[i], e2.coeffs[i]);

        return FieldElem(std::move(res));
    }

    static inline constexpr FieldElem mul(
        const FieldElem& e, const FieldConfigT::ValueT& s) noexcept
    {
        CoeffArrT res_arr = e.coeffs;
        for (auto& c : res_arr)
            c *= s;
        return FieldElem(std::move(res_arr));
    }

    static inline constexpr FieldElem mul(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        std::array<typename FieldConfigT::ValueT, FieldConfigT::DEGREE * 2 - 1> res = {};

        // Multiply
        for (size_t j = 0; j < FieldConfigT::DEGREE; ++j)
        {
            for (size_t i = 0; i < FieldConfigT::DEGREE; ++i)
                res[i + j] = Fp.add(res[i + j], Fp.mul(e1.coeffs[i], e2.coeffs[j]));
        }

        // Reduce
        for (size_t i = res.size() - 1; i > FieldConfigT::DEGREE - 1; --i)
        {
            for (const auto& mc : FieldConfigT::COEFFS)
                res[i - mc.first - 1] = Fp.sub(res[i - mc.first - 1], Fp.mul(res[i], mc.second));
        }

        CoeffArrT ret;
        for (size_t i = 0; i < FieldConfigT::DEGREE; ++i)
            ret[i] = res[i];

        return FieldElem(std::move(ret));
    }

    static inline constexpr FieldElem neg(const FieldElem& e) noexcept
    {
        CoeffArrT ret;
        for (size_t i = 0; i < FieldConfigT::DEGREE; ++i)
            ret[i] = Fp.sub(0, e.coeffs[i]);

        return FieldElem(std::move(ret));
    }
};

template <typename FieldConfigT>
std::ostream& operator<<(std::ostream& out, const FieldElem<FieldConfigT>& a)
{
    for (const auto& c : a.coeffs)
        out << hex(c) << ", ";

    out << std::endl;

    return out;
}

template <typename FieldElem>
inline constexpr FieldElem operator+(const FieldElem& e1, const FieldElem& e2) noexcept
{
    return FieldElem::add(e1, e2);
}

template <typename FieldElem>
inline constexpr FieldElem operator-(const FieldElem& e1, const FieldElem& e2) noexcept
{
    return FieldElem::sub(e1, e2);
}

template <typename FieldElem>
inline constexpr FieldElem operator*(const FieldElem& e1, const FieldElem& e2) noexcept
{
    return FieldElem::mul(e1, e2);
}

template <typename FieldElem>
inline constexpr FieldElem operator-(const FieldElem& e) noexcept
{
    return FieldElem::neg(e);
}

using Fq2Field = FieldElem<Fq2Config>;
using Fq6FieldNaive = FieldElem<Fq2Config>;
[[maybe_unused]] static constexpr auto t = Fq2Field({9, 1});

struct Fq6Field
{
    static constexpr std::array<std::pair<uint8_t, Fq2Field>, 1> m_coeffs = {
        {{0, Fq2Field({9_u256, 1_u256})}}};
    std::array<Fq2Field, 3> coeffs;

    static inline constexpr Fq6Field mul(const Fq6Field& e1, const Fq6Field& e2) noexcept
    {
        std::array<Fq2Field, 3 * 2 - 1> res;

        // Multiply
        for (size_t j = 0; j < 3; ++j)
        {
            for (size_t i = 0; i < 3; ++i)
                res[i + j] = res[i + j] + e1.coeffs[i] * e2.coeffs[j];
        }

        // Reduce
        for (size_t i = res.size() - 1; i > 3 - 1; --i)
        {
            for (const auto& mc : m_coeffs)
                res[i - mc.first - 1] = res[i - mc.first - 1] - res[i] * mc.second;
        }

        std::array<Fq2Field, 3> ret;
        for (size_t i = 0; i < 3; ++i)
            ret[i] = res[i];

        return Fq6Field(std::move(ret));
    }
};

std::ostream& operator<<(std::ostream& out, const Fq6Field& a)
{
    for (const auto c : a.coeffs)
        out << c << ", ";

    out << std::endl;

    return out;
}

[[maybe_unused]] static auto m = FieldElem<Fq2Config>({3, 4}) + FieldElem<Fq2Config>({2, 5});
[[maybe_unused]] static auto mm = FieldElem<Fq2Config>({3, 4}) * FieldElem<Fq2Config>({2, 5});
[[maybe_unused]] static auto m12 =
    FieldElem<Fq12Config>::mul(FieldElem<Fq12Config>({3, 4}), FieldElem<Fq12Config>({2, 5}));

[[maybe_unused]] static auto m6n =
    FieldElem<Fq6Config>({4, 5, 6, 3, 4, 5}) * FieldElem<Fq6Config>({4, 5, 6, 1, 2, 7});

[[maybe_unused]] static auto m6 =
    Fq6Field::mul(Fq6Field{Fq2Field({4, 5}), Fq2Field({6, 3}), Fq2Field({4, 5})},
        Fq6Field{Fq2Field({4, 5}), Fq2Field({6, 1}), Fq2Field({2, 7})});

}  // namespace

bool pairing([[maybe_unused]] const std::vector<Point>& pG1,
    [[maybe_unused]] const std::vector<Point>& pG2) noexcept
{
    std::cout << m6n;
    std::cout << m6;


    std::cout << hex(Fp.sub(0, Fp.to_mont(9)));

    return false;
}


}  // namespace evmmax::bn254

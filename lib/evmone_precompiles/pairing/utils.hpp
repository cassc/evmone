// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>

namespace evmmax::ecc
{

template <int DEGREE>
struct karatsuba
{};

template <>
struct karatsuba<2>
{
    template <typename FieldElem>
    static inline constexpr std::array<typename FieldElem::ValueT, 3> multiply(
        const FieldElem& a, const FieldElem& b)
    {
        std::array<typename FieldElem::ValueT, 3> r;

        const auto t = a.coeffs[0] * b.coeffs[0];
        const auto u = a.coeffs[1] * b.coeffs[1];
        r[0] = t;
        r[1] = (a.coeffs[0] + a.coeffs[1]) * (b.coeffs[0] + b.coeffs[1]) - u - t;
        r[2] = u;

        return r;
    }
};

template <>
struct karatsuba<3>
{
    template <typename FieldElem>
    static inline constexpr std::array<typename FieldElem::ValueT, 5> multiply(
        const FieldElem& a, const FieldElem& b)
    {
        std::array<typename FieldElem::ValueT, 5> r;

        const auto s = a.coeffs[0] * b.coeffs[0];
        const auto t = a.coeffs[1] * b.coeffs[1];
        const auto v = a.coeffs[2] * b.coeffs[2];

        r[0] = s;
        r[1] = (a.coeffs[0] + a.coeffs[1]) * (b.coeffs[0] + b.coeffs[1]) - s - t;
        r[2] = t + (a.coeffs[0] + a.coeffs[2]) * (b.coeffs[0] + b.coeffs[2]) - s - v;
        r[3] = (a.coeffs[1] + a.coeffs[2]) * (b.coeffs[1] + b.coeffs[2]) - t - v;
        r[4] = v;

        return r;
    }
};

} // namespace  evmmax::ecc

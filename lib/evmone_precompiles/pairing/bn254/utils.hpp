// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "fields.hpp"

namespace evmmax::bn254
{
template <int P>
inline constexpr Fq12 frobenius_operator(const Fq12& f) noexcept
{
    using namespace constants;
    return Fq12({
        Fq6({
            f.coeffs[0].coeffs[0].conjugate(),
            f.coeffs[0].coeffs[1].conjugate() * FROBENIUS_COEFFS[P - 1][1],
            f.coeffs[0].coeffs[2].conjugate() * FROBENIUS_COEFFS[P - 1][3],
        }),
        Fq6({
            f.coeffs[1].coeffs[0].conjugate() * FROBENIUS_COEFFS[P - 1][0],
            f.coeffs[1].coeffs[1].conjugate() * FROBENIUS_COEFFS[P - 1][2],
            f.coeffs[1].coeffs[2].conjugate() * FROBENIUS_COEFFS[P - 1][4],
        }),
    });
}

inline constexpr ecc::ProjPoint<Fq12> frobenius_endomophism(const ecc::ProjPoint<Fq12>& P) noexcept
{
    return {frobenius_operator<1>(P.x), frobenius_operator<1>(P.y), frobenius_operator<1>(P.z)};
}

inline constexpr ecc::ProjPoint<Fq12> dbl(const ecc::ProjPoint<Fq12>& P) noexcept
{
    const auto W = 3 * P.x * P.x;
    const auto S = P.y * P.z;
    const auto B = P.x * P.y * S;
    const auto H = W * W - 8 * B;
    const auto S_squared = S * S;
    const auto newx = 2 * H * S;
    const auto newy = W * (4 * B - H) - 8 * P.y * P.y * S_squared;
    const auto newz = 8 * S * S_squared;
    return {newx, newy, newz};
}

inline constexpr ecc::ProjPoint<Fq12> add(
    const ecc::ProjPoint<Fq12>& p, const ecc::ProjPoint<Fq12>& q, const Fq12& b3) noexcept
{
    const auto& x1 = p.x;
    const auto& y1 = p.y;
    const auto& z1 = p.z;
    const auto& x2 = q.x;
    const auto& y2 = q.y;
    const auto& z2 = q.z;
    Fq12 x3;
    Fq12 y3;
    Fq12 z3;
    Fq12 t0;
    Fq12 t1;
    Fq12 t2;
    Fq12 t3;
    Fq12 t4;

    t0 = x1 * x2;  // 1
    t1 = y1 * y2;  // 2
    t2 = z1 * z2;  // 3
    t3 = x1 + y1;  // 4
    t4 = x2 + y2;  // 5
    t3 = t3 * t4;  // 6
    t4 = t0 + t1;  // 7
    t3 = t3 - t4;  // 8
    t4 = y1 + z1;  // 9
    x3 = y2 + z2;  // 10
    t4 = t4 * x3;  // 11
    x3 = t1 + t2;  // 12
    t4 = t4 - x3;  // 13
    x3 = x1 + z1;  // 14
    y3 = x2 + z2;  // 15
    x3 = x3 * y3;  // 16
    y3 = t0 + t2;  // 17
    y3 = x3 - y3;  // 18
    x3 = t0 + t0;  // 19
    t0 = x3 + t0;  // 20
    t2 = b3 * t2;  // 21
    z3 = t1 + t2;  // 22
    t1 = t1 - t2;  // 23
    y3 = b3 * y3;  // 24
    x3 = t4 * y3;  // 25
    t2 = t3 * t1;  // 26
    x3 = t2 - x3;  // 27
    y3 = y3 * t0;  // 28
    t1 = t1 * z3;  // 29
    y3 = t1 + y3;  // 30
    t0 = t0 * t3;  // 31
    z3 = z3 * t4;  // 32
    z3 = z3 + t0;  // 33

    return {x3, y3, z3};
}

inline constexpr std::pair<Fq12, Fq12> linear_func(const ecc::ProjPoint<Fq12>& P1,
    const ecc::ProjPoint<Fq12>& P2, const ecc::ProjPoint<Fq12>& T) noexcept
{
    auto n = P2.y * P1.z - P1.y * P2.z;
    auto d = P2.x * P1.z - P1.x * P2.z;

    if (d != Fq12::zero())
        return {n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z};
    else if (n == Fq12::zero())
    {
        n = 3 * P1.x * P1.x;
        d = 2 * P1.y * P1.z;
        return {n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z};
    }
    else
        return {T.x * P1.z - P1.x * T.z, T.z * P1.z};
}
} // namespace  evmmax::ecc

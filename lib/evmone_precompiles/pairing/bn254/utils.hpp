// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "fields.hpp"
#include <iostream>

namespace evmmax::bn254
{
template <typename ValueT>
std::ostream& operator<<(std::ostream& out, const ecc::ProjPoint<ValueT>& p)
{
    out << "x: " + p.x.to_string() << std::endl;
    out << "y: " + p.y.to_string() << std::endl;
    out << "z: " + p.z.to_string() << std::endl;

    return out;
}

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
    using namespace constants;
    auto W = P.x * P.x;
    W = _3 * W;
    const auto S = P.y * P.z;
    const auto B = P.x * P.y * S;
    const auto H = W * W - _8 * B;
    const auto S_squared = S * S;
    const auto newx = _2 * H * S;
    const auto newy = W * (_4 * B - H) - _8 * P.y * P.y * S_squared;
    const auto newz = _8 * S * S_squared;
    return {newx, newy, newz};
}

inline constexpr ecc::ProjPoint<Fq12> add(
    const ecc::ProjPoint<Fq12>& p1, const ecc::ProjPoint<Fq12>& p2) noexcept
{
    using namespace constants;

    if (p1.z == Fq12::zero() || p2.z == Fq12::zero())
        return p2.z == Fq12::zero() ? p1 : p2;

    const auto x1 = p1.x;
    const auto y1 = p1.y;
    const auto z1 = p1.z;
    const auto x2 = p2.x;
    const auto y2 = p2.y;
    const auto z2 = p2.z;
    const auto U1 = y2 * z1;
    const auto U2 = y1 * z2;
    const auto V1 = x2 * z1;
    const auto V2 = x1 * z2;
    if (V1 == V2 && U1 == U2)
        return dbl(p1);
    else if (V1 == V2)
        return {Fq12::one(), Fq12::one(), Fq12::zero()};

    const auto U = U1 - U2;
    const auto V = V1 - V2;
    const auto V_squared = V * V;
    const auto V_squared_times_V2 = V_squared * V2;
    const auto V_cubed = V * V_squared;
    const auto W = z1 * z2;
    const auto A = U * U * W - V_cubed - _2 * V_squared_times_V2;
    const auto newx = V * A;
    const auto newy = U * (V_squared_times_V2 - A) - V_cubed * U2;
    const auto newz = V_cubed * W;
    return {newx, newy, newz};
}

inline constexpr std::pair<Fq12, Fq12> linear_func(const ecc::ProjPoint<Fq12>& P1,
    const ecc::ProjPoint<Fq12>& P2, const ecc::ProjPoint<Fq12>& T) noexcept
{
    using namespace constants;

    auto n = P2.y * P1.z - P1.y * P2.z;
    auto d = P2.x * P1.z - P1.x * P2.z;

    if (d != Fq12::zero())
        return {n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z};
    else if (n == Fq12::zero())
    {
        n = _3 * P1.x * P1.x;
        d = _2 * P1.y * P1.z;
        return {n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z};
    }
    else
        return {T.x * P1.z - P1.x * T.z, T.z * P1.z};
}
}  // namespace evmmax::bn254

// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../../bn254.hpp"
#include "constants.hpp"
#include "evmmax/evmmax.hpp"
#include "fields.hpp"
#include <iostream>

namespace evmmax::bn254
{
namespace
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

inline constexpr ecc::ProjPoint<Fq12> untwist(const ecc::ProjPoint<Fq2>& p) noexcept
{
    const ecc::ProjPoint<Fq12> ret = {
        .x = Fq12({Fq6({{p.x, Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .y = Fq12({Fq6({{p.y, Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .z = Fq12({Fq6({{p.z, Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
    };

    return {ret.x * omega2, ret.y * omega3, ret.z};
}

inline constexpr ecc::ProjPoint<Fq12> cast_to_fq12(const ecc::ProjPoint<Fq>& p) noexcept
{
    return ecc::ProjPoint<Fq12>{
        .x = Fq12({Fq6({{Fq2({p.x, Fq::zero()}), Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .y = Fq12({Fq6({{Fq2({p.y, Fq::zero()}), Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .z = Fq12({Fq6({{Fq2({p.z, Fq::zero()}), Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
    };
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

inline constexpr auto ate_loop_count = 29793968203157093288_u256;
inline constexpr int log_ate_loop_count = 63;
const auto B3 = make_fq12({{{3 * 3, 0}, {0, 0}, {0, 0}}}, {{{0, 0}, {1, 0}, {0, 0}}});

inline constexpr std::pair<Fq12, Fq12> miller_loop(
    const ecc::ProjPoint<Fq12>& Q, const ecc::ProjPoint<Fq12>& P) noexcept
{
    auto R = Q;
    auto f_num = Fq12::one();
    auto f_den = Fq12::one();

    for (int i = log_ate_loop_count; i >= 0; --i)
    {
        auto [_n, _d] = linear_func(R, R, P);
        R = dbl(R);
        f_num = f_num * f_num * _n;
        f_den = f_den * f_den * _d;
        if (ate_loop_count & (1 << i))
        {
            std::tie(_n, _d) = linear_func(R, Q, P);
            R = add(R, Q, B3);
            f_num = f_num * _n;
            f_den = f_den * _d;
        }
    }
    const auto Q1 = frobenius_endomophism(Q);
    const auto nQ2 = -frobenius_endomophism(Q1);

    const auto [_n1, _d1] = linear_func(R, Q1, P);
    R = add(R, Q1, B3);
    const auto [_n2, _d2] = linear_func(R, nQ2, P);

    return {f_num * _n1 * _n2, f_den * _d1 * _d2};
}

inline Fq12 final_exp(const Fq12& v) noexcept
{
    auto f = v;
    auto f1 = f.conjugate();

    f = f1 * f.inv();                   // easy 1
    f = frobenius_operator<2>(f) * f;  // easy 2

    f1 = f.conjugate();

    const auto ft1 = f ^ X;
    const auto ft2 = ft1 ^ X;
    const auto ft3 = ft2 ^ X;
    const auto fp1 = frobenius_operator<1>(f);
    const auto fp2 = frobenius_operator<2>(f);
    const auto fp3 = frobenius_operator<3>(f);
    const auto y0 = fp1 * fp2 * fp3;
    const auto y1 = f1;
    const auto y2 = frobenius_operator<2>(ft2);
    const auto y3 = frobenius_operator<1>(ft1).conjugate();
    const auto y4 = (frobenius_operator<1>(ft2) * ft1).conjugate();
    const auto y5 = ft2.conjugate();
    const auto y6 = (frobenius_operator<1>(ft3) * ft3).conjugate();

    auto t0 = (y6 ^ 2) * y4 * y5;
    auto t1 = y3 * y5 * t0;
    t0 = t0 * y2;
    t1 = ((t1 ^ 2) * t0) ^ 2;
    t0 = t1 * y1;
    t1 = t1 * y0;
    t0 = t0 ^ 2;
    return t1 * t0;
}
}  // namespace

bool pairing(const std::vector<std::array<uint256, 4>>& vG2, const std::vector<Point>& vG1) noexcept
{
    if (vG1.size() != vG2.size())
        return false;

    if (vG1.size() == 0)
        return true;

    auto n = Fq12::one();
    auto d = Fq12::one();

    for (size_t i = 0; i < vG2.size(); ++i)
    {
        const auto Q_proj = ecc::ProjPoint<Fq2>(Fq2({vG2[i][0], vG2[i][1]}).to_mont(),
            Fq2({vG2[i][2], vG2[i][3]}).to_mont(), Fq2::one());
        const auto P_proj =
            ecc::ProjPoint<Fq>(Fq(vG1[i].x).to_mont(), Fq(vG1[i].y).to_mont(), Fq::one());

        const auto [_n, _d] = miller_loop(untwist(Q_proj), cast_to_fq12(P_proj));

        n = n * _n;
        d = d * _d;
    }

    const auto f = final_exp(n * d.inv());

    std::cout << f.to_string() << std::endl;

    return f == Fq12::one();
}
}  // namespace evmmax::bn254

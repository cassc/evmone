// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "../../bn254.hpp"
#include "evmmax/evmmax.hpp"
#include "fields.hpp"
#include "utils.hpp"
#include <iostream>

namespace evmmax::bn254
{
namespace
{
inline constexpr ecc::ProjPoint<Fq12> untwist(const ecc::ProjPoint<Fq2>& p) noexcept
{
    using namespace constants;
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

        if (ate_loop_count & (1_u256 << i))
        {
            std::tie(_n, _d) = linear_func(R, Q, P);
            R = add(R, Q);
            f_num = f_num * _n;
            f_den = f_den * _d;
        }
    }

    const auto Q1 = frobenius_endomophism(Q);
    const auto nQ2 = -frobenius_endomophism(Q1);

    const auto [_n1, _d1] = linear_func(R, Q1, P);
    R = add(R, Q1);
    const auto [_n2, _d2] = linear_func(R, nQ2, P);

    return {f_num * _n1 * _n2, f_den * _d1 * _d2};
}

inline Fq12 final_exp(const Fq12& v) noexcept
{
    auto f = v;
    auto f1 = f.conjugate();

    f = f1 * f.inv();                  // easy 1
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

    return f == Fq12::one();
}
}  // namespace evmmax::bn254

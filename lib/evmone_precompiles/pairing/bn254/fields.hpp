// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../bn254.hpp"
#include "../../ecc.hpp"
#include "../field_template.hpp"
#include "../utils.hpp"

namespace evmmax::bn254
{
using namespace intx;

struct BaseFieldConfig
{
    typedef uint256 ValueT;
    static inline const ModArith<ValueT> MOD_ARITH = ModArith(FieldPrime);
    // 1 in Montgomery form
    static constexpr uint256 ONE =
        0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256;
};
using Fq = ecc::BaseFieldElem<BaseFieldConfig>;

struct Fq2Config
{
    typedef Fq BaseFieldT;
    typedef Fq ValueT;
    [[maybe_unused]] static constexpr auto DEGREE = 2;
    [[maybe_unused]] static constexpr std::array<std::pair<uint8_t, ValueT>, 1> COEFFS = {{{
        0,
        // 1 in mont
        0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256,
    }}};
};
using Fq2 = ecc::FieldElem<Fq2Config>;

struct Fq6Config
{
    typedef Fq BaseFieldT;
    typedef Fq2 ValueT;
    static constexpr uint8_t DEGREE = 3;
    static constexpr std::pair<uint8_t, ValueT> COEFFS[1] = {
        {
            0,
            ValueT({
                // -9 in mont
                0x12ceb58a394e07d28f0d12384840918c6843fb439555fa7b461a4448976f7d50_u256,
                // -1 in mont
                0x2259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa_u256,
            }),
        },
    };
};
using Fq6 = ecc::FieldElem<Fq6Config>;

struct Fq12Config
{
    typedef Fq BaseFieldT;
    typedef Fq6 ValueT;
    [[maybe_unused]] static constexpr uint8_t DEGREE = 2;
    [[maybe_unused]] static constexpr std::pair<uint8_t, ValueT> COEFFS[1] = {
        {
            0,
            Fq6({
                Fq2({
                    0_u256,
                    0_u256,
                }),
                Fq2({
                    // -1 in mont
                    0x2259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa_u256,
                    0_u256,
                }),
                Fq2({
                    0_u256,
                    0_u256,
                }),
            }),
        },
    };
};
using Fq12 = ecc::FieldElem<Fq12Config>;

static inline Fq inverse(const Fq& x)
{
    return field_inv(BaseFieldConfig::MOD_ARITH, x.value);
}

static inline Fq2 inverse(const Fq2& f)
{
    const auto& a0 = f.coeffs[0];
    const auto& a1 = f.coeffs[1];
    auto t0 = a0 * a0;
    auto t1 = a1 * a1;
    const auto& beta = Fq2Config::COEFFS[0].second;

    t0 = t0 + beta * t1;
    t1 = t0.inv();
    const auto c0 = a0 * t1;
    const auto c1 = -(a1 * t1);

    return Fq2({c0, c1});
}

static inline Fq6 inverse(const Fq6& f)
{
    const auto& a0 = f.coeffs[0];
    const auto& a1 = f.coeffs[1];
    const auto& a2 = f.coeffs[2];

    const Fq2& ksi = Fq6Config::COEFFS[0].second;

    const auto t0 = a0 * a0;
    const auto t1 = a1 * a1;
    const auto t2 = a2 * a2;

    const auto t3 = a0 * a1;
    const auto t4 = a0 * a2;
    const auto t5 = a2 * a1;

    const auto v0 = t0 + ksi * t5;
    const auto v1 = -ksi * t2 - t3;
    const auto v2 = t1 - t4;

    const auto c0 = a0 * v0;
    const auto c1 = -(a1 * v2);
    const auto c2 = -(a2 * v1);

    const auto t = (c0 + (c1 + c2) * ksi).inv();

    return Fq6({v0 * t, v1 * t, v2 * t});
}

static inline Fq12 inverse(const Fq12& f)
{
    const auto& a0 = f.coeffs[0];
    const auto& a1 = f.coeffs[1];

    auto t0 = a0 * a0;
    auto t1 = a1 * a1;

    const Fq6& gamma = Fq12Config::COEFFS[0].second;

    t0 = t0 + gamma * t1;
    t1 = t0.inv();
    const auto c0 = a0 * t1;
    const auto c1 = -(a1 * t1);

    return Fq12({c0, c1});
}

inline constexpr Fq2 make_fq2(std::array<uint256, 2> a) noexcept
{
    return Fq2({Fq(a[0]).to_mont(), Fq(a[1]).to_mont()});
}

inline constexpr Fq6 make_fq6(
    std::array<uint256, 2> a, std::array<uint256, 2> b, std::array<uint256, 2> c) noexcept
{
    return Fq6({make_fq2(a), make_fq2(b), make_fq2(c)});
}

inline constexpr Fq6 make_fq6(std::array<std::array<uint256, 2>, 3> a) noexcept
{
    return Fq6({make_fq2(a[0]), make_fq2(a[1]), make_fq2(a[2])});
}

inline constexpr Fq12 make_fq12(
    std::array<std::array<uint256, 2>, 3> a, std::array<std::array<uint256, 2>, 3> b) noexcept
{
    return Fq12({make_fq6(a), make_fq6(b)});
}

namespace constants
{
static inline auto omega2 = make_fq12(
    {
        {
            {0, 0},
            {1, 0},
            {0, 0},
        },
    },
    {
        {
            {0, 0},
            {0, 0},
            {0, 0},
        },
    });

static inline auto omega3 = make_fq12(
    {
        {
            {0, 0},
            {0, 0},
            {0, 0},
        },
    },
    {
        {
            {0, 0},
            {1, 0},
            {0, 0},
        },
    });

static inline std::array<std::array<Fq2, 5>, 3> FROBENIUS_COEFFS = {{
    {
        make_fq2({
            8376118865763821496583973867626364092589906065868298776909617916018768340080_u256,
            16469823323077808223889137241176536799009286646108169935659301613961712198316_u256,
        }),
        make_fq2({
            21575463638280843010398324269430826099269044274347216827212613867836435027261_u256,
            10307601595873709700152284273816112264069230130616436755625194854815875713954_u256,
        }),
        make_fq2({
            2821565182194536844548159561693502659359617185244120367078079554186484126554_u256,
            3505843767911556378687030309984248845540243509899259641013678093033130930403_u256,
        }),
        make_fq2({
            2581911344467009335267311115468803099551665605076196740867805258568234346338_u256,
            19937756971775647987995932169929341994314640652964949448313374472400716661030_u256,
        }),
        make_fq2({
            685108087231508774477564247770172212460312782337200605669322048753928464687_u256,
            8447204650696766136447902020341177575205426561248465145919723016860428151883_u256,
        }),
    },
    {
        make_fq2({
            21888242871839275220042445260109153167277707414472061641714758635765020556617_u256,
            0_u256,
        }),
        make_fq2({
            21888242871839275220042445260109153167277707414472061641714758635765020556616_u256,
            0_u256,
        }),
        make_fq2({
            21888242871839275222246405745257275088696311157297823662689037894645226208582_u256,
            0_u256,
        }),
        make_fq2({
            2203960485148121921418603742825762020974279258880205651966_u256,
            0_u256,
        }),
        make_fq2({
            2203960485148121921418603742825762020974279258880205651967_u256,
            0_u256,
        }),
    },
    {
        make_fq2({
            11697423496358154304825782922584725312912383441159505038794027105778954184319_u256,
            303847389135065887422783454877609941456349188919719272345083954437860409601_u256,
        }),
        make_fq2({
            3772000881919853776433695186713858239009073593817195771773381919316419345261_u256,
            2236595495967245188281701248203181795121068902605861227855261137820944008926_u256,
        }),
        make_fq2({
            19066677689644738377698246183563772429336693972053703295610958340458742082029_u256,
            18382399103927718843559375435273026243156067647398564021675359801612095278180_u256,
        }),
        make_fq2({
            5324479202449903542726783395506214481928257762400643279780343368557297135718_u256,
            16208900380737693084919495127334387981393726419856888799917914180988844123039_u256,
        }),
        make_fq2({
            8941241848238582420466759817324047081148088512956452953208002715982955420483_u256,
            10338197737521362862238855242243140895517409139741313354160881284257516364953_u256,
        }),
    },
}};
}  // namespace constants

}  // namespace evmmax::bn254

// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "bn254.hpp"
#include "evmmax/evmmax.hpp"
#include <iostream>

namespace evmmax::bn254
{
namespace
{
constexpr inline ModArith<uint256> Fp{FieldPrime,
    0x6d89f71cab8351f47ab1eff0a417ff6b5e71911d44501fbf32cfc5b538afa89_u256, 9786893198990664585u};

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

template <typename FieldConfigT>
struct FieldElem;

struct BaseFieldConfig
{
    typedef uint256 ValueT;
};

template <>
struct FieldElem<BaseFieldConfig>
{
    typedef BaseFieldConfig::ValueT ValueT;

    ValueT value;

    constexpr FieldElem() noexcept : value(ValueT{}) {}

    constexpr FieldElem(const ValueT& v) noexcept : value(v) {}

    static inline constexpr FieldElem add(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return FieldElem(Fp.add(e1.value, e2.value));
    }

    static inline constexpr FieldElem sub(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return FieldElem(Fp.sub(e1.value, e2.value));
    }

    static inline constexpr FieldElem mul(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return FieldElem(Fp.mul(e1.value, e2.value));
    }

    static inline constexpr FieldElem mul(const FieldElem& e1, const ValueT& s) noexcept
    {
        return FieldElem(Fp.mul(e1.value, s));
    }

    static inline constexpr bool eq(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return e1.value == e2.value;
    }

    static inline constexpr bool neq(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return e1.value != e2.value;
    }

    static inline constexpr FieldElem neg(const FieldElem& e) noexcept
    {
        return FieldElem(Fp.sub(BaseFieldConfig::ValueT{0}, e.value));
    }

    inline constexpr std::string to_string() const noexcept { return hex(Fp.from_mont(value)); }

    static inline constexpr FieldElem one() noexcept
    {
        // 1 in mont
        return FieldElem(0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256);
    }

    static inline constexpr FieldElem zero() noexcept { return FieldElem(0); }
};

using Base = FieldElem<BaseFieldConfig>;

template <typename FieldConfigT>
struct FieldElem
{
    typedef FieldConfigT::ValueT ValueT;
    static constexpr auto DEGREE = FieldConfigT::DEGREE;
    using CoeffArrT = std::array<ValueT, DEGREE>;
    CoeffArrT coeffs;

    explicit constexpr FieldElem() noexcept
    {
        for (size_t i = 0; i < DEGREE; ++i)
            coeffs[i] = ValueT();
    }

    explicit constexpr FieldElem(CoeffArrT cs) noexcept : coeffs(cs) {}

    static inline constexpr FieldElem conjugate(const FieldElem& e) noexcept
    {
        CoeffArrT res = e.coeffs;

        for (size_t i = 1; i < DEGREE; i += 2)
            res[i] = -res[i];

        return FieldElem(res);
    }

    static inline constexpr FieldElem add(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        CoeffArrT res = e1.coeffs;
        for (size_t i = 0; i < DEGREE; ++i)
            res[i] = ValueT::add(res[i], e2.coeffs[i]);

        return FieldElem(std::move(res));
    }

    static inline constexpr FieldElem sub(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        CoeffArrT res = e1.coeffs;
        for (size_t i = 0; i < DEGREE; ++i)
            res[i] = ValueT::sub(res[i], e2.coeffs[i]);

        return FieldElem(std::move(res));
    }

    static inline constexpr FieldElem mul(const FieldElem& e, const Base::ValueT& s) noexcept
    {
        CoeffArrT res_arr = e.coeffs;
        for (auto& c : res_arr)
            c = ValueT::mul(c, s);
        return FieldElem(std::move(res_arr));
    }

    static inline constexpr FieldElem reduce(std::array<ValueT, DEGREE * 2 - 1> e) noexcept
    {
        for (size_t i = e.size() - 1; i >= DEGREE; --i)
        {
            for (const auto& mc : FieldConfigT::COEFFS)
            {
                const auto d = i - DEGREE;
                e[d + mc.first] = ValueT::sub(e[d + mc.first], ValueT::mul(e[i], mc.second));
            }
        }

        CoeffArrT ret;
        for (size_t i = 0; i < DEGREE; ++i)
            ret[i] = e[i];

        return FieldElem(std::move(ret));
    }

    static inline constexpr FieldElem mul(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        std::array<ValueT, DEGREE * 2 - 1> res = karatsuba<DEGREE>::multiply(e1, e2);

        return reduce(res);
    }

    static inline constexpr FieldElem neg(const FieldElem& e) noexcept
    {
        CoeffArrT ret;
        for (size_t i = 0; i < DEGREE; ++i)
            ret[i] = ValueT::neg(e.coeffs[i]);

        return FieldElem(std::move(ret));
    }

    static inline constexpr bool eq(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        bool res = true;
        for (size_t i = 0; i < DEGREE && res; ++i)
            res = res && ValueT::eq(e1.coeffs[i], e2.coeffs[i]);

        return res;
    }

    static inline constexpr bool neq(const FieldElem& e1, const FieldElem& e2) noexcept
    {
        return !eq(e1, e2);
    }

    inline constexpr std::string to_string() const noexcept
    {
        std::string res;
        for (const auto& c : coeffs)
            res += c.to_string() + ", ";
        return "[" + res.substr(0, res.size() - 2) + "]";
    }

    static inline constexpr FieldElem one() noexcept
    {
        std::array<ValueT, DEGREE> v;
        v[0] = ValueT::one();
        for (size_t i = 1; i < DEGREE; ++i)
            v[i] = ValueT();

        return FieldElem(v);
    }

    static inline constexpr FieldElem zero() noexcept
    {
        std::array<ValueT, DEGREE> v;
        for (size_t i = 0; i < DEGREE; ++i)
            v[i] = ValueT();

        return FieldElem(v);
    }
};

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

template <typename FieldElem>
inline constexpr bool operator==(const FieldElem& e1, const FieldElem& e2) noexcept
{
    return FieldElem::eq(e1, e2);
}

template <typename FieldElem>
inline constexpr bool operator!=(const FieldElem& e1, const FieldElem& e2) noexcept
{
    return !FieldElem::eq(e1, e2);
}

template <typename FieldElem>
inline constexpr FieldElem operator*(const uint256& s, const FieldElem& e) noexcept
{
    return FieldElem::mul(e, s);
}

template <typename ValueT>
inline constexpr ecc::ProjPoint<ValueT> operator-(const ecc::ProjPoint<ValueT>& p) noexcept
{
    return {p.x, -p.y, p.z};
}

struct Fq2Config
{
    typedef Base ValueT;
    [[maybe_unused]] static constexpr auto DEGREE = 2;
    [[maybe_unused]] static constexpr std::array<std::pair<uint8_t, ValueT>, 1> COEFFS = {{{
        0,
        // 1 in mont
        Base(0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256),
    }}};
};
using Fq2 = FieldElem<Fq2Config>;

struct Fq6Config
{
    typedef Fq2 ValueT;
    static constexpr uint8_t DEGREE = 3;
    static constexpr std::pair<uint8_t, ValueT> COEFFS[1] = {
        {
            0,
            ValueT({
                // -9 in mont
                Base(0x12ceb58a394e07d28f0d12384840918c6843fb439555fa7b461a4448976f7d50_u256),
                // -1 in mont
                Base(0x2259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa_u256),
            }),
        },
    };
};
using Fq6 = FieldElem<Fq6Config>;

struct Fq12Config
{
    typedef Fq6 ValueT;
    [[maybe_unused]] static constexpr uint8_t DEGREE = 2;
    [[maybe_unused]] static constexpr std::pair<uint8_t, ValueT> COEFFS[1] = {
        {
            0,
            Fq6({
                Fq2({
                    Base(0),
                    Base(0),
                }),
                Fq2({
                    // -1 in mont
                    Base{0x2259d6b14729c0fa51e1a247090812318d087f6872aabf4f68c3488912edefaa_u256},
                    Base{0},
                }),
                Fq2({
                    Base{0},
                    Base{0},
                }),
            }),
        },
    };
};
using Fq12 = FieldElem<Fq12Config>;

// static inline std::array<Base, 12> get_poly_coeffs(const Fq12& f)
//{
//     std::array<Base, 12> r = {};
//
//     for (size_t i = 0; i < 2; ++i)
//     {
//         for (size_t j = 0; j < 6; ++j)
//         {
//             const auto c2 = f.coeffs[i].coeffs[j];
//             r[i + j * 2] = c2.coeffs[0] - 9 * c2.coeffs[1];
//             r[i + j * 2 + 6] = c2.coeffs[1];
//         }
//     }
//
//     return r;
// }

// inline constexpr size_t deg(const std::array<Base, 13>& p)
//{
//     size_t d = 12;
//     while (p[d].value == 0_u256 && d > 0)
//         d -= 1;
//
//     return d;
// }
//
// inline constexpr  poly_rounded_div(a, b):
//     dega = deg(a)
//     degb = deg(b)
//     temp = [x for x in a]
//     o = [FP(0) for x in a]
//     for i in range(dega - degb, -1, -1):
//         o[i] = (o[i] + temp[degb + i] * FP(prime_field_inv(b[degb].value, N)))
//         for c in range(degb + 1):
//             temp[c + i] = (temp[c + i] - o[c])
//     return [x for x in o[:deg(o) + 1]]
//
// static inline std::array<Base, 12> FQ12_modulus_coeffs = {
//     82_u256,
//     0_u256,
//     0_u256,
//     0_u256,
//     0_u256,
//     0_u256,
//     -18_u256,
//     0_u256,
//     0_u256,
//     0_u256,
//     0_u256,
//     0_u256,
// };

static inline Fq2 inv(const Fq2& f)
{
    const auto& a0 = f.coeffs[0];
    const auto& a1 = f.coeffs[1];
    auto t0 = a0 * a0;
    auto t1 = a1 * a1;
    const Base& beta = Fq2Config::COEFFS[0].second;

    t0 = t0 + beta * t1;
    t1 = field_inv(Fp, t0.value);
    const auto c0 = a0 * t1;
    const auto c1 = -(a1 * t1);

    return Fq2({c0, c1});
}

static inline Fq6 inv(const Fq6& f)
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

    const auto t = inv(c0 + (c1 + c2) * ksi);

    return Fq6({v0 * t, v1 * t, v2 * t});
}

static inline Fq12 inv(const Fq12& f)
{
    const auto& a0 = f.coeffs[0];
    const auto& a1 = f.coeffs[1];

    auto t0 = a0 * a0;
    auto t1 = a1 * a1;

    const Fq6& gamma = Fq12Config::COEFFS[0].second;

    t0 = t0 - gamma * t1;
    t1 = inv(t0);
    const auto c0 = a0 * t1;
    const auto c1 = -a1 * t1;

    return Fq12({c0, c1});
}

static inline Fq2 make_fq2(std::array<Base::ValueT, 2> a) noexcept
{
    return Fq2({Base(Fp.to_mont(a[0])), Base(Fp.to_mont(a[1]))});
}

static inline Fq2 make_fq2(const Base::ValueT& a, const Base::ValueT& b) noexcept
{
    return Fq2({Base(Fp.to_mont(a)), Base(Fp.to_mont(b))});
}

[[maybe_unused]] inline constexpr Fq6 make_fq6(std::array<Base::ValueT, 2> a,
    std::array<Base::ValueT, 2> b, std::array<Base::ValueT, 2> c) noexcept
{
    return Fq6({make_fq2(a), make_fq2(b), make_fq2(c)});
}

inline constexpr Fq6 make_fq6(std::array<std::array<Base::ValueT, 2>, 3> a) noexcept
{
    return Fq6({make_fq2(a[0]), make_fq2(a[1]), make_fq2(a[2])});
}

inline constexpr Fq12 make_fq12(std::array<std::array<Base::ValueT, 2>, 3> a,
    std::array<std::array<Base::ValueT, 2>, 3> b) noexcept
{
    return Fq12({make_fq6(a), make_fq6(b)});
}

template <typename ValueT>
std::ostream& operator<<(std::ostream& out, const ecc::ProjPoint<ValueT>& p)
{
    out << "x: " + p.x.to_string() << std::endl;
    out << "y: " + p.y.to_string() << std::endl;
    out << "z: " + p.z.to_string() << std::endl;

    return out;
}

static inline auto omega2 = make_fq12(
    {
        {
            {0, 0},
            {0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256, 0},
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
            {0xe0a77c19a07df2f666ea36f7879462c0a78eb28f5c70b3dd35d438dc58f0d9d_u256, 0},
            {0, 0},
        },
    });

static inline std::array<Fq2, 5> FROB_L1 = {
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
};

static inline std::array<Fq2, 5> FROB_L2 = {
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
};

static inline std::array<Fq2, 5> FROB_L3 = {
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
};

template <int P>
inline constexpr Fq12 frobenius_operator(const Fq12& f) noexcept
{
    return Fq12({
        Fq6({
            Fq2::conjugate(f.coeffs[0].coeffs[0]),
            Fq2::conjugate(f.coeffs[0].coeffs[1]) * FROB_L1[1],
            Fq2::conjugate(f.coeffs[0].coeffs[2]) * FROB_L1[3],
        }),
        Fq6({
            Fq2::conjugate(f.coeffs[1].coeffs[0]) * FROB_L1[0],
            Fq2::conjugate(f.coeffs[1].coeffs[1]) * FROB_L1[2],
            Fq2::conjugate(f.coeffs[1].coeffs[2]) * FROB_L1[4],
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

inline constexpr ecc::ProjPoint<Fq12> cast_to_fq12(const ecc::ProjPoint<Base>& p) noexcept
{
    return ecc::ProjPoint<Fq12>{
        .x = Fq12({Fq6({{Fq2({p.x, Base(0)}), Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .y = Fq12({Fq6({{Fq2({p.y, Base(0)}), Fq2::zero(), Fq2::zero()}}),
            Fq6({{Fq2::zero(), Fq2::zero(), Fq2::zero()}})}),
        .z = Fq12({Fq6({{Fq2({p.z, Base(0)}), Fq2::zero(), Fq2::zero()}}),
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

// inline constexpr Fq12 pow(const Fq12& v, uint256 p) noexcept
//{
//     auto o = Fq12::one();
//     auto t = v;
//     while (p > 0)
//     {
//         if (p & 1)
//             o = o * t;
//         p >>= 1;
//         t = t * t;
//     }
//     return t;
// }

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

}  // namespace

bool pairing(const std::vector<Point>&, const std::vector<Point>&) noexcept
{
    const auto t = make_fq2(0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678_u256,
        0x209dd15ebff5d46c4bd888e51a93cf99a7329636c63514396b4a452003a35bf7_u256);

    std::cout << t.to_string() << std::endl;
    std::cout << inv(t).to_string() << std::endl;
    std::cout << (inv(t) * t).to_string() << std::endl;

    const auto t2 = make_fq2(0xc446a57c71889fbfa5e03eddfe9c6d87971e59ae7af13a70085c93933db9766_u256,
        0x300823b181f4271b67c83280632005deb3d87159ad6f31b839cffe33a822645a_u256);

    std::cout << t2.to_string() << std::endl;
    std::cout << inv(t2).to_string() << std::endl;
    std::cout << (inv(t2) * t2).to_string() << std::endl;

    const auto t1 = Fq6({
        make_fq2(0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678_u256,
            0x209dd15ebff5d46c4bd888e51a93cf99a7329636c63514396b4a452003a35bf7_u256),
        make_fq2(0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550_u256,
            0x2bb8324af6cfc93537a2ad1a445cfd0ca2a71acd7ac41fadbf933c2a51be344d_u256),
        make_fq2(0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550_u256,
            0x2bb8324af6cfc93537a2ad1a445cfd0ca2a71acd7ac41fadbf933c2a51be344d_u256),
    });


    std::cout << t1.to_string() << std::endl;
    std::cout << inv(t1).to_string() << std::endl;
    std::cout << (inv(t1) * t1).to_string() << std::endl;


    auto n = Fq12::one();
    auto d = Fq12::one();


    //    for (size_t i = 0; i < pG1.size(); ++i)
    //    {
    //        const auto proj_pG1 = ecc::ProjPoint<Base>(
    //            Base(Fp.to_mont(pG1[i].x)), Base(Fp.to_mont(pG1[i].y)), Base(Fp.to_mont(1)));
    //        const auto proj_pG2 = ecc::ProjPoint<Fq2>(
    //            Fq2({Base(pG2[i].x), Base(0)}), Fq2({Base(pG2[i].y), Base(0)}), Fq2::one());
    //
    //        const auto [_n, _d] = miller_loop(untwist(proj_pG2), cast_to_fq12(proj_pG1));
    //
    //        n = n * _n;
    //        d = d * _d;
    //    }

    //    std::cout << n.to_string() << std::endl;
    //    std::cout << d.to_string() << std::endl;

    std::cout << (n * inv(d)).to_string() << std::endl;

    ecc::ProjPoint<Fq2> Q1 = {
        .x = make_fq2(0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678_u256,
            0x209dd15ebff5d46c4bd888e51a93cf99a7329636c63514396b4a452003a35bf7_u256),
        .y = make_fq2(0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550_u256,
            0x2bb8324af6cfc93537a2ad1a445cfd0ca2a71acd7ac41fadbf933c2a51be344d_u256),
        .z = Fq2::one(),

    };

    std::cout << Q1 << std::endl;

    const auto uQ1 = untwist(Q1);
    std::cout << uQ1 << std::endl;

    ecc::ProjPoint<Base> P1 = {
        .x = Base(
            Fp.to_mont(0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678_u256)),
        .y = Base(
            Fp.to_mont(0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550_u256)),
        .z = Base::one(),
    };

    const auto [_n, _d] = miller_loop(untwist(Q1), cast_to_fq12(P1));

    n = n * _n;
    d = d * _d;

    std::cout << (n * inv(d)).to_string() << std::endl;
    const auto P = cast_to_fq12(P1);
    std::cout << P << std::endl;

    //    auto r2 = Fq2({Base(Fp.to_mont(3)), Base(Fp.to_mont(4))}) *
    //              Fq2({Base(Fp.to_mont(2)), Base(Fp.to_mont(5))});
    //
    //    std::cout << r2.to_string() << std::endl;
    //
    //    std::cout << hex(Fp.sub(0, Fp.to_mont(9))) << std::endl;
    //    std::cout << hex(Fp.sub(0, Fp.to_mont(1))) << std::endl;
    //    std::cout << hex(Fp.to_mont(1)) << std::endl;
    //
    //
    //    auto r6 = Fq6({r2, r2, r2}) * Fq6({r2, r2, r2});
    //
    //    std::cout << r6.to_string() << std::endl;
    //
    //    auto r12 = Fq12({r6, r6}) * Fq12({r6, r6});
    //
    //    std::cout << r12.to_string() << std::endl;

    return false;
}


}  // namespace evmmax::bn254

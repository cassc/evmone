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

    explicit constexpr FieldElem() noexcept : value(ValueT{}) {}

    explicit constexpr FieldElem(const ValueT& v) noexcept : value(v) {}

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

    explicit constexpr FieldElem(CoeffArrT cs) noexcept : coeffs(cs)
    {
        static_assert(cs.size() == DEGREE);
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
            c = s;
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

    inline constexpr std::string to_string() const noexcept
    {
        std::string res;
        for (const auto& c : coeffs)
            res += c.to_string() + ", ";
        return res.substr(0, res.size() - 2);
    }

    static inline constexpr FieldElem one() noexcept
    {
        std::array<ValueT, DEGREE> v;
        v[0] = ValueT::one();
        for (size_t i = 1; i < DEGREE; ++i)
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

static inline auto omega2 = make_fq12({{{0, 0}, {1, 0}, {0, 0}}}, {{{0, 0}, {0, 0}, {0, 0}}});

static inline auto omega3 = make_fq12({{{0, 0}, {0, 0}, {0, 0}}}, {{{0, 0}, {1, 0}, {0, 0}}});

inline ecc::ProjPoint<Fq12> untwist(const ecc::ProjPoint<Fq2>& p) noexcept
{
    const auto zero = Fq2({Base(0), Base(0)});

    const ecc::ProjPoint<Fq12> ret = {
        .x = Fq12({Fq6({{p.x, zero, zero}}), Fq6({{zero, zero, zero}})}),
        .y = Fq12({Fq6({{p.y, zero, zero}}), Fq6({{zero, zero, zero}})}),
        .z = Fq12({Fq6({{p.z, zero, zero}}), Fq6({{zero, zero, zero}})}),
    };

    std::cout << ret << std::endl;

    return {ret.x * omega2, ret.y * omega3, ret.z};
}

}  // namespace

bool pairing([[maybe_unused]] const std::vector<Point>& pG1,
    [[maybe_unused]] const std::vector<Point>& pG2) noexcept
{
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

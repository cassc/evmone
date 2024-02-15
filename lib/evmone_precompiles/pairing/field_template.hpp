// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "utils.hpp"
#include <array>
#include <string>

namespace evmmax::ecc
{
template <typename BaseFieldConfigT>
struct BaseFieldElem
{
    typedef BaseFieldConfigT::ValueT ValueT;

    static const ModArith<ValueT> Fp;

    ValueT value;

    constexpr BaseFieldElem() noexcept : value(ValueT{}) {}

    constexpr BaseFieldElem(const ValueT& v) noexcept : value(v) {}

    static inline constexpr BaseFieldElem add(
        const BaseFieldElem& e1, const BaseFieldElem& e2) noexcept
    {
        return BaseFieldElem(Fp.add(e1.value, e2.value));
    }

    static inline constexpr BaseFieldElem sub(
        const BaseFieldElem& e1, const BaseFieldElem& e2) noexcept
    {
        return BaseFieldElem(Fp.sub(e1.value, e2.value));
    }

    static inline constexpr BaseFieldElem mul(
        const BaseFieldElem& e1, const BaseFieldElem& e2) noexcept
    {
        return BaseFieldElem(Fp.mul(e1.value, e2.value));
    }

    static inline constexpr BaseFieldElem mul(const BaseFieldElem& e1, const ValueT& s) noexcept
    {
        return BaseFieldElem(Fp.mul(e1.value, s));
    }

    static inline constexpr bool eq(const BaseFieldElem& e1, const BaseFieldElem& e2) noexcept
    {
        return e1.value == e2.value;
    }

    static inline constexpr bool neq(const BaseFieldElem& e1, const BaseFieldElem& e2) noexcept
    {
        return e1.value != e2.value;
    }

    static inline constexpr BaseFieldElem neg(const BaseFieldElem& e) noexcept
    {
        return BaseFieldElem(Fp.sub(typename BaseFieldConfigT::ValueT{0}, e.value));
    }

    inline constexpr BaseFieldElem inv() const noexcept
    {
        return inverse(*this);
    }

    inline constexpr std::string to_string() const noexcept { return "0x" + hex(Fp.from_mont(value)); }

    static inline constexpr BaseFieldElem one() noexcept { return BaseFieldConfigT::ONE; }

    static inline constexpr BaseFieldElem zero() noexcept { return BaseFieldElem(0); }

    inline BaseFieldElem to_mont() const { return BaseFieldElem(Fp.to_mont(value)); }
};

template <typename BaseFieldConfigT>
const ModArith<typename BaseFieldConfigT::ValueT> BaseFieldElem<BaseFieldConfigT>::Fp =
    BaseFieldConfigT::MOD_ARITH;

template <typename FieldElem>
FieldElem inverse(const FieldElem& e);

template <typename FieldConfigT>
struct FieldElem
{
    typedef FieldConfigT::ValueT ValueT;
    typedef FieldConfigT::BaseFieldT Base;
    static constexpr auto DEGREE = FieldConfigT::DEGREE;
    using CoeffArrT = std::array<ValueT, DEGREE>;
    CoeffArrT coeffs;

    explicit constexpr FieldElem() noexcept
    {
        for (size_t i = 0; i < DEGREE; ++i)
            coeffs[i] = ValueT();
    }

    explicit constexpr FieldElem(CoeffArrT cs) noexcept : coeffs(cs) {}

    inline constexpr FieldElem conjugate() const noexcept
    {
        CoeffArrT res = this->coeffs;

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

    static inline constexpr FieldElem pow(const FieldElem& e, Base::ValueT p)
    {
        auto o = one();
        auto t = e;
        while (p > 0)
        {
            if (p & 1)
                o = o * t;
            p >>= 1;
            t = t * t;
        }
        return o;
    }

    inline constexpr FieldElem inv() noexcept
    {
        return inverse(*this);
    }

    inline FieldElem to_mont() const
    {
        CoeffArrT ret;
        for (size_t i = 0; i < DEGREE; ++i)
            ret[i] = coeffs[i].to_mont();

        return FieldElem(ret);
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
inline constexpr FieldElem operator*(
    const typename FieldElem::Base::ValueT& s, const FieldElem& e) noexcept
{
    return FieldElem::mul(e, s);
}

template <typename FieldElem>
inline constexpr FieldElem operator^(
    const FieldElem& e, const typename FieldElem::Base::ValueT& p) noexcept
{
    return FieldElem::pow(e, p);
}

template <typename ValueT>
inline constexpr ecc::ProjPoint<ValueT> operator-(const ecc::ProjPoint<ValueT>& p) noexcept
{
    return {p.x, -p.y, p.z};
}
}  // namespace evmmax::ecc

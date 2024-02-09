# degree = 2
#
# def mul(a_coeffs, b_coeffs, m_coeffs):
#     r = [0 for i in range(0, degree * 2 - 1)]
#     for j in range(degree):
#         for i in range(degree):
#             r[i + j] = r[i + j] + a_coeffs[i] * b_coeffs[j]
#
#     print(r);
#
#     for i in range(0, len(r) - len(m_coeffs) + 1):
#         T = r[i]
#         for j in range(len(m_coeffs)):
#             r[i + j] = r[i + j] - T * m_coeffs[j]
#             print(r)
#
#
# def reminder(a_coeffs, m_coeffs):
#     r = [i for i in a_coeffs]
#
#     for i in range(0, len(r) - len(m_coeffs) + 1):
#         T = r[i]
#         for j in range(len(m_coeffs)):
#             r[i + j] = r[i + j] - T * m_coeffs[j]
#             print(r)
#     return r[len(a_coeffs) - len(m_coeffs) + 1:]
#
# print(reminder([3,5,6,7,2], [1, 2, 1]))
#
# mul([4, 3], [5, 2], [1, 2, 1])
#
#
# print ("---------------")
# ##################################
# def reminder(a_coeffs, m_coeffs):
#     r = [i for i in a_coeffs]
#
#     for i in range(0, len(r) - len(m_coeffs)):
#         T = r[i]
#         r[i] = 0
#         for j in range(len(m_coeffs)):
#             r[i + 1 + j] = r[i + 1 + j] - T * m_coeffs[j]
#             print(r)
#     return r[len(a_coeffs) - len(m_coeffs):]
#
# print(reminder([3,5,6,7,2], [2, 1]))
#
#
# print ("---------------")
# ##################################
# def reminder(a_coeffs, m_coeffs, degree):
#     r = [i for i in a_coeffs]
#
#     for i in range(0, len(r) - degree):
#         for j in m_coeffs.keys():
#             r[i + 1 + j] = r[i + 1 + j] - r[i] * m_coeffs[j]
#             print(r)
#     return r[len(a_coeffs) - degree:]
#
# print(reminder([3,5,6,7,2], {0: 2, 1: 1}, 2))
#
# print(reminder([3,5,6,7,2], {0: 2, 2: 1}, 3))
# print(reminder([3,5,6,7,2], {2: 1, 0: 2}, 3))
#
#
# print ("---------------")
# ##################################
# def reminder(a_coeffs, m_coeffs, degree):
#     r = [i for i in a_coeffs]
#
#     for i in range(len(r) - 1, degree - 1, -1):
#         d = i - degree
#         for j in m_coeffs.keys():
#             r[d + j] = r[d + j] - r[i] * m_coeffs[j]
#     return r
#
# print(reminder([2,7,6,5,3,0,0,0], {0: 2, 1: 1}, 2))
#
# print(reminder([2,7,6,5,3], {0: 2, 2: 1}, 3))
# print(reminder([2,7,6,5,3], {2: 1, 0: 2}, 3))

N = 0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47
R = 0x30644E72E131A029B85045B68181585D2833E84879B9709143E1F593F0000001

# num_mul = 0
# red_mul_fq2 = 0
# red_mul_fq6 = 0


class FP:
    def __init__(self, value):
        self.value = value % N

    def __add__(self, other):
        return FP(self.value + other.value)

    def __mul__(self, other):
        if isinstance(other, int):
            return FP(self.value * other)
        # global num_mul
        # num_mul += 1
        return FP(self.value * other.value)

    def __rmul__(self, other):
        return self * other

    def __sub__(self, other):
        return FP(self.value - other.value)

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return self.__str__()

    def __eq__(self, other):
        if isinstance(other, int):
            return self.value == other
        assert isinstance(other, self.__class__)
        return self.value == other.value

    def __ne__(self, other):
        return not self == other

    def __neg__(self):
        return self.__class__(-self.value)


class FQ2:
    FIELD_COEFFS = {0: FP(1)}
    DEGREE = 2

    def __init__(self, coeffs):
        assert len(coeffs) == self.DEGREE
        self.coeffs = [c if isinstance(c, FP) else FP(c) for c in coeffs]

    def mul(cls, a, b):
        r = [FP(0) for i in range(0, cls.DEGREE * 2 - 1)]
        for j in range(cls.DEGREE):
            for i in range(cls.DEGREE):
                r[i + j] = r[i + j] + a.coeffs[i] * b.coeffs[j]

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ2(r[:cls.DEGREE])

    # https://cr.yp.to/papers/m3-20010811-retypeset-20220327.pdf
    def kmul(cls, a, b):
        global red_mul_fq2
        r = [FP(0) for i in range(0, cls.DEGREE * 2 - 1)]

        t = a.coeffs[0] * b.coeffs[0]
        u = a.coeffs[1] * b.coeffs[1]

        r[0] = t
        r[1] = (a.coeffs[0] + a.coeffs[1]) * (b.coeffs[0] + b.coeffs[1]) - u - t
        r[2] = u

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                #red_mul_fq2 = red_mul_fq2 + 1
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ2(r[:cls.DEGREE])

    def __mul__(self, other):
        if isinstance(other, (int)):
            return self.__class__([c * other for c in self.coeffs])
        return self.kmul(self, other)

    def __rmul__(self, other):
        return self * other

    def __add__(self, other):
        return FQ2([self.coeffs[i] + other.coeffs[i] for i in range(self.DEGREE)])

    def __sub__(self, other):
        assert isinstance(other, FQ2)
        return FQ2([(x - y) for x, y in zip(self.coeffs, other.coeffs)])

    def __str__(self):
        return str([c for c in self.coeffs])

    def __repr__(self):
        return self.__str__()

    def __eq__(self, other):
        assert isinstance(other, self.__class__)

        return self.coeffs[0] == other.coeffs[0] and self.coeffs[1] == other.coeffs[1]

    def __ne__(self, other):
        return not self == other

    def __neg__(self):
        return self.__class__([-c for c in self.coeffs])

    @classmethod
    def one(cls):
        return cls([1, 0])

    @classmethod
    def zero(cls):
        return cls([0, 0])

    def deg(self):
        return 1 if self.coeffs[1] != 0 else 0


class FQ6:
    FIELD_COEFFS = {0: FQ2([FP(-9), FP(-1)])}
    DEGREE = 3

    def __init__(self, coeffs):
        assert len(coeffs) == self.DEGREE
        self.coeffs = coeffs

    def mul(cls, a, b):
        r = [FQ2([0, 0]) for i in range(0, cls.DEGREE * 2 - 1)]
        for j in range(cls.DEGREE):
            for i in range(cls.DEGREE):
                r[i + j] = r[i + j] + a.coeffs[i] * b.coeffs[j]

        # print(r)

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ6(r[:cls.DEGREE])

    def kmul(cls, a, b):
        r = [FQ2([0, 0]) for i in range(0, cls.DEGREE * 2 - 1)]

        s = a.coeffs[0] * b.coeffs[0]
        t = a.coeffs[1] * b.coeffs[1]
        v = a.coeffs[2] * b.coeffs[2]

        r[0] = s
        r[1] = (a.coeffs[0] + a.coeffs[1]) * (b.coeffs[0] + b.coeffs[1]) - s - t
        r[2] = t + (a.coeffs[0] + a.coeffs[2]) * (b.coeffs[0] + b.coeffs[2]) - s - v
        r[3] = (a.coeffs[1] + a.coeffs[2]) * (b.coeffs[1] + b.coeffs[2]) - t - v
        r[4] = v

        # print(r)

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ6(r[:cls.DEGREE])

    def __mul__(self, other):
        if isinstance(other, (int)):
            return self.__class__([c * other for c in self.coeffs])
        return self.kmul(self, other)

    def __rmul__(self, other):
        return self * other

    def __add__(self, other):
        return FQ6([self.coeffs[i] + other.coeffs[i] for i in range(self.DEGREE)])

    def __sub__(self, other):
        assert isinstance(other, FQ6)
        return FQ6([(x - y) for x, y in zip(self.coeffs, other.coeffs)])

    def __str__(self):
        return str([c for c in self.coeffs])

    def __repr__(self):
        return self.__str__()

    def __eq__(self, other):
        assert isinstance(other, self.__class__)

        return self.coeffs[0] == other.coeffs[0] and self.coeffs[1] == other.coeffs[1] and self.coeffs[2] == \
            other.coeffs[2]

    def __ne__(self, other):
        return not self == other

    def __neg__(self):
        return self.__class__([-c for c in self.coeffs])

    @classmethod
    def one(cls):
        return cls([FQ2.one()] + [FQ2.zero()] * (cls.DEGREE - 1))

    @classmethod
    def zero(cls):
        return cls([FQ2.zero()] * (cls.DEGREE))


class FQ6N:
    FIELD_COEFFS = {3: FP(-18), 0: FP(82)}
    DEGREE = 6

    def __init__(self, coeffs):
        assert len(coeffs) == self.DEGREE
        self.coeffs = coeffs

    def mul(cls, a, b):
        r = [FP(0) for i in range(0, cls.DEGREE * 2 - 1)]
        for j in range(cls.DEGREE):
            for i in range(cls.DEGREE):
                r[i + j] = (r[i + j] + a.coeffs[i] * b.coeffs[j])

        # print(r)

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ6N(r[:cls.DEGREE])

    def __mul__(self, other):
        return self.mul(self, other)

    def __str__(self):
        return str([c for c in self.coeffs])

    def __repr__(self):
        return self.__str__()


# Extended euclidean algorithm to find modular inverses for
# integers
def prime_field_inv(a, n):
    if a == 0:
        return 0
    lm, hm = 1, 0
    low, high = a % n, n
    while low > 1:
        r = high//low
        nm, new = hm-lm*r, high-low*r
        lm, low, hm, high = nm, new, lm, low
    return lm % n


# Utility methods for polynomial math
def deg(p):
    d = len(p) - 1
    while p[d] == 0 and d:
        d -= 1
    return d


def poly_rounded_div(a, b):
    dega = deg(a)
    degb = deg(b)
    temp = [x for x in a]
    o = [0 for x in a]
    for i in range(dega - degb, -1, -1):
        o[i] = (o[i] + temp[degb + i] * prime_field_inv(b[degb], N))
        for c in range(degb + 1):
            temp[c + i] = (temp[c + i] - o[c])
    return [x % N for x in o[:deg(o)+1]]


class FQ12:
    FIELD_COEFFS = {0: FQ6([FQ2([0, 0]), FQ2([-1, 0]), FQ2([0, 0])])}
    DEGREE = 2

    def __init__(self, coeffs):
        assert len(coeffs) == self.DEGREE
        self.coeffs = coeffs

    def mul(cls, a, b):
        r = [FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])]) for i in range(0, cls.DEGREE * 2 - 1)]
        for j in range(cls.DEGREE):
            for i in range(cls.DEGREE):
                r[i + j] = r[i + j] + a.coeffs[i] * b.coeffs[j]

        # print(r)

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ12(r[:cls.DEGREE])

    def kmul(cls, a, b):
        r = [FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])]) for i in range(0, cls.DEGREE * 2 - 1)]

        t = a.coeffs[0] * b.coeffs[0]
        u = a.coeffs[1] * b.coeffs[1]

        r[0] = t
        r[1] = (a.coeffs[0] + a.coeffs[1]) * (b.coeffs[0] + b.coeffs[1]) - u - t
        r[2] = u

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ12(r[:cls.DEGREE])

    def __pow__(self, other):
        o = self.__class__.one()
        t = self
        while other > 0:
            if other & 1:
                o = o * t
            other >>= 1
            t = t * t
        return o

    def __mul__(self, other):
        if isinstance(other, (int)):
            return self.__class__([c * other for c in self.coeffs])

        return self.kmul(self, other)

    def __rmul__(self, other):
        return self * other

    def __sub__(self, other):
        assert isinstance(other, FQ12)
        return FQ12([(x - y) for x, y in zip(self.coeffs, other.coeffs)])

    def __str__(self):
        return str([c for c in self.coeffs])

    def __repr__(self):
        return self.__str__()

    def __eq__(self, other):
        assert isinstance(other, self.__class__)

        return self.coeffs[0] == other.coeffs[0] and self.coeffs[1] == other.coeffs[1]

    def __ne__(self, other):
        return not self == other

    def __neg__(self):
        return self.__class__([-c for c in self.coeffs])

    @classmethod
    def zero(cls):
        return cls([FQ6.zero()] * cls.DEGREE)

    @classmethod
    def one(cls):
        return cls([FQ6.one(), FQ6.zero()])

    def __div__(self, other):
        return self * other.inv()

    # Extended euclidean algorithm used to find the modular inverse
    def inv(self):
        lm, hm = [1] + [0] * self.degree, [0] * (self.degree + 1)
        low, high = self.coeffs + [0], self.modulus_coeffs + [1]
        while deg(low):
            r = poly_rounded_div(high, low)
            r += [0] * (self.degree + 1 - len(r))
            nm = [x for x in hm]
            new = [x for x in high]
            # assert len(lm) == len(hm) == len(low) == len(high) == len(nm) == len(new) == self.degree + 1
            for i in range(self.degree + 1):
                for j in range(self.degree + 1 - i):
                    nm[i+j] -= lm[i] * r[j]
                    new[i+j] -= low[i] * r[j]
            nm = [x % N for x in nm]
            new = [x % N for x in new]
            lm, low, hm, high = nm, new, lm, low
        return self.__class__(lm[:self.degree]) / low[0]


class FQ12N:
    FIELD_COEFFS = {6: FP(-18), 0: FP(82)}
    DEGREE = 12

    def __init__(self, coeffs):
        assert len(coeffs) == self.DEGREE
        self.coeffs = coeffs

    def mul(cls, a, b):
        r = [FP(0) for i in range(0, cls.DEGREE * 2 - 1)]
        for j in range(cls.DEGREE):
            for i in range(cls.DEGREE):
                r[i + j] = (r[i + j] + a.coeffs[i] * b.coeffs[j])

        # print(r)

        for i in range(len(r) - 1, cls.DEGREE - 1, -1):
            d = i - cls.DEGREE
            for j in cls.FIELD_COEFFS.keys():
                r[d + j] = r[d + j] - r[i] * cls.FIELD_COEFFS[j]

        return FQ12N(r[:cls.DEGREE])

    def __mul__(self, other):
        return self.mul(self, other)

    def __str__(self):
        return str([c for c in self.coeffs])

    def __repr__(self):
        return self.__str__()


# x = FQ6([FQ2([FP(1), FP(2)]), FQ2([FP(3), FP(4)]), FQ2([FP(5), FP(6)])])
# print(x)
# y = FQ6([FQ2([FP(7), FP(8)]), FQ2([FP(9), FP(10)]), FQ2([FP(11), FP(12)])])
# print(y)
# fq = x * y
# print(str(fq))
# # print(num_mul)
# # print(red_mul_fq2)
# # print(red_mul_fq6)
#
# # [FQ2([c.coeffs[0] + FP(9) * c.coeffs[1], c.coeffs[1]]) for c in x.coeffs]
#
# x1 = FQ6N([FP(1) - FP(9) * FP(2), FP(3) - FP(9) * FP(4), FP(5) - FP(9) * FP(6), FP(2), FP(4), FP(6)])
# print(x1)
# y1 = FQ6N([FP(7) - FP(9) * FP(8), FP(9) - FP(9) * FP(10), FP(11) - FP(9) * FP(12), FP(8), FP(10), FP(12)])
# print(y1)
#
# # num_mul = 0
# # red_mul_fq2 = 0
# # red_mul_fq6 = 0
# fq2 = x1 * y1
# print(str(fq2))
# # print(num_mul)
# # print(red_mul_fq2)
# # print(red_mul_fq6)
#
# fq2 = [fq2.coeffs[i] + FP(9) * fq2.coeffs[i + 3] for i in range(3)] + fq2.coeffs[3:]
# print(str(fq2))
#
# x = FQ12([FQ6([FQ2([FP(1), FP(2)]), FQ2([FP(3), FP(4)]), FQ2([FP(5), FP(6)])]),
#           FQ6([FQ2([FP(7), FP(8)]), FQ2([FP(9), FP(10)]), FQ2([FP(11), FP(12)])])])
# print(x)
# y = FQ12([FQ6([FQ2([FP(13), FP(14)]), FQ2([FP(15), FP(16)]), FQ2([FP(17), FP(18)])]),
#           FQ6([FQ2([FP(19), FP(20)]), FQ2([FP(21), FP(22)]), FQ2([FP(23), FP(24)])])])
# print(y)
#
# # num_mul = 0
# # red_mul_fq2 = 0
# # red_mul_fq6 = 0
# fq = x * y
# print(str(fq))
# # print(num_mul)
# # print(red_mul_fq2)
# # print(red_mul_fq6)
#
# x1 = FQ12N([
#     FP(1) - FP(9) * FP(2),
#     FP(7) - FP(9) * FP(8),
#     FP(3) - FP(9) * FP(4),
#     FP(9) - FP(9) * FP(10),
#     FP(5) - FP(9) * FP(6),
#     FP(11) - FP(9) * FP(12),
#     FP(2), FP(8), FP(4),
#     FP(10), FP(6), FP(12)])
# print(x1)
# y1 = FQ12N([
#     FP(13) - FP(9) * FP(14),
#     FP(19) - FP(9) * FP(20),
#     FP(15) - FP(9) * FP(16),
#     FP(21) - FP(9) * FP(22),
#     FP(17) - FP(9) * FP(18),
#     FP(23) - FP(9) * FP(24),
#     FP(14), FP(20), FP(16),
#     FP(22), FP(18), FP(24)])
# print(y1)
#
# # num_mul = 0
# # red_mul_fq2 = 0
# # red_mul_fq6 = 0
#
# fq2 = x1 * y1
# print(str(fq2))
#
# # print(num_mul)
# # print(red_mul_fq2)
# # print(red_mul_fq6)
#
# fq2 = [fq2.coeffs[i] + FP(9) * fq2.coeffs[i + 6] for i in range(6)] + fq2.coeffs[6:]
# print(str(fq2))


class Point:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return "x: " + str(self.x) + "\n" + "y: " + str(self.y) + "\n" + "z: " + str(self.z) + "\n"

    def __repr__(self):
        return self.__str__()

    def __neg__(self):
        return self.__class__(self.x, -self.y, self.z)


# w = FQ12([FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([1, 0]), FQ2([0, 0]), FQ2([0, 0])])])
#
# w2 = w * w
#
w2 = FQ12([FQ6([FQ2([0, 0]), FQ2([1, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])
#
w3 = FQ12([FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([1, 0]), FQ2([0, 0])])])
# w3 = w * w * w
# w3 = w2 * w
# print(w3 * w3)


def untwist(pt: Point):
    # x = FQ2([pt.x.coeffs[0] - FP(9) * pt.x.coeffs[1], pt.x.coeffs[1]])
    # y = FQ2([pt.y.coeffs[0] - FP(9) * pt.y.coeffs[1], pt.y.coeffs[1]])
    # z = FQ2([pt.z.coeffs[0] - FP(9) * pt.z.coeffs[1], pt.z.coeffs[1]])
    # #
    # x = FQ12([FQ6([x, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])
    # y = FQ12([FQ6([y, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])
    # z = FQ12([FQ6([z, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])

    x = FQ12([FQ6([pt.x, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])
    y = FQ12([FQ6([pt.y, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])
    z = FQ12([FQ6([pt.z, FQ2([0, 0]), FQ2([0, 0])]), FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])])

    return Point(x * w2, y * w3, z)


G2 = Point(FQ2([10857046999023057135944570762232829481370756359578518086990519993285655852781,
                11559732032986387107991004021392285783925812861821192530917403151452391805634]),
           FQ2([8495653923123431417604973247489272438418190587263600148770280649306958101930,
                4082367875863433681332203403145435568316851327593401208105741076214120093531]), FQ2([1, 0]))


def linear_func(P1: Point, P2: Point, T: Point):
    zero = P1.x.__class__.zero()

    n = P2.y * P1.z - P1.y * P2.z
    d = P2.x * P1.z - P1.x * P2.z

    if d != zero:
        return n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z
    elif n == zero:
        n = 3 * P1.x * P1.x
        d = 2 * P1.y * P1.z
        return n * (T.x * P1.z - T.z * P1.x) - d * (T.y * P1.z - P1.y * T.z), d * T.z * P1.z
    else:
        return T.x * P1.z - P1.x * T.z, T.z * P1.z



def double(pt):
    x, y, z = pt.x, pt.y, pt.z
    W = 3 * x * x
    S = y * z
    B = x * y * S
    H = W * W - 8 * B
    S_squared = S * S
    newx = 2 * H * S
    newy = W * (4 * B - H) - 8 * y * y * S_squared
    newz = 8 * S * S_squared
    return Point(newx, newy, newz)


# Elliptic curve addition
def add(p1, p2):
    one, zero = p1.x.__class__.one(), p1.x.__class__.zero()
    if p1.z == zero or p2.z == zero:
        return p1 if p2.z == zero else p2
    x1, y1, z1 = p1.x, p1.y, p1.z
    x2, y2, z2 = p2.x, p2.y, p2.z
    U1 = y2 * z1
    U2 = y1 * z2
    V1 = x2 * z1
    V2 = x1 * z2
    if V1 == V2 and U1 == U2:
        return double(p1)
    elif V1 == V2:
        return Point(one, one, zero)
    U = U1 - U2
    V = V1 - V2
    V_squared = V * V
    V_squared_times_V2 = V_squared * V2
    V_cubed = V * V_squared
    W = z1 * z2
    A = U * U * W - V_cubed - 2 * V_squared_times_V2
    newx = V * A
    newy = U * (V_squared_times_V2 - A) - V_cubed * U2
    newz = V_cubed * W
    return Point(newx, newy, newz)


def frobenius_endomophism(pt: Point):
    return Point(pt.x ** N, pt.y ** N, pt.z ** N)


ate_loop_count = 29793968203157093288
log_ate_loop_count = 63


def miller_loop(Q, P):
    R = Q
    f_num = FQ12.one()
    f_den = FQ12.one()

    print (Q)

    for i in range(log_ate_loop_count, -1, -1):
        print ("miller loop iter " + str(i))
        _n, _d = linear_func(R, R, P)
        R = double(R)
        f_num = f_num * f_num * _n
        f_den = f_den * f_den * _d

        print (f_num)
        print (f_den)

        if ate_loop_count & (2 ** i):
            _n, _d = linear_func(R, Q, P)
            R = add(R, Q)
            f_num = f_num * _n
            f_den = f_den * _d

    print (f_num)
    print (f_den)

    Q1 = frobenius_endomophism(Q)
    nQ2 = -frobenius_endomophism(Q1)

    print(Q1)
    print(nQ2)

    _n1, _d1 = linear_func(R, Q1, P)
    R = add(R, Q1)
    _n2, _d2 = linear_func(R, nQ2, P)

    return f_num * _n1 * _n2, f_den * _d1 * _d2


def final_exp(f: FQ12):
    return f ** ((N ** 12 - 1) // R)


def cast_to_fq12(pt: Point):
    return Point(
        FQ12([
            FQ6([FQ2([pt.x, 0]), FQ2([0, 0]), FQ2([0, 0])]),
            FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])
        ]),
        FQ12([
            FQ6([FQ2([pt.y, 0]), FQ2([0, 0]), FQ2([0, 0])]),
            FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])
        ]),
        FQ12([
            FQ6([FQ2([pt.z, 0]), FQ2([0, 0]), FQ2([0, 0])]),
            FQ6([FQ2([0, 0]), FQ2([0, 0]), FQ2([0, 0])])
        ]))


def pairing(Q: Point, P: Point):

    f = miller_loop(untwist(Q), cast_to_fq12(P))

    #return final_exp(f)
    return f

P1 = Point(FP(0x1c76476f4def4bb94541d57ebba1193381ffa7aa76ada664dd31c16024c43f59), FP(0x3034dd2920f673e204fee2811c678745fc819b55d3e9d294e45c9b03a76aef41), FP(1))
Q1 = Point(FQ2([0x04bf11ca01483bfa8b34b43561848d28905960114c8ac04049af4b6315a41678, 0x209dd15ebff5d46c4bd888e51a93cf99a7329636c63514396b4a452003a35bf7]), FQ2([0x120a2a4cf30c1bf9845f20c6fe39e07ea2cce61f0c9bb048165fe5e4de877550, 0x2bb8324af6cfc93537a2ad1a445cfd0ca2a71acd7ac41fadbf933c2a51be344d]), FQ2.one())

# print (Q1)
# tQ1 = untwist(Q1)
# print(tQ1)
#
# r = tQ1.x * tQ1.x * tQ1.x
# print (r)


p = pairing(Q1, P1)
print(p)

# Generator for curve over FQ
G1 = Point(FP(1), FP(2), FP(1))
# Generator for twisted curve over FQ2
G2 = Point(FQ2([10857046999023057135944570762232829481370756359578518086990519993285655852781, 11559732032986387107991004021392285783925812861821192530917403151452391805634]),
      FQ2([8495653923123431417604973247489272438418190587263600148770280649306958101930, 4082367875863433681332203403145435568316851327593401208105741076214120093531]), FQ2.one())


p = pairing(G2, G1)
p_inv = pairing(G2, -G1)

assert p * p_inv == FQ12.one()

print(untwist(G2))
# fq2 = FQ6N([FP(1), FP(2), FP(3), FP(4), FP(3), FP(1)]) * FQ6N([FP(2), FP(2), FP(1), FP(1), FP(2), FP(2)])
# print(str(fq2))

# fq2 = FQ6N([1,2,3,4,5,6]) * FQ6N([7,8,9,10,11,12])
# print (str(fq2))

# print (FQ2([1, 2]) * FQ2([1, 3]))


degree = 2

def mul(a_coeffs, b_coeffs, m_coeffs):
    r = [0 for i in range(0, degree * 2 - 1)]
    for j in range(degree):
        for i in range(degree):
            r[i + j] = r[i + j] + a_coeffs[i] * b_coeffs[j]

    print(r);

    for i in range(0, len(r) - len(m_coeffs) + 1):
        T = r[i]
        for j in range(len(m_coeffs)):
            r[i + j] = r[i + j] - T * m_coeffs[j]
            print(r)


def reminder(a_coeffs, m_coeffs):
    r = [i for i in a_coeffs]

    for i in range(0, len(r) - len(m_coeffs) + 1):
        T = r[i]
        for j in range(len(m_coeffs)):
            r[i + j] = r[i + j] - T * m_coeffs[j]
            print(r)
    return r[len(a_coeffs) - len(m_coeffs) + 1:]

print(reminder([3,5,6,7,2], [1, 2, 1]))

mul([4, 3], [5, 2], [1, 2, 1])


print ("---------------")
##################################
def reminder(a_coeffs, m_coeffs):
    r = [i for i in a_coeffs]

    for i in range(0, len(r) - len(m_coeffs)):
        T = r[i]
        r[i] = 0
        for j in range(len(m_coeffs)):
            r[i + 1 + j] = r[i + 1 + j] - T * m_coeffs[j]
            print(r)
    return r[len(a_coeffs) - len(m_coeffs):]

print(reminder([3,5,6,7,2], [2, 1]))


print ("---------------")
##################################
def reminder(a_coeffs, m_coeffs, degree):
    r = [i for i in a_coeffs]

    for i in range(0, len(r) - degree):
        for j in m_coeffs.keys():
            r[i + 1 + j] = r[i + 1 + j] - r[i] * m_coeffs[j]
            print(r)
    return r[len(a_coeffs) - degree:]

print(reminder([3,5,6,7,2], {0: 2, 1: 1}, 2))

print(reminder([3,5,6,7,2], {0: 2, 2: 1}, 3))
print(reminder([3,5,6,7,2], {2: 1, 0: 2}, 3))


print ("---------------")
##################################
def reminder(a_coeffs, m_coeffs, degree):
    r = [i for i in a_coeffs]

    for i in range(len(r) - 1, degree - 1, -1):
        for j in m_coeffs.keys():
            r[i - j - 1] = r[i - j - 1] - r[i] * m_coeffs[j]
            print(r)
    return r[:degree]

print(reminder([2,7,6,5,3], {0: 2, 1: 1}, 2))

print(reminder([2,7,6,5,3], {0: 2, 2: 1}, 3))
print(reminder([2,7,6,5,3], {2: 1, 0: 2}, 3))
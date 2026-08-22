#include "matrix.h"

namespace std {
    #define IMPLEMENT_ELEMENTWISE_UNARY(func_name)         \
    Matrix func_name(const Matrix& a) {                    \
        Matrix result(a);                                  \
        for (double& val : result) {                       \
            val = std::func_name(val);                     \
        }                                                  \
        return result;                                     \
    }
    Matrix pow(const Matrix& a, double e) {
        Matrix result(a);
        for (double& val : result) {
            val = std::pow(val, e);
        }
        return result;
    }
    Matrix pow(const Matrix& a, const Matrix& b) {
        assert(a.w() == b.w() && a.h() == b.h());
        Matrix result(a);
        const double* b_ptr = b.begin();
        for (double& val : result) {
            val = std::pow(val, *b_ptr);
            b_ptr++;
        }
        return result;
    }
    IMPLEMENT_ELEMENTWISE_UNARY(abs)
    IMPLEMENT_ELEMENTWISE_UNARY(sin)
    IMPLEMENT_ELEMENTWISE_UNARY(cos)
    IMPLEMENT_ELEMENTWISE_UNARY(tan)
    IMPLEMENT_ELEMENTWISE_UNARY(asin)
    IMPLEMENT_ELEMENTWISE_UNARY(acos)
    IMPLEMENT_ELEMENTWISE_UNARY(atan)
    IMPLEMENT_ELEMENTWISE_UNARY(sinh)
    IMPLEMENT_ELEMENTWISE_UNARY(cosh)
    IMPLEMENT_ELEMENTWISE_UNARY(tanh)
    IMPLEMENT_ELEMENTWISE_UNARY(exp)
    IMPLEMENT_ELEMENTWISE_UNARY(log)
    IMPLEMENT_ELEMENTWISE_UNARY(log2)
    IMPLEMENT_ELEMENTWISE_UNARY(log10)
    #undef IMPLEMENT_ELEMENTWISE_UNARY
}

namespace funtalk {
    Matrix mulm(const Matrix& a, const Matrix& b) {
        assert(a.w() == b.h());
        Matrix result(b.w(), a.h());
        for (uint32_t r = 0; r < a.h(); ++r) {
            for (uint32_t c = 0; c < b.w(); ++c) {
                double sum = 0.0;
                for (uint32_t k = 0; k < a.w(); ++k) {
                    sum += a.get(k, r) * b.get(c, k);
                }
                result.get(c, r) = sum;
            }
        }
        return result;
    }
    Matrix invm(const Matrix& a) {
        assert(a.w() == a.h());
        assert(std::abs(a.det()) > 1e-9);
        uint32_t n = a.w();
        Matrix aug(n * 2, n);
        for (uint32_t r = 0; r < n; ++r) {
            for (uint32_t c = 0; c < n; ++c) {
                aug.get(c, r) = a.get(c, r);
            }
            aug.get(n + r, r) = 1.0;
        }
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t pivot = i;
            for (uint32_t r = i + 1; r < n; ++r) {
                if (std::abs(aug.get(i, r)) > std::abs(aug.get(i, pivot))) {
                    pivot = r;
                }
            }
            if (pivot != i) {
                for (uint32_t c = 0; c < n * 2; ++c) {
                    std::swap(aug.get(c, i), aug.get(c, pivot));
                }
            }
            double factor = aug.get(i, i);
            for (uint32_t c = 0; c < n * 2; ++c) {
                aug.get(c, i) /= factor;
            }
            for (uint32_t r = 0; r < n; ++r) {
                if (r != i) {
                    double f = aug.get(i, r);
                    for (uint32_t c = 0; c < n * 2; ++c) {
                        aug.get(c, r) -= f * aug.get(c, i);
                    }
                }
            }
        }
        Matrix result(n, n);
        for (uint32_t r = 0; r < n; ++r) {
            for (uint32_t c = 0; c < n; ++c) {
                result.get(c, r) = aug.get(n + c, r);
            }
        }
        return result;
    }
    Matrix powm(const Matrix& a, int e) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        for (uint32_t i = 0; i < n; ++i) {
            result.get(i, i) = 1.0;
        }
        Matrix base = (e < 0) ? invm(a) : a;
        unsigned int p = std::abs(e);
        while (p > 0) {
            if (p & 1) result = mulm(result, base);
            base = mulm(base, base);
            p >>= 1;
        }
        return result;
    }
    Matrix expm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        Matrix term(n, n);
        for (uint32_t i = 0; i < n; ++i) {
            result.get(i, i) = 1.0;
            term.get(i, i) = 1.0;
        }
        for (int i = 1; i <= 20; ++i) {
            term = mulm(term, a);
            term *= (1.0 / i);
            result += term;
        }
        return result;
    }
    Matrix sinm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        Matrix term = a;
        Matrix a_sq = mulm(a, a);
        result += term;
        for (int i = 1; i <= 10; ++i) {
            term = mulm(term, a_sq);
            term *= -1.0 / ((2 * i) * (2 * i + 1));
            result += term;
        }
        return result;
    }
    Matrix cosm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        Matrix term(n, n);
        for (uint32_t i = 0; i < n; ++i) result.get(i, i) = 1.0;
        Matrix a_sq = mulm(a, a);
        for (int i = 1; i <= 10; ++i) {
            term = mulm(term, a_sq);
            term *= -1.0 / ((2 * i - 1) * (2 * i));
            result += term;
        }
        return result;
    }
    Matrix tanm(const Matrix& a) {
        return mulm(sinm(a), invm(cosm(a)));
    }
    Matrix sinhm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        Matrix term = a;
        Matrix a_sq = mulm(a, a);
        result += term;
        for (int i = 1; i <= 10; ++i) {
            term = mulm(term, a_sq);
            term *= 1.0 / ((2 * i) * (2 * i + 1));
            result += term;
        }
        return result;
    }
    Matrix coshm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        Matrix term(n, n);
        for (uint32_t i = 0; i < n; ++i) result.get(i, i) = 1.0;
        Matrix a_sq = mulm(a, a);
        for (int i = 1; i <= 10; ++i) {
            term = mulm(term, a_sq);
            term *= 1.0 / ((2 * i - 1) * (2 * i));
            result += term;
        }
        return result;
    }
    Matrix tanhm(const Matrix& a) {
        return mulm(sinhm(a), invm(coshm(a)));
    }
    Matrix logm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix identity(n, n);
        for (uint32_t i = 0; i < n; ++i) identity.get(i, i) = 1.0;
        Matrix j = a;
        for (uint32_t i = 0; i < n; ++i) j.get(i, i) -= 1.0;
        Matrix result(n, n);
        Matrix term = j;
        result += term;
        for (int i = 2; i <= 20; ++i) {
            term = mulm(term, j);
            Matrix current_term = term;
            current_term *= ((i % 2 == 0) ? -1.0 : 1.0) / i;
            result += current_term;
        }
        return result;
    }
    Matrix log2m(const Matrix& a) {
        Matrix ln_a = logm(a);
        ln_a *= (1.0 / std::log(2.0));
        return ln_a;
    }
    Matrix log10m(const Matrix& a) {
        Matrix ln_a = logm(a);
        ln_a *= (1.0 / std::log(10.0));
        return ln_a;
    }
    Matrix asinm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result = a;
        Matrix term = a;
        Matrix a_sq = mulm(a, a);
        double num = 1.0;
        double den = 2.0;
        for (int i = 1; i <= 10; ++i) {
            term = mulm(term, a_sq);
            Matrix current = term;
            current *= (num / den) / (2 * i + 1);
            result += current;
            num *= (2 * i + 1);
            den *= (2 * i + 2);
        }
        return result;
    }
    Matrix acosm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result(n, n);
        for (uint32_t i = 0; i < n; ++i) result.get(i, i) = 1.5707963267948966;
        result += -asinm(a);
        return result;
    }
    Matrix atanm(const Matrix& a) {
        assert(a.w() == a.h());
        uint32_t n = a.w();
        Matrix result = a;
        Matrix term = a;
        Matrix a_sq = mulm(a, a);
        for (int i = 1; i <= 15; ++i) {
            term = mulm(term, a_sq);
            Matrix current = term;
            current *= ((i % 2 == 0) ? 1.0 : -1.0) / (2 * i + 1);
            result += current;
        }
        return result;
    }
}


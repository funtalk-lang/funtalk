#pragma once

class Matrix {
    double* arr;
    uint32_t sizes[2];
public:
    Matrix() {
        sizes[0] = 0;
        sizes[1] = 0;
        arr = nullptr;
    }
    Matrix(uint32_t w, uint32_t h) {
        sizes[0] = w;
        sizes[1] = h;
        assert(size() <= MAX_MATRIX_SIZE);
        assert(w != 0);
        assert(h != 0);
        arr = new double[w * h]{};
    }
    Matrix(const Matrix& m) {
        if(m.arr == nullptr) {
            sizes[0] = 0;
            sizes[1] = 0;
            arr = nullptr;
            return;
        }
        arr = new double[m.size()];
        memcpy(arr, m.arr, m.size() * sizeof(double));
        sizes[0] = m.sizes[0];
        sizes[1] = m.sizes[1];
    }
    Matrix(Matrix&& m) {
        arr = m.arr;
        m.arr = nullptr;
        sizes[0] = m.sizes[0];
        sizes[1] = m.sizes[1];
        m.sizes[0] = 0;
        m.sizes[1] = 0;
    }
    ~Matrix() {
        delete[] arr;
    }
    Matrix& operator=(const Matrix& m) {
        if (this == &m) {
            return *this;
        }
        delete[] arr;
        if(m.arr == nullptr) {
            sizes[0] = 0;
            sizes[1] = 0;
            arr = nullptr;
            return *this;
        }
        arr = new double[m.size()];
        memcpy(arr, m.arr, m.size() * sizeof(double));
        sizes[0] = m.sizes[0];
        sizes[1] = m.sizes[1];
        return *this;
    }
    Matrix& operator=(Matrix&& m) {
        if (this == &m) {
            return *this;
        }
        delete[] arr;
        arr = m.arr;
        m.arr = nullptr;
        sizes[0] = m.sizes[0];
        sizes[1] = m.sizes[1];
        m.sizes[0] = 0;
        m.sizes[1] = 0;
        return *this;
    }
    uint32_t w() const {
        return sizes[0];
    }
    uint32_t h() const {
        return sizes[1];
    }
    uint32_t size() const {
        return w() * h();
    }
    double& get(uint32_t x, uint32_t y) {
        assert(x < w());
        assert(y < h());
        return arr[x + y * w()];
    }
    double get(uint32_t x, uint32_t y) const {
        assert(x < w());
        assert(y < h());
        return arr[x + y * w()];
    }
    double* begin() {
        return arr;
    }
    double* end() {
        return arr + size();
    }
    const double* begin() const {
        return arr;
    }
    const double* end() const {
        return arr + size();
    }
    bool operator==(const Matrix& m) const {
        if (w() != m.w() || h() != m.h()) {
            return false;
        }
        if (size() == 0) {
            return true;
        }
        return std::memcmp(arr, m.arr, size() * sizeof(double)) == 0;
    }
    Matrix& operator+=(const Matrix& m) {
        assert(w() == m.w());
        assert(h() == m.h());
        for (uint32_t i = 0; i < size(); i++) {
            arr[i] += m.arr[i];
        }
        return *this;
    }
    Matrix& operator*=(double a) {
        for (uint32_t i = 0; i < size(); i++) {
            arr[i] *= a;
        }
        return *this;
    }
    Matrix& operator*=(const Matrix& m) {
        assert(w() == m.w());
        assert(h() == m.h());
        for (uint32_t i = 0; i < size(); i++) {
            arr[i] *= m.arr[i];
        }
        return *this;
    }
    Matrix operator+(const Matrix& m) const {
        Matrix m1 = *this;
        m1 += m;
        return m1;
    }
    Matrix operator*(double a) const {
        Matrix m1 = *this;
        m1 *= a;
        return m1;
    }
    friend Matrix operator*(double a, const Matrix& m) {
        return m * a;
    }
    Matrix operator*(const Matrix& m) const {
        Matrix m1 = *this;
        m1 *= m;
        return m1;
    }
    Matrix operator-() const {
        Matrix m = Matrix(*this);
        for (uint32_t i = 0; i < size(); i++) {
            m.begin()[i] = -begin()[i];
        }
        return m;
    }
    Matrix& operator%=(double d) {
        for (double& a : *this) {
            a = std::fmod(a, d);
        }
        return *this;
    }
    Matrix operator%(double d) const {
        Matrix m = *this;
        m %= d;
        return m;
    }
    Matrix inv() const {
        Matrix m = Matrix(*this);
        for (uint32_t i = 0; i < size(); i++) {
            m.begin()[i] = 1 / begin()[i];
        }
        return m;
    }
    double det() const {
        assert(w() == h());
        uint32_t n = w();
        if (n == 0) return 1.0;
        Matrix temp(*this);
        double det_val = 1.0;
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t pivot = i;
            for (uint32_t r = i + 1; r < n; ++r) {
                if (std::abs(temp.get(i, r)) > std::abs(temp.get(i, pivot))) {
                    pivot = r;
                }
            }
            if (std::abs(temp.get(i, pivot)) < 1e-9) return 0.0;
            if (pivot != i) {
                for (uint32_t c = 0; c < n; ++c) {
                    std::swap(temp.get(c, i), temp.get(c, pivot));
                }
                det_val *= -1.0;
            }
            det_val *= temp.get(i, i);
            for (uint32_t r = i + 1; r < n; ++r) {
                double factor = temp.get(i, r) / temp.get(i, i);
                for (uint32_t c = i; c < n; ++c) {
                    temp.get(c, r) -= factor * temp.get(c, i);
                }
            }
        }
        return det_val;
    }
    Matrix transpose() const {
        Matrix result(h(), w());
        for (uint32_t y = 0; y < h(); ++y) {
            for (uint32_t x = 0; x < w(); ++x) {
                result.get(y, x) = get(x, y);
            }
        }
        return result;
    }
    Matrix drop_rows(int n) const {
        uint32_t drop_count = std::abs(n);
        assert(drop_count < h());
        uint32_t new_h = h() - drop_count;
        Matrix result(w(), new_h);
        uint32_t start_row = (n > 0) ? drop_count : 0;
        for (uint32_t y = 0; y < new_h; ++y) {
            for (uint32_t x = 0; x < w(); ++x) {
                result.get(x, y) = get(x, start_row + y);
            }
        }
        return result;
    }
    Matrix drop_columns(int n) const {
        uint32_t drop_count = std::abs(n);
        assert(drop_count < w());
        uint32_t new_w = w() - drop_count;
        Matrix result(new_w, h());
        uint32_t start_col = (n > 0) ? drop_count : 0;
        for (uint32_t y = 0; y < h(); ++y) {
            for (uint32_t x = 0; x < new_w; ++x) {
                result.get(x, y) = get(start_col + x, y);
            }
        }
        return result;
    }
    friend class MatrixView;
};

class MatrixView : public Matrix {
public:
    MatrixView(const Matrix& m) : Matrix() {
        this->arr = m.arr;
        this->sizes[0] = m.sizes[0];
        this->sizes[1] = m.sizes[1];
    }
    MatrixView(const MatrixView& m) : Matrix() {
        this->arr = m.arr;
        this->sizes[0] = m.sizes[0];
        this->sizes[1] = m.sizes[1];
    }
    MatrixView& operator=(const Matrix& m) {
        if (this == &m) {
            return *this;
        }
        this->arr = m.arr;
        this->sizes[0] = m.sizes[0];
        this->sizes[1] = m.sizes[1];
        return *this;
    }
    MatrixView& operator=(const MatrixView& m) {
        if (this == &m) {
            return *this;
        }
        this->arr = m.arr;
        this->sizes[0] = m.sizes[0];
        this->sizes[1] = m.sizes[1];
        return *this;
    }
    ~MatrixView() {
        this->arr = nullptr;
    }
};

namespace std {
    Matrix abs(const Matrix& a);
    Matrix pow(const Matrix& a, double e);
    Matrix pow(const Matrix& a, const Matrix& b);

    Matrix sin(const Matrix& a);
    Matrix cos(const Matrix& a);
    Matrix tan(const Matrix& a);
    Matrix asin(const Matrix& a);
    Matrix acos(const Matrix& a);
    Matrix atan(const Matrix& a);
    Matrix sinh(const Matrix& a);
    Matrix cosh(const Matrix& a);
    Matrix tanh(const Matrix& a);
    Matrix exp(const Matrix& a);
    Matrix log(const Matrix& a);
    Matrix log2(const Matrix& a);
    Matrix log10(const Matrix& a);
}

namespace funtalk {
    Matrix mulm(const Matrix& a, const Matrix& b);
    Matrix invm(const Matrix& a);
    Matrix powm(const Matrix& a, int e);

    Matrix sinm(const Matrix& a);
    Matrix cosm(const Matrix& a);
    Matrix tanm(const Matrix& a);
    Matrix asinm(const Matrix& a);
    Matrix acosm(const Matrix& a);
    Matrix atanm(const Matrix& a);
    Matrix sinhm(const Matrix& a);
    Matrix coshm(const Matrix& a);
    Matrix tanhm(const Matrix& a);
    Matrix expm(const Matrix& a);
    Matrix logm(const Matrix& a);
    Matrix log2m(const Matrix& a);
    Matrix log10m(const Matrix& a);
}

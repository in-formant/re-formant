#ifndef REFORMANT_PROCESSING_VECTOR2D_H
#define REFORMANT_PROCESSING_VECTOR2D_H

#include <stdexcept>
#include <vector>

template <typename T>
class vector2d {
   private:
    class const_row_view;
    class row_view;

   public:
    explicit vector2d(const int rows, const int cols, const T value = T{})
        : m_rows(rows), m_cols(cols), m_array(rows * cols, value) {}

    vector2d() : m_rows(0), m_cols(0) {}

    int rows() const { return m_rows; }

    int cols() const { return m_cols; }

    void resize(const int rows, const int cols) {
        m_rows = rows;
        m_cols = cols;
        m_array.resize(rows * cols);
    }

    void shrink_to_fit() { m_array.shrink_to_fit(); }

    void clear() {
        m_rows = m_cols = 0;
        m_array.clear();
    }

    const T& operator()(const int i, const int j) const {
        boundCheck(i, j);
        return at(i, j);
    }

    T& operator()(const int i, const int j) {
        boundCheck(i, j);
        return at(i, j);
    }

    const_row_view operator()(const int i) const {
        if (i < 0 || i >= m_rows) throw std::domain_error("Row index out of bounds");
        return const_row_view(*this, i);
    }

    const_row_view operator[](const int i) const {
        if (i < 0 || i >= m_rows) throw std::domain_error("Row index out of bounds");
        return const_row_view(*this, i);
    }

    row_view operator()(const int i) {
        if (i < 0 || i >= m_rows) throw std::domain_error("Row index out of bounds");
        return row_view(*this, i);
    }

    row_view operator[](const int i) {
        if (i < 0 || i >= m_rows) throw std::domain_error("Row index out of bounds");
        return row_view(*this, i);
    }

   private:
    inline void boundCheck(const int i, const int j) const {
        if (i < 0 || i >= m_rows) throw std::domain_error("Row index out of bounds");
        if (j < 0 || j >= m_cols) throw std::domain_error("Column index out of bounds");
    }

    inline int index(const int i, const int j) const { return i * m_cols + j; }

    inline T& at(const int i, const int j) { return m_array[index(i, j)]; }
    inline const T& at(const int i, const int j) const { return m_array[index(i, j)]; }

    int m_rows;
    int m_cols;
    std::vector<T> m_array;

    class const_row_view {
       public:
        const T& operator()(const int j) const {
            boundCheck(j);
            return m_ref.at(m_row, j);
        }

        const T& operator[](const int j) const {
            boundCheck(j);
            return m_ref.at(m_row, j);
        }

       private:
        const_row_view(const vector2d<T>& ref, const int row) : m_ref(ref), m_row(row) {}

        inline void boundCheck(const int j) {
            if (j < 0 || j >= m_cols)
                throw std::domain_error("Column index out of bounds");
        }

        const vector2d<T>& m_ref;
        const int m_row;

        friend class vector2d<T>;
    };

    class row_view {
       public:
        T& operator()(const int j) {
            m_ref.boundCheck(m_row, j);
            return m_ref.at(m_row, j);
        }

        T& operator[](const int j) {
            m_ref.boundCheck(m_row, j);
            return m_ref.at(m_row, j);
        }

       private:
        row_view(vector2d<T>& ref, const int row) : m_ref(ref), m_row(row) {}

        vector2d<T>& m_ref;
        const int m_row;

        friend class vector2d<T>;
    };

    friend class const_row_view;
    friend class row_view;
};

#endif  // REFORMANT_PROCESSING_VECTOR2D_H
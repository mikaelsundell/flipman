#ifndef UTIL_H
#define UTIL_H
#include <stdarg.h> // va_list
#include <vector>
#include <stddef.h> // size_t

namespace glsl {

// An implementation of std::find
template <typename I, typename T>
static inline I find(I first, I last, const T &value) {
    for (; first != last; ++first)
        if (*first == value)
            return first;
    return last;
}

// An implementation of vasprintf
int allocvfmt(char **str, const char *fmt, va_list vp);

// An implementation of vsprintf
int allocfmt(char **str, const char *fmt, ...);

// a tiny wrapper around std::vector so you can provide your own
template <typename T>
struct vector {
    size_t size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }
    const T& operator[](size_t index) const { return m_data[index]; }
    T& operator[](size_t index) { return m_data[index]; }
    // Flipman: never form pointers by subscripting an empty vector or its end.
    T* begin() { return m_data.data(); }
    T* end() { return pointer(size()); }
    const T* begin() const { return m_data.data(); }
    const T* end() const { return pointer(size()); }
    void insert(T *at, const T& value = T()) { m_data.insert(m_data.begin() + offset(at), value); }
    void insert(T *at, const T *beg, const T *end) { m_data.insert(m_data.begin() + offset(at), beg, end); }
    void push_back(const T &value) { m_data.push_back(value); }
    void reserve(size_t size) { m_data.reserve(size); }
    T* erase(T *position) {
        size_t index = offset(position);
        m_data.erase(m_data.begin() + index);
        return pointer(index);
    }
    T* erase(T *first, T *last) {
        size_t index = offset(first);
        m_data.erase(m_data.begin() + index, m_data.begin() + offset(last));
        return pointer(index);
    }
    void pop_back() { m_data.pop_back(); }
    T &front() { return m_data.front(); }
    const T &front() const { return m_data.front(); }
    T &back() { return m_data.back(); }
    const T& back() const { return m_data.back(); }
    void resize(size_t size) { m_data.resize(size); }
private:
    size_t offset(const T *position) const { return empty() ? 0 : size_t(position - m_data.data()); }
    T *pointer(size_t index) { return index ? m_data.data() + index : m_data.data(); }
    const T *pointer(size_t index) const { return index ? m_data.data() + index : m_data.data(); }
    std::vector<T> m_data;
};

}

#endif

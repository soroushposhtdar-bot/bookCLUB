#pragma once

#include <QString>

namespace bookclub::common {

template <typename T>
class Result {
public:
    Result() = default;
    static Result success(const T& value) { return Result(true, value, {}); }
    static Result failure(const QString& error) { return Result(false, T{}, error); }

    bool isSuccess() const { return m_success; }
    const T& value() const { return m_value; }
    const QString& errorMessage() const { return m_error; }

private:
    Result(bool success, const T& value, const QString& error)
        : m_success(success), m_value(value), m_error(error) {}

    bool m_success = false;
    T m_value{};
    QString m_error;
};

using VoidResult = Result<int>;

} // namespace bookclub::common

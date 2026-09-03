#pragma once
// Error policy: recoverable runtime errors -> Result<T>; programmer errors ->
// WO_ASSERT. Result stores std::variant<T, ErrorInfo>, so T may be
// non-default-constructible (M 17.5 closure). No exceptions in our code;
// STL allocation may still throw, which is a top-level crash policy matter
// (see 20_ERROR_HANDLING_LOGGING.md).

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace writeover {

struct ErrorInfo {
    uint32_t code = 0;
    std::string message;

    static ErrorInfo Make(uint32_t c, std::string msg) {
        ErrorInfo e;
        e.code = c;
        e.message = std::move(msg);
        return e;
    }
};

template <typename T>
class Result {
public:
    using Storage = std::variant<T, ErrorInfo>;

    static Result Ok(T value) { return Result(Storage(std::move(value))); }

    static Result Err(ErrorInfo err) { return Result(Storage(std::move(err))); }
    static Result Err(uint32_t code, std::string message) {
        return Result(Storage(ErrorInfo::Make(code, std::move(message))));
    }

    bool IsOk() const { return std::holds_alternative<T>(storage_); }
    bool IsError() const { return !IsOk(); }

    T& Value() { return std::get<T>(storage_); }
    const T& Value() const { return std::get<T>(storage_); }

    const ErrorInfo& Error() const { return std::get<ErrorInfo>(storage_); }

    T ValueOr(T fallback) const {
        if (IsOk()) {
            return Value();
        }
        return fallback;
    }

private:
    explicit Result(Storage s) : storage_(std::move(s)) {}
    Storage storage_;
};

template <>
class Result<void> {
public:
    static Result Ok() { return Result(); }
    static Result Err(ErrorInfo err) { return Result(std::move(err)); }
    static Result Err(uint32_t code, std::string message) {
        return Result(ErrorInfo::Make(code, std::move(message)));
    }

    bool IsOk() const { return !err_.has_value(); }
    bool IsError() const { return err_.has_value(); }
    const ErrorInfo& Error() const { return *err_; }

private:
    Result() = default;
    explicit Result(ErrorInfo e) : err_(std::move(e)) {}
    std::optional<ErrorInfo> err_;
};

} // namespace writeover
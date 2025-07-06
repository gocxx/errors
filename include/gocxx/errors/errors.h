#pragma once

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>
#include <utility>
#include <algorithm>

namespace gocxx::errors {

    // ---------- Error Interface ----------

    /**
     * @brief Interface for all error types in gocxx::errors.
     *
     * All custom error types should derive from this base class.
     */
    struct Error {
        /**
         * @brief Returns the error message as a string.
         * @return std::string The error message.
         */
        virtual std::string error() const noexcept = 0;

        /**
         * @brief Unwraps the inner error if present.
         * @return std::shared_ptr<Error> The wrapped error, or nullptr.
         */
        virtual std::shared_ptr<Error> Unwrap() const noexcept { return nullptr; }

        /**
         * @brief Compares the current error with another.
         * @param other The error to compare with.
         * @return true if both errors are considered equal (default: pointer equality).
         */
        virtual bool IsEqualTo(const std::shared_ptr<Error>& other) const noexcept {
            return this == other.get();
        }

        virtual ~Error() = default;
    };

    // ---------- simpleError ----------

    /**
     * @brief A simple implementation of Error for static string messages.
     */
    class simpleError : public Error {
        std::string msg;

    public:
        /**
         * @brief Constructs a simple error with a message.
         * @param message The error message.
         */
        explicit simpleError(std::string message) noexcept
            : msg(std::move(message)) {
        }

        /**
         * @brief Returns the error message.
         * @return std::string The stored message.
         */
        std::string error() const noexcept override { return msg; }
    };

    /**
     * @brief Creates a new simple error.
     *
     * @param msg The error message.
     * @return std::shared_ptr<Error> A shared pointer to the new error.
     */
    [[nodiscard]] inline std::shared_ptr<Error> New(const std::string& msg) {
        return std::make_shared<simpleError>(msg);
    }

    // ---------- wrappedError ----------

    /**
     * @brief An error that wraps another error with additional context.
     */
    class wrappedError : public Error {
        std::string msg;
        std::shared_ptr<Error> inner;

    public:
        /**
         * @brief Constructs a wrapped error.
         * @param message The context or outer error message.
         * @param err The inner error to wrap.
         */
        wrappedError(std::string message, std::shared_ptr<Error> err) noexcept
            : msg(std::move(message)), inner(std::move(err)) {
        }

        /**
         * @brief Returns the outer error message.
         * @return std::string The context message.
         */
        std::string error() const noexcept override { return msg; }

        /**
         * @brief Returns the wrapped inner error.
         * @return std::shared_ptr<Error> The inner error.
         */
        std::shared_ptr<Error> Unwrap() const noexcept override { return inner; }
    };

    /**
     * @brief Wraps an existing error with a message.
     *
     * @param msg The wrapping message.
     * @param err The error to wrap.
     * @return std::shared_ptr<Error> A new wrapped error.
     */
    [[nodiscard]] inline std::shared_ptr<Error> Wrap(const std::string& msg, const std::shared_ptr<Error>& err) {
        if (!err) return nullptr;
        return std::make_shared<wrappedError>(msg, err);
    }

    /**
     * @brief Unwraps the inner error from a single error.
     *
     * @param err The error to unwrap.
     * @return std::shared_ptr<Error> The inner error or nullptr.
     */
    [[nodiscard]] inline std::shared_ptr<Error> Unwrap(const std::shared_ptr<Error>& err) {
        return err ? err->Unwrap() : nullptr;
    }

    /**
     * @brief Checks if a target error exists in the chain of an error.
     *
     * @param err The top-level error to check.
     * @param target The target error to search for.
     * @return true If the target exists in the chain.
     * @return false Otherwise.
     */
    inline bool Is(const std::shared_ptr<Error>& err, const std::shared_ptr<Error>& target) noexcept {
        if (!err || !target) return false;

        auto current = err;
        while (current) {
            if (current->IsEqualTo(target)) return true;
            current = current->Unwrap();
        }
        return false;
    }

    /**
     * @brief Tries to cast an error or one of its inner errors to the specified type.
     *
     * @tparam T The desired error type.
     * @param err The error to search through.
     * @param out Reference to hold the matched error (if found).
     * @return true If a matching type was found.
     * @return false Otherwise.
     */
    template <typename T>
    bool As(const std::shared_ptr<Error>& err, std::shared_ptr<T>& out) noexcept {
        static_assert(std::is_base_of<Error, T>::value, "T must derive from Error");
        auto current = err;
        while (current) {
            if (auto casted = std::dynamic_pointer_cast<T>(current)) {
                out = casted;
                return true;
            }
            current = current->Unwrap();
        }
        return false;
    }

    // ---------- joinError ----------

    /**
     * @brief Represents multiple errors joined together.
     */
    class joinError : public Error {
        std::vector<std::shared_ptr<Error>> errs;
        std::string msg;

    public:
        /**
         * @brief Constructs a joinError from a list of errors.
         *
         * @param errors The list of errors to join.
         */
        explicit joinError(std::vector<std::shared_ptr<Error>> errors) noexcept
            : errs(std::move(errors)) {
            std::ostringstream oss;
            for (size_t i = 0; i < errs.size(); ++i) {
                if (errs[i]) {
                    if (i > 0) oss << "; ";
                    oss << errs[i]->error();
                }
            }
            msg = oss.str();
        }

        /**
         * @brief Returns the joined error message.
         * @return std::string The combined error string.
         */
        std::string error() const noexcept override { return msg; }

        /**
         * @brief Access the list of joined errors.
         * @return const std::vector<std::shared_ptr<Error>>& List of errors.
         */
        const std::vector<std::shared_ptr<Error>>& Errors() const noexcept { return errs; }

        /**
         * @brief Returns the first inner error to allow partial unwrapping.
         * @return std::shared_ptr<Error> The first error in the list.
         */
        std::shared_ptr<Error> Unwrap() const noexcept override {
            return errs.empty() ? nullptr : errs.front();
        }
    };

    /**
     * @brief Joins multiple errors into a single error.
     *
     * @param errs The vector of errors to join.
     * @return std::shared_ptr<Error> The resulting joined error or nullptr.
     */
    [[nodiscard]] inline std::shared_ptr<Error> Join(std::vector<std::shared_ptr<Error>> errs) {
        std::vector<std::shared_ptr<Error>> filtered;
        for (auto& e : errs) {
            if (e) filtered.push_back(e);
        }

        if (filtered.empty()) return nullptr;
        if (filtered.size() == 1) return filtered[0];
        return std::make_shared<joinError>(std::move(filtered));
    }

} // namespace gocxx::errors

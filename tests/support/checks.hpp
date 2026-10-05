#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ark::test {
// Release builds retain every assertion; each case owns its counter and diagnostic context.
class Checks {
  public:
    explicit Checks(std::string_view case_name) : case_name_(case_name) {}

    void operator()(bool passed, std::string_view message) {
        ++count_;
        if (!passed)
            throw std::runtime_error(case_name_ + ": " + std::string(message));
    }

    std::size_t count() const { return count_; }

  private:
    std::string case_name_;
    std::size_t count_{};
};
} // namespace ark::test

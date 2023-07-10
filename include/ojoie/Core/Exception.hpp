//
// Created by Aleudillonam on 7/26/2022.
//

#ifndef OJOIE_EXCEPTION_HPP
#define OJOIE_EXCEPTION_HPP

#include <stdexcept>
#include <format>
#ifdef __cpp_lib_source_location
#include <source_location>
#endif

namespace AN {

class Exception : public std::exception {


    std::string msg;

public:

#ifdef __cpp_lib_source_location
    Exception(const char *message, const std::source_location location = std::source_location::current()) noexcept {
        msg = std::format("{} \n[File] {} ({}:{}) \n[Function] {}", message, location.file_name(), location.line(), location.column(), location.function_name());
    }

    virtual char const* what() const {
        return msg.c_str();
    }

#else
    explicit Exception(const char *message) noexcept : msg(message) {}

    virtual char const* what() const {
        return msg.c_str();
    }
#endif

};

class Error {
    uint64_t _code;
    std::string _description;
public:

    Error() noexcept : _code() {}

    Error(uint64_t code, std::string description) noexcept : _code(code), _description(std::move(description)) {}

    uint64_t getCode() const noexcept {
        return _code;
    }
    const std::string &getDescription() const noexcept {
        return _description;
    }
};

}

#endif//OJOIE_EXCEPTION_HPP

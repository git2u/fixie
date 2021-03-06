#ifndef _FIXIE_LIB_EXCEPTIONS_HPP_
#define _FIXIE_LIB_EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

#include "fixie/fixie_gl_types.h"
#include "fixie/fixie_gl_es.h"

namespace fixie
{
    class fixie_error : public std::runtime_error
    {
    public:
        fixie_error(const std::string& msg);

        const std::string& error_msg() const;

    private:
        std::string _error_msg;
    };

    class context_error : public fixie_error
    {
    public:
        context_error(const std::string& msg);
    };

    class no_context_error : public context_error
    {
    public:
        no_context_error();
    };

    class state_error : public context_error
    {
    public:
        state_error(const std::string& msg);
    };

    class gl_error : public fixie_error
    {
    public:
        gl_error(GLenum error_code, const std::string& error_code_desc, const std::string& msg);

        GLenum error_code() const;
        const std::string& error_code_description() const;

    private:
        GLenum _error_code;
        std::string _error_code_description;
    };

    class invalid_enum_error : public gl_error
    {
    public:
        invalid_enum_error(const std::string& msg);
    };

    class invalid_value_error : public gl_error
    {
    public:
        invalid_value_error(const std::string& msg);
    };

    class invalid_operation_error : public gl_error
    {
    public:
        invalid_operation_error(const std::string& msg);
    };

    class stack_overflow_error : public gl_error
    {
    public:
        stack_overflow_error(const std::string& msg);
    };

    class stack_underflow_error : public gl_error
    {
    public:
        stack_underflow_error(const std::string& msg);
    };

    class out_of_memory_error : public gl_error
    {
    public:
        out_of_memory_error(const std::string& msg);
    };

    void throw_gl_error(GLenum error_code, const std::string& file, size_t line);
    void throw_gl_error(GLenum error_code, const std::string& msg);
}

#endif // _FIXIE_LIB_EXCEPTIONS_HPP_

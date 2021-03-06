#ifndef _FIXIE_LIB_BUFFER_HPP_
#define _FIXIE_LIB_BUFFER_HPP_

#include <memory>

#include "fixie/fixie_gl_types.h"
#include "fixie_lib/noncopyable.hpp"

namespace fixie
{
    class buffer_impl : public noncopyable
    {
    public:
        virtual void set_type(GLenum type) = 0;
        virtual void set_data(GLsizeiptr size, const GLvoid* data, GLenum usage) = 0;
        virtual void set_sub_data(GLintptr offset, GLsizeiptr size, const GLvoid* data) = 0;
    };

    class buffer : public noncopyable
    {
    public:
        explicit buffer(std::unique_ptr<buffer_impl> impl);

        GLenum type() const;
        GLsizei size() const;
        GLenum usage() const;

        std::weak_ptr<buffer_impl> impl();
        std::weak_ptr<const buffer_impl> impl() const;

        void bind(GLenum type);
        void set_data(GLsizeiptr size, const GLvoid* data, GLenum usage);
        void set_sub_data(GLintptr offset, GLsizeiptr size, const GLvoid* data);

    private:
        GLenum _type;
        GLsizei _size;
        GLenum _usage;
        std::shared_ptr<buffer_impl> _impl;
    };
}

#endif // _FIXIE_LIB_BUFFER_HPP

#ifndef _NULL_CONTEXT_HPP_
#define _NULL_CONTEXT_HPP_

#include "context.hpp"

namespace fixie
{
    namespace null_impl
    {
        class context : public fixie::context_impl
        {
        public:
            virtual const fixie::caps& caps() override;

            virtual std::shared_ptr<texture_impl> create_texture() override;
            virtual std::shared_ptr<buffer_impl> create_buffer() override;

            virtual void draw_arrays(const state& state, GLenum mode, GLint first, GLsizei count) override;
            virtual void draw_elements(const state& state, GLenum mode, GLsizei count, GLenum type, GLvoid* indices) override;

            virtual void clear(const state& state, GLbitfield mask) override;

        private:
            fixie::caps _caps;
        };
    }
}

#endif // _NULL_CONTEXT_HPP_

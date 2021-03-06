#include "fixie_lib/depth_buffer_state.hpp"

#include "fixie/fixie_gl_es.h"

namespace fixie
{
    depth_buffer_state::depth_buffer_state()
        : _depth_test_enabled()
        , _depth_func()
        , _depth_write_mask()
        , _clear_depth()
    {
    }

    const GLboolean& depth_buffer_state::depth_test_enabled() const
    {
        return _depth_test_enabled;
    }

    GLboolean& depth_buffer_state::depth_test_enabled()
    {
        return _depth_test_enabled;
    }

    const GLenum& depth_buffer_state::depth_func() const
    {
        return _depth_func;
    }

    GLenum& depth_buffer_state::depth_func()
    {
        return _depth_func;
    }

    const GLboolean& depth_buffer_state::depth_write_mask() const
    {
        return _depth_write_mask;
    }

    GLboolean& depth_buffer_state::depth_write_mask()
    {
        return _depth_write_mask;
    }

    GLclampf& depth_buffer_state::clear_depth()
    {
        return _clear_depth;
    }

    const GLclampf& depth_buffer_state::clear_depth() const
    {
        return _clear_depth;
    }

    fixie::depth_buffer_state default_depth_buffer_state()
    {
        depth_buffer_state state;
        state.depth_test_enabled() = GL_FALSE;
        state.depth_func() = GL_LESS;
        state.depth_write_mask() = GL_TRUE;
        state.clear_depth() = 1.0f;
        return state;
    }
}

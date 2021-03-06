#include "fixie_lib/desktop_gl_impl/context.hpp"
#include "fixie_lib/desktop_gl_impl/texture.hpp"
#include "fixie_lib/desktop_gl_impl/renderbuffer.hpp"
#include "fixie_lib/desktop_gl_impl/framebuffer.hpp"
#include "fixie_lib/desktop_gl_impl/buffer.hpp"
#include "fixie_lib/desktop_gl_impl/exceptions.hpp"
#include "fixie_lib/util.hpp"

#include "fixie/fixie_gl_es.h"

#include <assert.h>

namespace fixie
{
    namespace desktop_gl_impl
    {
        #define GL_FRAMEBUFFER 0x8D40

        void FIXIE_APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                           const GLchar* message, GLvoid* user_aram)
        {
            log_message(source, type, id, severity, std::string(message));
        }

        void enable_gl_state(std::shared_ptr<const gl_functions> funtions, GLenum cap, GLboolean enabled)
        {
            if (enabled)
            {
                gl_call(funtions, enable, cap);
            }
            else
            {
                gl_call(funtions, disable, cap);
            }
        }

        context::context()
            : _functions(std::make_shared<gl_functions>())
            , _version(initialize_version(_functions))
            , _extensions(intialize_extensions(_functions, _version))
            , _caps(initialize_caps(_functions, _version, _extensions))
            , _shader_cache(_functions)
            , _cur_viewport_state(default_viewport_state())
            , _cur_color_buffer_state(default_color_buffer_state())
            , _cur_depth_buffer_state(default_depth_buffer_state())
            , _cur_stencil_buffer_state(default_stencil_buffer_state())
            , _cur_point_state(default_point_state())
            , _cur_line_state(default_line_state())
            , _cur_polygon_state(default_polygon_state())
            , _cur_multisample_state(default_multisample_state())
            , _vao(0)
        {
            const GLubyte* gl_renderer_string = gl_call(_functions, get_string, GL_RENDERER);
            _renderer_string = format("%s OpenGL %s", reinterpret_cast<const char*>(gl_renderer_string), _version.str().c_str());

            if (_version >= gl_4_3 || _extensions.find("GL_KHR_debug") != end(_extensions))
            {
                gl_call(_functions, enable, GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
                gl_call(_functions, debug_message_control, GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM_KHR, 0, nullptr, GL_TRUE);
                gl_call(_functions, debug_message_callback, debug_callback, this);
            }

            if (_version >= gl_3_0 || _version >= gl_es_3_0 || _extensions.find("GL_ARB_vertex_array_object") != end(_extensions))
            {
                // need to generate a vao to use so 0 it not bound
                gl_call(_functions, gen_vertex_arrays, 1, &_vao);
                gl_call(_functions, bind_vertex_array, _vao);
            }
        }

        context::~context()
        {
            if (_vao != 0)
            {
                gl_call_nothrow(_functions, delete_vertex_arrays, 1, &_vao);
            }
        }

        const fixie::caps& context::caps()
        {
            return _caps;
        }

        const std::string& context::renderer_desc()
        {
            return _renderer_string;
        }

        void context::initialize_state(fixie::state& state)
        {
            GLint viewport_values[4];
            gl_call(_functions, get_integer_v, GL_VIEWPORT, viewport_values);
            state.viewport_state().viewport() = rectangle(viewport_values[0], viewport_values[1], viewport_values[2], viewport_values[3]);

            GLint scissor_values[4];
            gl_call(_functions, get_integer_v, GL_SCISSOR_BOX, scissor_values);
            state.scissor_state().scissor() = rectangle(scissor_values[0], scissor_values[1], scissor_values[2], scissor_values[3]);
        }

        std::unique_ptr<texture_impl> context::create_texture()
        {
            return std::unique_ptr<texture_impl>(new texture(_functions));
        }

        std::unique_ptr<renderbuffer_impl> context::create_renderbuffer()
        {
            return std::unique_ptr<renderbuffer_impl>(new renderbuffer(_functions));
        }

        std::unique_ptr<framebuffer_impl> context::create_default_framebuffer()
        {
            return std::unique_ptr<framebuffer_impl>(new default_framebuffer(_functions));
        }

        std::unique_ptr<framebuffer_impl> context::create_framebuffer()
        {
            return std::unique_ptr<framebuffer_impl>(new framebuffer(_functions));
        }

        std::unique_ptr<buffer_impl> context::create_buffer()
        {
            return std::unique_ptr<buffer_impl>(new buffer(_functions));
        }

        void context::draw_arrays(const state& state, GLenum mode, GLint first, GLsizei count)
        {
            sync_draw_state(state);

            gl_call(_functions, draw_arrays, mode, first, count);
        }

        void context::draw_elements(const state& state, GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
        {
            sync_draw_state(state);

            gl_call(_functions, draw_elements, mode, count, type, indices);
        }

        void context::clear(const state& state, GLbitfield mask)
        {
            sync_viewport_state(state.viewport_state());
            sync_scissor_state(state.scissor_state());
            sync_color_buffer_state(state.color_buffer_state());
            sync_depth_buffer_state(state.depth_buffer_state());
            sync_stencil_buffer_state(state.stencil_buffer_state());
            sync_framebuffer(state);
            gl_call(_functions, clear, mask);
        }

        void context::flush()
        {
            gl_call(_functions, flush);
        }

        void context::finish()
        {
            gl_call(_functions, finish);
        }

        void context::sync_viewport_state(const viewport_state& state)
        {
            if (_cur_viewport_state.viewport() != state.viewport())
            {
                gl_call(_functions, viewport, state.viewport().x(), state.viewport().y(), state.viewport().width(), state.viewport().height());
                _cur_viewport_state.viewport() = state.viewport();
            }

            if (_cur_viewport_state.depth_range() != state.depth_range())
            {
                gl_call(_functions, depth_range_f, state.depth_range().near(), state.depth_range().far());
                _cur_viewport_state.depth_range() = state.depth_range();
            }
        }

        void context::sync_scissor_state(const scissor_state& state)
        {
            if (_cur_scissor_state.scissor_test_enabled() != state.scissor_test_enabled())
            {
                enable_gl_state(_functions, GL_SCISSOR_TEST, state.scissor_test_enabled());
                _cur_scissor_state.scissor_test_enabled() = state.scissor_test_enabled();
            }

            if (_cur_scissor_state.scissor() != state.scissor())
            {
                gl_call(_functions, scissor, state.scissor().x(), state.scissor().y(), state.scissor().width(), state.scissor().height());
                _cur_scissor_state.scissor() = state.scissor();
            }
        }

        void context::sync_color_buffer_state(const color_buffer_state& state)
        {
            if (_cur_color_buffer_state.alpha_test_enabled() != state.alpha_test_enabled())
            {
                enable_gl_state(_functions, GL_ALPHA_TEST, state.alpha_test_enabled());
                _cur_color_buffer_state.alpha_test_enabled() = state.alpha_test_enabled();
            }

            if (_cur_color_buffer_state.alpha_test_func() != state.alpha_test_func() ||
                _cur_color_buffer_state.alpha_test_ref() != state.alpha_test_ref())
            {
                gl_call(_functions, alpha_func, state.alpha_test_func(), state.alpha_test_ref());
                _cur_color_buffer_state.alpha_test_func() = state.alpha_test_func();
                _cur_color_buffer_state.alpha_test_ref() = state.alpha_test_ref();
            }

            if (_cur_color_buffer_state.blend_enabled() != state.blend_enabled())
            {
                enable_gl_state(_functions, GL_BLEND, state.blend_enabled());
                _cur_color_buffer_state.blend_enabled() = state.blend_enabled();
            }

            if (_cur_color_buffer_state.blend_src_rgb_func() != state.blend_src_rgb_func() ||
                _cur_color_buffer_state.blend_dst_rgb_func() != state.blend_dst_rgb_func() ||
                _cur_color_buffer_state.blend_src_alpha_func() != state.blend_src_alpha_func() ||
                _cur_color_buffer_state.blend_dst_alpha_func() != state.blend_dst_alpha_func())
            {
                if (state.blend_src_rgb_func() == state.blend_src_alpha_func() &&
                    state.blend_dst_rgb_func() == state.blend_dst_alpha_func())
                {
                    gl_call(_functions, blend_func, state.blend_src_rgb_func(), state.blend_dst_rgb_func());
                }
                else
                {
                    gl_call(_functions, blend_func_seperate, state.blend_src_rgb_func(), state.blend_dst_rgb_func(),
                                                             state.blend_src_alpha_func(), state.blend_dst_alpha_func());
                }
                _cur_color_buffer_state.blend_src_rgb_func() = state.blend_src_rgb_func();
                _cur_color_buffer_state.blend_dst_rgb_func() = state.blend_dst_rgb_func();
                _cur_color_buffer_state.blend_src_alpha_func() = state.blend_src_alpha_func();
                _cur_color_buffer_state.blend_dst_alpha_func() = state.blend_dst_alpha_func();
            }

            if (_cur_color_buffer_state.dither_enabled() != state.dither_enabled())
            {
                enable_gl_state(_functions, GL_DITHER, state.dither_enabled());
                _cur_color_buffer_state.dither_enabled() = state.dither_enabled();
            }

            if (_cur_color_buffer_state.color_logic_op_enabled() != state.color_logic_op_enabled())
            {
                enable_gl_state(_functions, GL_COLOR_LOGIC_OP, state.color_logic_op_enabled());
                _cur_color_buffer_state.color_logic_op_enabled() = state.color_logic_op_enabled();
            }

            if (_cur_color_buffer_state.color_logic_op_func() != state.color_logic_op_func())
            {
                gl_call(_functions, logic_op, state.color_logic_op_func());
                _cur_color_buffer_state.color_logic_op_func() = state.color_logic_op_func();
            }

            if (_cur_color_buffer_state.clear_color() != state.clear_color())
            {
                gl_call(_functions, clear_color, state.clear_color().r(), state.clear_color().g(), state.clear_color().b(), state.clear_color().a());
                _cur_color_buffer_state.clear_color() = state.clear_color();
            }
        }

        void context::sync_depth_buffer_state(const depth_buffer_state& state)
        {
            if (_cur_depth_buffer_state.depth_test_enabled() != state.depth_test_enabled())
            {
                enable_gl_state(_functions, GL_DEPTH_TEST, state.depth_test_enabled());
                _cur_depth_buffer_state.depth_test_enabled() = state.depth_test_enabled();
            }

            if (_cur_depth_buffer_state.depth_func() != state.depth_func())
            {
                gl_call(_functions, depth_func, state.depth_func());
                _cur_depth_buffer_state.depth_func() = state.depth_func();
            }

            if (_cur_depth_buffer_state.depth_write_mask() != state.depth_write_mask())
            {
                gl_call(_functions, depth_mask, state.depth_write_mask());
                _cur_depth_buffer_state.depth_write_mask() = state.depth_write_mask();
            }

            if (_cur_depth_buffer_state.clear_depth() != state.clear_depth())
            {
                gl_call(_functions, clear_depthf, state.clear_depth());
                _cur_depth_buffer_state.clear_depth() = state.clear_depth();
            }
        }

        void context::sync_stencil_buffer_state(const stencil_buffer_state& state)
        {
            if (_cur_stencil_buffer_state.stencil_test_enabled() != state.stencil_test_enabled())
            {
                enable_gl_state(_functions, GL_STENCIL_TEST, state.stencil_test_enabled());
                _cur_stencil_buffer_state.stencil_test_enabled() = state.stencil_test_enabled();
            }

            if (_cur_stencil_buffer_state.stencil_func() != state.stencil_func() ||
                _cur_stencil_buffer_state.stencil_ref() != state.stencil_ref() ||
                _cur_stencil_buffer_state.stencil_read_mask() != state.stencil_read_mask())
            {
                gl_call(_functions, stencil_func, state.stencil_func(), state.stencil_ref(), state.stencil_read_mask());
                _cur_stencil_buffer_state.stencil_func() = state.stencil_func();
                _cur_stencil_buffer_state.stencil_ref() = state.stencil_ref();
                _cur_stencil_buffer_state.stencil_read_mask() = state.stencil_read_mask();
            }

            if (_cur_stencil_buffer_state.stencil_fail_operation() != state.stencil_fail_operation() ||
                _cur_stencil_buffer_state.stencil_pass_depth_fail_operation() != state.stencil_pass_depth_fail_operation() ||
                _cur_stencil_buffer_state.stencil_pass_depth_pass_operation() != state.stencil_pass_depth_pass_operation())
            {
                gl_call(_functions, stencil_op, state.stencil_fail_operation(), state.stencil_pass_depth_fail_operation(),
                                                state.stencil_pass_depth_pass_operation());
                _cur_stencil_buffer_state.stencil_fail_operation() = state.stencil_fail_operation();
                _cur_stencil_buffer_state.stencil_pass_depth_fail_operation() = state.stencil_pass_depth_fail_operation();
                _cur_stencil_buffer_state.stencil_pass_depth_pass_operation() = state.stencil_pass_depth_pass_operation();
            }

            if (_cur_stencil_buffer_state.stencil_write_mask() != state.stencil_write_mask())
            {
                gl_call(_functions, stencil_mask, state.stencil_write_mask());
                _cur_stencil_buffer_state.stencil_write_mask() = state.stencil_write_mask();
            }

            if (_cur_stencil_buffer_state.clear_stencil() != state.clear_stencil())
            {
                gl_call(_functions, clear_stencil, state.clear_stencil());
                _cur_stencil_buffer_state.clear_stencil() = state.clear_stencil();
            }
        }

        void context::sync_point_state(const point_state& state)
        {
        }

        void context::sync_line_state(const line_state& state)
        {
        }

        void context::sync_polygon_state(const polygon_state& state)
        {
        }

        void context::sync_multisample_state(const multisample_state& state)
        {
            if (_cur_multisample_state.multisample_enabled() != state.multisample_enabled())
            {
                enable_gl_state(_functions, GL_MULTISAMPLE, state.multisample_enabled());
                _cur_multisample_state.multisample_enabled() = state.multisample_enabled();
            }

            if (_cur_multisample_state.sample_to_alpha_coverage_enabled() != state.sample_to_alpha_coverage_enabled())
            {
                enable_gl_state(_functions, GL_SAMPLE_ALPHA_TO_COVERAGE, state.sample_to_alpha_coverage_enabled());
                _cur_multisample_state.sample_to_alpha_coverage_enabled() = state.sample_to_alpha_coverage_enabled();
            }

            if (_cur_multisample_state.sample_alpha_to_one_enabled() != state.sample_alpha_to_one_enabled())
            {
                enable_gl_state(_functions, GL_SAMPLE_ALPHA_TO_ONE, state.sample_alpha_to_one_enabled());
                _cur_multisample_state.sample_alpha_to_one_enabled() = state.sample_alpha_to_one_enabled();
            }

            if (_cur_multisample_state.sample_coverage_enabled() != state.sample_coverage_enabled())
            {
                enable_gl_state(_functions, GL_SAMPLE_COVERAGE, state.sample_coverage_enabled());
                _cur_multisample_state.sample_coverage_enabled() = state.sample_coverage_enabled();
            }


            if (_cur_multisample_state.sample_coverage_value() != state.sample_coverage_value() ||
                _cur_multisample_state.sample_coverage_invert() != state.sample_coverage_invert())
            {
                gl_call(_functions, sample_coverage, state.sample_coverage_value(), state.sample_coverage_invert());
                _cur_multisample_state.sample_coverage_value() = state.sample_coverage_value();
                _cur_multisample_state.sample_coverage_invert() = state.sample_coverage_invert();
            }
        }

        void context::sync_vertex_attribute(const vertex_attribute& attribute, GLint location, GLboolean normalized)
        {
            if (location != -1)
            {
                auto cur_attribute = _cur_vertex_attributes.find(location);
                if (cur_attribute == end(_cur_vertex_attributes) || cur_attribute->second != attribute)
                {
                    if (attribute.attribute_enabled())
                    {
                        std::shared_ptr<const fixie::buffer> bound_buffer = attribute.buffer().lock();
                        std::shared_ptr<const fixie::buffer_impl> bound_buffer_impl = (bound_buffer != nullptr) ? bound_buffer->impl().lock() : nullptr;
                        std::shared_ptr<const buffer> desktop_buffer = std::dynamic_pointer_cast<const buffer>(bound_buffer_impl);
                        GLuint buffer_id = desktop_buffer ? desktop_buffer->id() : 0;

                        gl_call(_functions, bind_buffer, GL_ARRAY_BUFFER, buffer_id);
                        gl_call(_functions, enable_vertex_attrib_array, location);
                        gl_call(_functions, vertex_attrib_pointer, location, attribute.size(), attribute.type(), normalized, attribute.stride(), attribute.pointer());
                    }
                    else
                    {
                        const vector4& generic_values = attribute.generic_values();

                        gl_call(_functions, bind_buffer, GL_ARRAY_BUFFER, 0);
                        gl_call(_functions, disable_vertex_attrib_array, location);
                        gl_call(_functions, vertex_attrib_4f, location, generic_values.x(), generic_values.y(), generic_values.z(), generic_values.w());
                    }

                    _cur_vertex_attributes[location] = attribute;
                }
            }
        }

        void context::sync_vertex_attributes(std::weak_ptr<const fixie::vertex_array> vertex_array, std::weak_ptr<const shader> shader)
        {
            std::shared_ptr<const desktop_gl_impl::shader> locked_shader = shader.lock();
            assert(locked_shader != nullptr);

            std::shared_ptr<const fixie::vertex_array> locked_vertex_array = vertex_array.lock();
            assert(locked_vertex_array != nullptr);

            sync_vertex_attribute(locked_vertex_array->vertex_attribute(), locked_shader->vertex_attribute_location(), GL_FALSE);
            sync_vertex_attribute(locked_vertex_array->color_attribute(), locked_shader->color_attribute_location(), GL_TRUE);
            sync_vertex_attribute(locked_vertex_array->normal_attribute(), locked_shader->normal_attribute_location(), GL_TRUE);
            for_each_n(0, _caps.max_texture_units(), [&](size_t i) { sync_vertex_attribute(locked_vertex_array->texcoord_attribute(i), locked_shader->texcoord_attribute_location(i), GL_TRUE); });
        }

        void context::sync_texture(std::weak_ptr<const fixie::texture> texture, size_t index)
        {
            std::shared_ptr<const fixie::texture> locked_texture = texture.lock();
            std::shared_ptr<const fixie::texture_impl> texture_impl = (locked_texture != nullptr) ? locked_texture->impl().lock() : nullptr;
            std::shared_ptr<const desktop_gl_impl::texture> desktop_texture = std::dynamic_pointer_cast<const desktop_gl_impl::texture>(texture_impl);
            GLuint texuture_id = desktop_texture ? desktop_texture->id() : 0;

            gl_call(_functions, active_texture, static_cast<GLenum>(GL_TEXTURE0 + index));
            gl_call(_functions, bind_texture, GL_TEXTURE_2D, texuture_id);
            if (locked_texture)
            {
                gl_call(_functions, enable, GL_TEXTURE_2D);
                gl_call(_functions, tex_parameter_i, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, locked_texture->sampler_state().wrap_s());
                gl_call(_functions, tex_parameter_i, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, locked_texture->sampler_state().wrap_t());
                gl_call(_functions, tex_parameter_i, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, locked_texture->sampler_state().min_filter());
                gl_call(_functions, tex_parameter_i, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, locked_texture->sampler_state().mag_filter());
            }
            else
            {
                gl_call(_functions, disable, GL_TEXTURE_2D);
            }
        }

        void context::sync_textures(const state& state)
        {
            for_each_n(0, _caps.max_texture_units(), [&](size_t i){ if (state.texture_environment(i).texture_enabled()) { sync_texture(state.bound_texture(i), i); }});
        }

        void context::sync_framebuffer(const state& state)
        {
            std::shared_ptr<const fixie::framebuffer> bound_framebuffer = state.bound_framebuffer().lock();
            std::shared_ptr<const fixie::framebuffer_impl> bound_framebuffer_impl = bound_framebuffer ? bound_framebuffer->impl().lock() : nullptr;
            std::shared_ptr<const framebuffer> desktop_framebuffer = std::dynamic_pointer_cast<const framebuffer>(bound_framebuffer_impl);
            GLuint framebuffer_id = desktop_framebuffer ? desktop_framebuffer->id() : 0;

            gl_call(_functions, bind_framebuffer, GL_FRAMEBUFFER, framebuffer_id);
        }

        void context::sync_draw_state(const state& state)
        {
            std::shared_ptr<shader> shader = _shader_cache.get_shader(state, _caps).lock();
            shader->sync_state(state);
            sync_vertex_attributes(state.bound_vertex_array(), shader);
            sync_textures(state);
            sync_framebuffer(state);
            sync_viewport_state(state.viewport_state());
            sync_scissor_state(state.scissor_state());
            sync_color_buffer_state(state.color_buffer_state());
            sync_depth_buffer_state(state.depth_buffer_state());
            sync_stencil_buffer_state(state.stencil_buffer_state());
            sync_point_state(state.point_state());
            sync_line_state(state.line_state());
            sync_polygon_state(state.polygon_state());
        }

        gl_version context::initialize_version(std::shared_ptr<const gl_functions> functions)
        {
            const GLubyte* gl_version_string = gl_call(functions, get_string, GL_VERSION);
            return gl_version(std::string(reinterpret_cast<const char*>(gl_version_string)));
        }

        std::unordered_set<std::string> context::intialize_extensions(std::shared_ptr<const gl_functions> functions, const gl_version& version)
        {
            std::unordered_set<std::string> extensions;

            if (version >= gl_3_0 || version >= gl_es_3_0)
            {
                #define GL_NUM_EXTENSIONS 0x821D

                GLint num_extensions;
                gl_call(functions, get_integer_v, GL_NUM_EXTENSIONS, &num_extensions);
                for (GLint i = 0; i < num_extensions; i++)
                {
                    const GLubyte* extension_name = gl_call(functions, get_string_i, GL_EXTENSIONS, i);
                    extensions.insert(reinterpret_cast<const char*>(extension_name));
                }
            }
            else
            {
                const GLubyte* gl_extension_string = gl_call(functions, get_string, GL_EXTENSIONS);
                split(reinterpret_cast<const char*>(gl_extension_string), ' ', std::inserter(extensions, extensions.end()));
            }

            return extensions;
        }

        fixie::caps context::initialize_caps(std::shared_ptr<const gl_functions> functions, const gl_version& version, const std::unordered_set<std::string>& extensions)
        {
            fixie::caps caps;

            caps.max_lights() = 8;
            caps.max_clip_planes() = 6;
            caps.max_model_view_stack_depth() = 1024;
            caps.max_projection_stack_depth() = 1024;
            caps.max_texture_stack_depth() = 1024;
            gl_call(functions, get_integer_v, GL_SUBPIXEL_BITS, &caps.subpixel_bits());
            gl_call(functions, get_integer_v, GL_MAX_TEXTURE_SIZE, &caps.max_texture_size());

            GLint max_viewport_dims[2];
            gl_call(functions, get_integer_v, GL_MAX_VIEWPORT_DIMS, max_viewport_dims);
            caps.max_viewport_width() = max_viewport_dims[0];
            caps.max_viewport_height() = max_viewport_dims[1];

            #define GL_POINT_SIZE_RANGE 0x0B12
            GLfloat point_point_size_range_values[2];
            gl_call(functions, get_float_v, GL_POINT_SIZE_RANGE, point_point_size_range_values);
            caps.aliased_point_size_range() = range(point_point_size_range_values[0], point_point_size_range_values[1]);
            caps.smooth_point_size_range() = range(point_point_size_range_values[0], point_point_size_range_values[1]);

            GLfloat aliased_line_width_range_values[2];
            gl_call(functions, get_float_v, GL_ALIASED_LINE_WIDTH_RANGE, aliased_line_width_range_values);
            caps.aliased_line_width_range() = range(aliased_line_width_range_values[0], aliased_line_width_range_values[1]);

            GLfloat smooth_line_width_range_values[2];
            gl_call(functions, get_float_v, GL_SMOOTH_LINE_WIDTH_RANGE, smooth_line_width_range_values);
            caps.smooth_line_width_range() = range(smooth_line_width_range_values[0], smooth_line_width_range_values[1]);

            gl_call(functions, get_integer_v, GL_MAX_TEXTURE_UNITS, &caps.max_texture_units());
            gl_call(functions, get_integer_v, GL_SAMPLE_BUFFERS, &caps.sample_buffers());
            gl_call(functions, get_integer_v, GL_SAMPLES, &caps.samples());

            GLint compressed_format_count;
            gl_call(functions, get_integer_v, GL_NUM_COMPRESSED_TEXTURE_FORMATS, &compressed_format_count);

            std::vector<GLint> compressed_formats(compressed_format_count);
            gl_call(functions, get_integer_v, GL_COMPRESSED_TEXTURE_FORMATS, compressed_formats.data());
            for (GLint i = 0; i < compressed_format_count; ++i)
            {
                caps.insert_compressed_format(compressed_formats[i]);
            }

            caps.supports_vertex_array_objects() = version >= gl_3_0 || version >= gl_es_3_0 || extensions.find("GL_ARB_vertex_array_object") != end(extensions);

            if (version >= gl_3_0 || version >= gl_es_3_0 || extensions.find("GL_EXT_framebuffer_object") != end(extensions))
            {
                #define GL_FRONT_LEFT 0x0400
                #define GL_MAX_RENDERBUFFER_SIZE 0x84E8
                #define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
                #define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
                #define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
                #define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
                #define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
                #define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
                #define GL_DEPTH 0x1801
                #define GL_STENCIL 0x1802

                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &caps.red_bits());
                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &caps.green_bits());
                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &caps.blue_bits());
                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &caps.alpha_bits());
                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &caps.depth_bits());
                gl_call(functions, get_framebuffer_attachment_parameter_iv, GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &caps.stencil_bits());

                caps.supports_framebuffer_objects() = GL_TRUE;
                gl_call(functions, get_integer_v, GL_MAX_RENDERBUFFER_SIZE, &caps.max_renderbuffer_size());
                caps.supports_rgb8_rgba8() = (version >= gl_3_0 || extensions.find("GL_OES_rgb8_rgba8") != end(extensions)) ? GL_TRUE : GL_FALSE;
                caps.supports_depth24() = (version >= gl_3_0 || extensions.find("GL_OES_depth24") != end(extensions)) ? GL_TRUE : GL_FALSE;
                caps.supports_depth32() = (version >= gl_3_0 || extensions.find("GL_OES_depth32") != end(extensions)) ? GL_TRUE : GL_FALSE;
                caps.supports_stencil1() = (version >= gl_3_0 || extensions.find("GL_OES_stencil1") != end(extensions)) ? GL_TRUE : GL_FALSE;
                caps.supports_stencil4() = (version >= gl_3_0 || extensions.find("GL_OES_stencil4") != end(extensions)) ? GL_TRUE : GL_FALSE;
                caps.supports_stencil8() = (version >= gl_3_0 || extensions.find("GL_OES_stencil8") != end(extensions)) ? GL_TRUE : GL_FALSE;
            }
            else
            {
                gl_call(functions, get_integer_v, GL_RED_BITS, &caps.red_bits());
                gl_call(functions, get_integer_v, GL_GREEN_BITS, &caps.green_bits());
                gl_call(functions, get_integer_v, GL_BLUE_BITS, &caps.blue_bits());
                gl_call(functions, get_integer_v, GL_ALPHA_BITS, &caps.alpha_bits());
                gl_call(functions, get_integer_v, GL_DEPTH_BITS, &caps.depth_bits());
                gl_call(functions, get_integer_v, GL_STENCIL_BITS, &caps.stencil_bits());

                caps.supports_framebuffer_objects() = GL_FALSE;
                caps.max_renderbuffer_size() = 0;
                caps.supports_rgb8_rgba8() = GL_FALSE;
                caps.supports_depth24() = GL_FALSE;
                caps.supports_depth32() = GL_FALSE;
                caps.supports_stencil1() = GL_FALSE;
                caps.supports_stencil4() = GL_FALSE;
                caps.supports_stencil8() = GL_FALSE;
            }

            return caps;
        }
    }
}

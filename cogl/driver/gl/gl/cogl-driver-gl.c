/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2007,2008,2009 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "cogl-private.h"
#include "cogl-context-private.h"
#include "cogl-util-gl-private.h"
#include "cogl-feature-private.h"
#include "cogl-renderer-private.h"
#include "cogl-error-private.h"
#include "cogl-framebuffer-gl-private.h"
#include "cogl-texture-2d-gl-private.h"
#include "cogl-attribute-gl-private.h"
#include "cogl-clip-stack-gl-private.h"
#include "cogl-buffer-gl-private.h"

static CoglBool
_cogl_driver_pixel_format_from_gl_internal (CoglContext *context,
                                            GLenum gl_int_format,
                                            CoglPixelFormat *out_format)
{
  /* It doesn't really matter we convert to exact same
     format (some have no cogl match anyway) since format
     is re-matched against cogl when getting or setting
     texture image data.
  */

  switch (gl_int_format)
    {
    case GL_ALPHA: case GL_ALPHA4: case GL_ALPHA8:
    case GL_ALPHA12: case GL_ALPHA16:
      /* Cogl only supports one single-component texture so if we have
       * ended up with a red texture then it is probably being used as
       * a component-alpha texture */
    case GL_RED:

      *out_format = COGL_PIXEL_FORMAT_A_8;
      return TRUE;

    case GL_LUMINANCE: case GL_LUMINANCE4: case GL_LUMINANCE8:
    case GL_LUMINANCE12: case GL_LUMINANCE16:

      *out_format = COGL_PIXEL_FORMAT_G_8;
      return TRUE;

    case GL_RGB: case GL_RGB4: case GL_RGB5: case GL_RGB8:
    case GL_RGB10: case GL_RGB12: case GL_RGB16: case GL_R3_G3_B2:

      *out_format = COGL_PIXEL_FORMAT_RGB_888;
      return TRUE;

    case GL_RGBA: case GL_RGBA2: case GL_RGBA4: case GL_RGB5_A1:
    case GL_RGBA8: case GL_RGB10_A2: case GL_RGBA12: case GL_RGBA16:

      *out_format = COGL_PIXEL_FORMAT_RGBA_8888;
      return TRUE;
    }

  return FALSE;
}

static CoglPixelFormat
_cogl_driver_pixel_format_to_gl (CoglContext *context,
                                 CoglPixelFormat  format,
                                 GLenum *out_glintformat,
                                 GLenum *out_glformat,
                                 GLenum *out_gltype)
{
  CoglPixelFormat required_format;
  GLenum glintformat;
  GLenum glformat = 0;
  GLenum gltype;

  required_format = format;

  /* Find GL equivalents */
  switch (format)
    {
    case COGL_PIXEL_FORMAT_A_8:
      /* If the driver doesn't natively support alpha textures then we
       * will use a red component texture with a swizzle to implement
       * the texture */
      if ((context->private_feature_flags &
           COGL_PRIVATE_FEATURE_ALPHA_TEXTURES) == 0)
        {
          glintformat = GL_RED;
          glformat = GL_RED;
        }
      else
        {
          glintformat = GL_ALPHA;
          glformat = GL_ALPHA;
        }
      gltype = GL_UNSIGNED_BYTE;
      break;
    case COGL_PIXEL_FORMAT_G_8:
      glintformat = GL_LUMINANCE;
      glformat = GL_LUMINANCE;
      gltype = GL_UNSIGNED_BYTE;
      break;

    case COGL_PIXEL_FORMAT_RGB_888:
      glintformat = GL_RGB;
      glformat = GL_RGB;
      gltype = GL_UNSIGNED_BYTE;
      break;
    case COGL_PIXEL_FORMAT_BGR_888:
      glintformat = GL_RGB;
      glformat = GL_BGR;
      gltype = GL_UNSIGNED_BYTE;
      break;
    case COGL_PIXEL_FORMAT_RGBA_8888:
    case COGL_PIXEL_FORMAT_RGBA_8888_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
      gltype = GL_UNSIGNED_BYTE;
      break;
    case COGL_PIXEL_FORMAT_BGRA_8888:
    case COGL_PIXEL_FORMAT_BGRA_8888_PRE:
      glintformat = GL_RGBA;
      glformat = GL_BGRA;
      gltype = GL_UNSIGNED_BYTE;
      break;

      /* The following two types of channel ordering
       * have no GL equivalent unless defined using
       * system word byte ordering */
    case COGL_PIXEL_FORMAT_ARGB_8888:
    case COGL_PIXEL_FORMAT_ARGB_8888_PRE:
      glintformat = GL_RGBA;
      glformat = GL_BGRA;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      gltype = GL_UNSIGNED_INT_8_8_8_8;
#else
      gltype = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      break;

    case COGL_PIXEL_FORMAT_ABGR_8888:
    case COGL_PIXEL_FORMAT_ABGR_8888_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      gltype = GL_UNSIGNED_INT_8_8_8_8;
#else
      gltype = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      break;

    case COGL_PIXEL_FORMAT_RGBA_1010102:
    case COGL_PIXEL_FORMAT_RGBA_1010102_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
      gltype = GL_UNSIGNED_INT_10_10_10_2;
      break;

    case COGL_PIXEL_FORMAT_BGRA_1010102:
    case COGL_PIXEL_FORMAT_BGRA_1010102_PRE:
      glintformat = GL_RGBA;
      glformat = GL_BGRA;
      gltype = GL_UNSIGNED_INT_10_10_10_2;
      break;

    case COGL_PIXEL_FORMAT_ABGR_2101010:
    case COGL_PIXEL_FORMAT_ABGR_2101010_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
      gltype = GL_UNSIGNED_INT_2_10_10_10_REV;
      break;

    case COGL_PIXEL_FORMAT_ARGB_2101010:
    case COGL_PIXEL_FORMAT_ARGB_2101010_PRE:
      glintformat = GL_RGBA;
      glformat = GL_BGRA;
      gltype = GL_UNSIGNED_INT_2_10_10_10_REV;
      break;

      /* The following three types of channel ordering
       * are always defined using system word byte
       * ordering (even according to GLES spec) */
    case COGL_PIXEL_FORMAT_RGB_565:
      glintformat = GL_RGB;
      glformat = GL_RGB;
      gltype = GL_UNSIGNED_SHORT_5_6_5;
      break;
    case COGL_PIXEL_FORMAT_RGBA_4444:
    case COGL_PIXEL_FORMAT_RGBA_4444_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
      gltype = GL_UNSIGNED_SHORT_4_4_4_4;
      break;
    case COGL_PIXEL_FORMAT_RGBA_5551:
    case COGL_PIXEL_FORMAT_RGBA_5551_PRE:
      glintformat = GL_RGBA;
      glformat = GL_RGBA;
      gltype = GL_UNSIGNED_SHORT_5_5_5_1;
      break;

    case COGL_PIXEL_FORMAT_DEPTH_16:
      glintformat = GL_DEPTH_COMPONENT16;
      glformat = GL_DEPTH_COMPONENT;
      gltype = GL_UNSIGNED_SHORT;
      break;
    case COGL_PIXEL_FORMAT_DEPTH_32:
      glintformat = GL_DEPTH_COMPONENT32;
      glformat = GL_DEPTH_COMPONENT;
      gltype = GL_UNSIGNED_INT;
      break;

    case COGL_PIXEL_FORMAT_DEPTH_24_STENCIL_8:
      glintformat = GL_DEPTH_STENCIL;
      glformat = GL_DEPTH_STENCIL;
      gltype = GL_UNSIGNED_INT_24_8;
      break;

    case COGL_PIXEL_FORMAT_ANY:
    case COGL_PIXEL_FORMAT_YUV:
      g_assert_not_reached ();
      break;
    }

  /* All of the pixel formats are handled above so if this hits then
     we've been given an invalid pixel format */
  g_assert (glformat != 0);

  if (out_glintformat != NULL)
    *out_glintformat = glintformat;
  if (out_glformat != NULL)
    *out_glformat = glformat;
  if (out_gltype != NULL)
    *out_gltype = gltype;

  return required_format;
}

static CoglBool
parse_gl_version (const char *version_string,
                  int *major_out,
                  int *minor_out)
{
  const char *major_end, *minor_end;
  int major = 0, minor = 0;

  /* Extract the major number */
  for (major_end = version_string; *major_end >= '0'
	 && *major_end <= '9'; major_end++)
    major = (major * 10) + *major_end - '0';
  /* If there were no digits or the major number isn't followed by a
     dot then it is invalid */
  if (major_end == version_string || *major_end != '.')
    return FALSE;

  /* Extract the minor number */
  for (minor_end = major_end + 1; *minor_end >= '0'
	 && *minor_end <= '9'; minor_end++)
    minor = (minor * 10) + *minor_end - '0';
  /* If there were no digits or there is an unexpected character then
     it is invalid */
  if (minor_end == major_end + 1
      || (*minor_end && *minor_end != ' ' && *minor_end != '.'))
    return FALSE;

  *major_out = major;
  *minor_out = minor;

  return TRUE;
}

static CoglBool
_cogl_get_gl_version (CoglContext *ctx,
                      int *major_out,
                      int *minor_out)
{
  const char *version_string;

  /* Get the OpenGL version number */
  if ((version_string = _cogl_context_get_gl_version (ctx)) == NULL)
    return FALSE;

  return parse_gl_version (version_string, major_out, minor_out);
}

static CoglBool
check_gl_version (CoglContext *ctx,
                  char **gl_extensions,
                  CoglError **error)
{
  int major, minor;

  if (!_cogl_get_gl_version (ctx, &major, &minor))
    {
      _cogl_set_error (error,
                   COGL_DRIVER_ERROR,
                   COGL_DRIVER_ERROR_UNKNOWN_VERSION,
                   "The OpenGL version could not be determined");
      return FALSE;
    }

  /* GL 1.3 supports all of the required functionality in core */
  if (COGL_CHECK_GL_VERSION (major, minor, 1, 3))
    return TRUE;

  /* OpenGL 1.2 is only supported if we have the multitexturing
     extension */
  if (!_cogl_check_extension ("GL_ARB_multitexture", gl_extensions))
    {
      _cogl_set_error (error,
                   COGL_DRIVER_ERROR,
                   COGL_DRIVER_ERROR_INVALID_VERSION,
                   "The OpenGL driver is missing "
                   "the GL_ARB_multitexture extension");
      return FALSE;
    }

  /* OpenGL 1.2 is required */
  if (!COGL_CHECK_GL_VERSION (major, minor, 1, 2))
    {
      _cogl_set_error (error,
                   COGL_DRIVER_ERROR,
                   COGL_DRIVER_ERROR_INVALID_VERSION,
                   "The OpenGL version of your driver (%i.%i) "
                   "is not compatible with Cogl",
                   major, minor);
      return FALSE;
    }

  return TRUE;
}

static CoglBool
_cogl_driver_update_features (CoglContext *ctx,
                              CoglError **error)
{
  CoglPrivateFeatureFlags private_flags = 0;
  CoglFeatureFlags flags = 0;
  char **gl_extensions;
  int gl_major = 0, gl_minor = 0;

  /* We have to special case getting the pointer to the glGetString*
     functions because we need to use them to determine what functions
     we can expect */
  ctx->glGetString =
    (void *) _cogl_renderer_get_proc_address (ctx->display->renderer,
                                              "glGetString",
                                              TRUE);
  ctx->glGetStringi =
    (void *) _cogl_renderer_get_proc_address (ctx->display->renderer,
                                              "glGetStringi",
                                              TRUE);
  ctx->glGetIntegerv =
    (void *) _cogl_renderer_get_proc_address (ctx->display->renderer,
                                              "glGetIntegerv",
                                              TRUE);

  gl_extensions = _cogl_context_get_gl_extensions (ctx);

  if (!check_gl_version (ctx, gl_extensions, error))
    return FALSE;

  if (G_UNLIKELY (COGL_DEBUG_ENABLED (COGL_DEBUG_WINSYS)))
    {
      char *all_extensions = g_strjoinv (" ", gl_extensions);

      COGL_NOTE (WINSYS,
                 "Checking features\n"
                 "  GL_VENDOR: %s\n"
                 "  GL_RENDERER: %s\n"
                 "  GL_VERSION: %s\n"
                 "  GL_EXTENSIONS: %s",
                 ctx->glGetString (GL_VENDOR),
                 ctx->glGetString (GL_RENDERER),
                 _cogl_context_get_gl_version (ctx),
                 all_extensions);

      g_free (all_extensions);
    }

  _cogl_get_gl_version (ctx, &gl_major, &gl_minor);

  _cogl_gpu_info_init (ctx, &ctx->gpu);

  ctx->glsl_major = 1;
  ctx->glsl_minor = 1;

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 2, 0))
    {
      const char *glsl_version =
        (char *)ctx->glGetString (GL_SHADING_LANGUAGE_VERSION);
      parse_gl_version (glsl_version, &ctx->glsl_major, &ctx->glsl_minor);
    }

  flags = (COGL_FEATURE_TEXTURE_READ_PIXELS
           | COGL_FEATURE_UNSIGNED_INT_INDICES
           | COGL_FEATURE_DEPTH_RANGE);
  COGL_FLAGS_SET (ctx->features,
                  COGL_FEATURE_ID_UNSIGNED_INT_INDICES, TRUE);
  COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_DEPTH_RANGE, TRUE);

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 1, 4))
    COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_MIRRORED_REPEAT, TRUE);

  _cogl_feature_check_ext_functions (ctx,
                                     gl_major,
                                     gl_minor,
                                     gl_extensions);

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 2, 0) ||
      _cogl_check_extension ("GL_ARB_texture_non_power_of_two", gl_extensions))
    {
      flags |= COGL_FEATURE_TEXTURE_NPOT
        | COGL_FEATURE_TEXTURE_NPOT_BASIC
        | COGL_FEATURE_TEXTURE_NPOT_MIPMAP
        | COGL_FEATURE_TEXTURE_NPOT_REPEAT;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_TEXTURE_NPOT, TRUE);
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_TEXTURE_NPOT_BASIC, TRUE);
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_TEXTURE_NPOT_MIPMAP, TRUE);
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_TEXTURE_NPOT_REPEAT, TRUE);
    }

  if (_cogl_check_extension ("GL_MESA_pack_invert", gl_extensions))
    private_flags |= COGL_PRIVATE_FEATURE_MESA_PACK_INVERT;

  if (ctx->glGenRenderbuffers)
    {
      flags |= COGL_FEATURE_OFFSCREEN;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_OFFSCREEN, TRUE);
      private_flags |= COGL_PRIVATE_FEATURE_QUERY_FRAMEBUFFER_BITS;
    }

  if (ctx->glBlitFramebuffer)
    private_flags |= COGL_PRIVATE_FEATURE_OFFSCREEN_BLIT;

  if (ctx->glRenderbufferStorageMultisampleIMG)
    {
      flags |= COGL_FEATURE_OFFSCREEN_MULTISAMPLE;
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_OFFSCREEN_MULTISAMPLE, TRUE);
    }

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 3, 0) ||
      _cogl_check_extension ("GL_ARB_depth_texture", gl_extensions))
    {
      flags |= COGL_FEATURE_DEPTH_TEXTURE;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_DEPTH_TEXTURE, TRUE);
    }

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 2, 1) ||
      _cogl_check_extension ("GL_EXT_pixel_buffer_object", gl_extensions))
    private_flags |= COGL_PRIVATE_FEATURE_PBOS;

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 1, 4) ||
      _cogl_check_extension ("GL_EXT_blend_color", gl_extensions))
    private_flags |= COGL_PRIVATE_FEATURE_BLEND_CONSTANT;

  if (ctx->glGenPrograms)
    {
      flags |= COGL_FEATURE_SHADERS_ARBFP;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_ARBFP, TRUE);
    }

  if (ctx->glCreateProgram)
    {
      flags |= COGL_FEATURE_SHADERS_GLSL;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_GLSL, TRUE);
    }
  else
    {
      /* If all of the old GLSL extensions are available then we can fake
       * the GL 2.0 GLSL support by diverting to the old function names */
      if (ctx->glCreateProgramObject && /* GL_ARB_shader_objects */
           ctx->glVertexAttribPointer && /* GL_ARB_vertex_shader */
           _cogl_check_extension ("GL_ARB_fragment_shader", gl_extensions))
        {
          ctx->glCreateShader = ctx->glCreateShaderObject;
          ctx->glCreateProgram = ctx->glCreateProgramObject;
          ctx->glDeleteShader = ctx->glDeleteObject;
          ctx->glDeleteProgram = ctx->glDeleteObject;
          ctx->glAttachShader = ctx->glAttachObject;
          ctx->glUseProgram = ctx->glUseProgramObject;
          ctx->glGetProgramInfoLog = ctx->glGetInfoLog;
          ctx->glGetShaderInfoLog = ctx->glGetInfoLog;
          ctx->glGetShaderiv = ctx->glGetObjectParameteriv;
          ctx->glGetProgramiv = ctx->glGetObjectParameteriv;
          ctx->glDetachShader = ctx->glDetachObject;
          ctx->glGetAttachedShaders = ctx->glGetAttachedObjects;
          /* FIXME: there doesn't seem to be an equivalent for glIsShader
           * and glIsProgram. This doesn't matter for now because Cogl
           * doesn't use these but if we add support for simulating a
           * GLES2 context on top of regular GL then we'll need to do
           * something here */

          flags |= COGL_FEATURE_SHADERS_GLSL;
          COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_GLSL, TRUE);
        }
    }

  if ((COGL_CHECK_GL_VERSION (gl_major, gl_minor, 2, 0) ||
       _cogl_check_extension ("GL_ARB_point_sprite", gl_extensions)) &&

      /* If GLSL is supported then we only enable point sprite support
       * too if we have glsl >= 1.2 otherwise we don't have the
       * gl_PointCoord builtin which we depend on in the glsl backend.
       */
      (!COGL_FLAGS_GET (ctx->features, COGL_FEATURE_ID_GLSL) ||
       COGL_CHECK_GL_VERSION (ctx->glsl_major, ctx->glsl_minor, 1, 2)))
    {
      flags |= COGL_FEATURE_POINT_SPRITE;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_POINT_SPRITE, TRUE);
    }

  if (ctx->glGenBuffers)
    {
      private_flags |= COGL_PRIVATE_FEATURE_VBOS;
      flags |= (COGL_FEATURE_MAP_BUFFER_FOR_READ |
                COGL_FEATURE_MAP_BUFFER_FOR_WRITE);
      COGL_FLAGS_SET (ctx->features,
                         COGL_FEATURE_ID_MAP_BUFFER_FOR_READ, TRUE);
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_MAP_BUFFER_FOR_WRITE, TRUE);
    }

  if (_cogl_check_extension ("GL_ARB_texture_rectangle", gl_extensions))
    {
      flags |= COGL_FEATURE_TEXTURE_RECTANGLE;
      COGL_FLAGS_SET (ctx->features,
                      COGL_FEATURE_ID_TEXTURE_RECTANGLE, TRUE);
    }

  if (ctx->glTexImage3D)
    {
      flags |= COGL_FEATURE_TEXTURE_3D;
      COGL_FLAGS_SET (ctx->features, COGL_FEATURE_ID_TEXTURE_3D, TRUE);
    }

  if (ctx->glEGLImageTargetTexture2D)
    private_flags |= COGL_PRIVATE_FEATURE_TEXTURE_2D_FROM_EGL_IMAGE;

  if (_cogl_check_extension ("GL_EXT_packed_depth_stencil", gl_extensions))
    private_flags |= COGL_PRIVATE_FEATURE_EXT_PACKED_DEPTH_STENCIL;

  if (ctx->glGenSamplers)
    private_flags |= COGL_PRIVATE_FEATURE_SAMPLER_OBJECTS;

  if (COGL_CHECK_GL_VERSION (gl_major, gl_minor, 3, 3) ||
      _cogl_check_extension ("GL_ARB_texture_swizzle", gl_extensions) ||
      _cogl_check_extension ("GL_EXT_texture_swizzle", gl_extensions))
    private_flags |= COGL_PRIVATE_FEATURE_TEXTURE_SWIZZLE;

  if (ctx->driver == COGL_DRIVER_GL)
    {
      int max_clip_planes = 0;

      /* Features which are not available in GL 3 */
      private_flags |= (COGL_PRIVATE_FEATURE_FIXED_FUNCTION |
                        COGL_PRIVATE_FEATURE_ALPHA_TEST |
                        COGL_PRIVATE_FEATURE_QUADS |
                        COGL_PRIVATE_FEATURE_ALPHA_TEXTURES);

      GE( ctx, glGetIntegerv (GL_MAX_CLIP_PLANES, &max_clip_planes) );
      if (max_clip_planes >= 4)
        private_flags |= COGL_PRIVATE_FEATURE_FOUR_CLIP_PLANES;
    }

  private_flags |= (COGL_PRIVATE_FEATURE_READ_PIXELS_ANY_FORMAT |
                    COGL_PRIVATE_FEATURE_ANY_GL |
                    COGL_PRIVATE_FEATURE_FORMAT_CONVERSION |
                    COGL_PRIVATE_FEATURE_BLEND_CONSTANT |
                    COGL_PRIVATE_FEATURE_BUILTIN_POINT_SIZE_UNIFORM |
                    COGL_PRIVATE_FEATURE_QUERY_TEXTURE_PARAMETERS |
                    COGL_PRIVATE_FEATURE_TEXTURE_MAX_LEVEL);

  /* Cache features */
  ctx->private_feature_flags |= private_flags;
  ctx->feature_flags |= flags;

  g_strfreev (gl_extensions);

  if ((private_flags & (COGL_PRIVATE_FEATURE_ALPHA_TEXTURES |
                        COGL_PRIVATE_FEATURE_TEXTURE_SWIZZLE)) == 0)
    {
      _cogl_set_error (error,
                       COGL_DRIVER_ERROR,
                       COGL_DRIVER_ERROR_NO_SUITABLE_DRIVER_FOUND,
                       "The GL_ARB_texture_swizzle extension is required "
                       "to use the GL3 driver");
      return FALSE;
    }

  return TRUE;
}

const CoglDriverVtable
_cogl_driver_gl =
  {
    _cogl_driver_pixel_format_from_gl_internal,
    _cogl_driver_pixel_format_to_gl,
    _cogl_driver_update_features,
    _cogl_offscreen_gl_allocate,
    _cogl_offscreen_gl_free,
    _cogl_framebuffer_gl_flush_state,
    _cogl_framebuffer_gl_clear,
    _cogl_framebuffer_gl_query_bits,
    _cogl_framebuffer_gl_finish,
    _cogl_framebuffer_gl_discard_buffers,
    _cogl_framebuffer_gl_draw_attributes,
    _cogl_framebuffer_gl_draw_indexed_attributes,
    _cogl_framebuffer_gl_read_pixels_into_bitmap,
    _cogl_texture_2d_gl_free,
    _cogl_texture_2d_gl_can_create,
    _cogl_texture_2d_gl_init,
    _cogl_texture_2d_gl_allocate,
    _cogl_texture_2d_gl_new_from_bitmap,
#if defined (COGL_HAS_EGL_SUPPORT) && defined (EGL_KHR_image_base)
    _cogl_egl_texture_2d_gl_new_from_image,
#endif
    _cogl_texture_2d_gl_copy_from_framebuffer,
    _cogl_texture_2d_gl_get_gl_handle,
    _cogl_texture_2d_gl_generate_mipmap,
    _cogl_texture_2d_gl_copy_from_bitmap,
    _cogl_texture_2d_gl_get_data,
    _cogl_gl_flush_attributes_state,
    _cogl_clip_stack_gl_flush,
    _cogl_buffer_gl_create,
    _cogl_buffer_gl_destroy,
    _cogl_buffer_gl_map_range,
    _cogl_buffer_gl_unmap,
    _cogl_buffer_gl_set_data,
  };

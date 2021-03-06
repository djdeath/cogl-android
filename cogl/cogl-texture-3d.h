/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2010 Intel Corporation.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Neil Roberts <neil@linux.intel.com>
 */

#if !defined(__COGL_H_INSIDE__) && !defined(COGL_COMPILATION)
#error "Only <cogl/cogl.h> can be included directly."
#endif

#ifndef __COGL_TEXTURE_3D_H
#define __COGL_TEXTURE_3D_H

COGL_BEGIN_DECLS

/**
 * SECTION:cogl-texture-3d
 * @short_description: Functions for creating and manipulating 3D textures
 *
 * These functions allow 3D textures to be used. 3D textures can be
 * thought of as layers of 2D images arranged into a cuboid
 * shape. When choosing a texel from the texture, Cogl will take into
 * account the 'r' texture coordinate to select one of the images.
 */

typedef struct _CoglTexture3D CoglTexture3D;

#define COGL_TEXTURE_3D(X) ((CoglTexture3D *)X)

/**
 * cogl_texture_3d_new_with_size:
 * @context: a #CoglContext
 * @width: width of the texture in pixels.
 * @height: height of the texture in pixels.
 * @depth: depth of the texture in pixels.
 * @internal_format: the #CoglPixelFormat to use for the GPU
 *    storage of the texture.
 *
 * Creates a new #CoglTexture3D texture with the specified dimensions
 * and pixel format.
 *
 * The storage for the texture is not allocated before this function
 * returns. You can call cogl_texture_allocate() to explicitly
 * allocate the underlying storage or preferably let Cogl
 * automatically allocate storage lazily when it may know more about
 * how the texture is going to be used and can optimize how it is
 * allocated.
 *
 * <note>This texture will fail to allocate later if
 * %COGL_FEATURE_ID_TEXTURE_3D is not advertised. Allocation can also
 * fail if the requested dimensions are not supported by the
 * GPU.</note>
 *
 * Returns: A new #CoglTexture3D object with no storage yet allocated.
 * Since: 1.10
 * Stability: Unstable
 */
CoglTexture3D *
cogl_texture_3d_new_with_size (CoglContext *context,
                               int width,
                               int height,
                               int depth,
                               CoglPixelFormat internal_format);

/**
 * cogl_texture_3d_new_from_data:
 * @context: a #CoglContext
 * @width: width of the texture in pixels.
 * @height: height of the texture in pixels.
 * @depth: depth of the texture in pixels.
 * @format: the #CoglPixelFormat the buffer is stored in in RAM
 * @internal_format: the #CoglPixelFormat that will be used for storing
 *    the buffer on the GPU. If %COGL_PIXEL_FORMAT_ANY is given then a
 *    premultiplied format similar to the format of the source data will
 *    be used. The default blending equations of Cogl expect premultiplied
 *    color data; the main use of passing a non-premultiplied format here
 *    is if you have non-premultiplied source data and are going to adjust
 *    the blend mode (see cogl_pipeline_set_blend()) or use the data for
 *    something other than straight blending.
 * @rowstride: the memory offset in bytes between the starts of
 *    scanlines in @data or 0 to infer it from the width and format
 * @image_stride: the number of bytes from one image to the next. This
 *    can be used to add padding between the images in a similar way
 *    that the rowstride can be used to add padding between
 *    rows. Alternatively 0 can be passed to infer the @image_stride
 *    from the @height.
 * @data: pointer the memory region where the source buffer resides
 * @error: A CoglError return location.
 *
 * Creates a new 3D texture and initializes it with @data. The data is
 * assumed to be packed array of @depth images. There can be padding
 * between the images using @image_stride.
 *
 * Note that this function will throw a #CoglError if
 * %COGL_FEATURE_ID_TEXTURE_3D is not advertised. It can also fail if the
 * requested dimensions are not supported by the GPU.
 *
 * Return value: the newly created #CoglTexture3D or %NULL if
 *               there was an error an an exception will be returned
 *               through @error.
 * Since: 1.10
 * Stability: Unstable
 */
CoglTexture3D *
cogl_texture_3d_new_from_data (CoglContext *context,
                               int width,
                               int height,
                               int depth,
                               CoglPixelFormat format,
                               CoglPixelFormat internal_format,
                               int rowstride,
                               int image_stride,
                               const uint8_t *data,
                               CoglError **error);

/**
 * cogl_texture_3d_new_from_bitmap:
 * @bitmap: A #CoglBitmap object.
 * @height: height of the texture in pixels.
 * @depth: depth of the texture in pixels.
 * @internal_format: the #CoglPixelFormat that will be used for storing
 *    the buffer on the GPU. If %COGL_PIXEL_FORMAT_ANY is given then a
 *    premultiplied format similar to the format of the source data will
 *    be used. The default blending equations of Cogl expect premultiplied
 *    color data; the main use of passing a non-premultiplied format here
 *    is if you have non-premultiplied source data and are going to adjust
 *    the blend mode (see cogl_pipeline_set_blend()) or use the data for
 *    something other than straight blending.
 * @error: A CoglError return location.
 *
 * Creates a new 3D texture and initializes it with the images in
 * @bitmap. The images are assumed to be packed together after one
 * another in the increasing y axis. The height of individual image is
 * given as @height and the number of images is given in @depth. The
 * actual height of the bitmap can be larger than @height × @depth. In
 * this case it assumes there is padding between the images.
 *
 * Return value: the newly created texture or %NULL if
 *   there was an error.
 * Since: 2.0
 * Stability: unstable
 */
CoglTexture3D *
cogl_texture_3d_new_from_bitmap (CoglBitmap *bitmap,
                                 unsigned int height,
                                 unsigned int depth,
                                 CoglPixelFormat internal_format,
                                 CoglError **error);

/**
 * cogl_is_texture_3d:
 * @object: a #CoglObject
 *
 * Checks whether the given object references a #CoglTexture3D
 *
 * Return value: %TRUE if the passed object represents a 3D texture
 *   and %FALSE otherwise
 *
 * Since: 1.4
 * Stability: Unstable
 */
CoglBool
cogl_is_texture_3d (void *object);

COGL_END_DECLS

#endif /* __COGL_TEXTURE_3D_H */

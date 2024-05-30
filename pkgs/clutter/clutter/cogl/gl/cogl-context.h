/*
 * Clutter COGL
 *
 * A basic GL/GLES Abstraction/Utility Layer
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2007 OpenedHand
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
 */

#ifndef __COGL_CONTEXT_H
#define __COGL_CONTEXT_H

#include "cogl-primitives.h"

typedef struct
{
  /* Features cache */
  CoglFeatureFlags  feature_flags;
  gboolean          features_cached;
  GLint             num_stencil_bits;
  
  /* Enable cache */
  gulong            enable_flags;
  guint8            color_alpha;
  COGLenum          blend_src_factor;
  COGLenum          blend_dst_factor;
  
  /* Primitives */
  CoglFixedVec2     path_start;
  CoglFixedVec2     path_pen;
  CoglFloatVec2    *path_nodes;
  guint             path_nodes_cap;
  guint             path_nodes_size;
  CoglFixedVec2     path_nodes_min;
  CoglFixedVec2     path_nodes_max;

  /* Cache of inverse projection matrix */
  GLfloat           inverse_projection[16];
  
  /* Textures */
  GArray           *texture_handles;
  
  /* Framebuffer objects */
  GArray           *fbo_handles;
  CoglBufferTarget  draw_buffer;

  /* Shaders */
  GArray           *shader_handles;

  /* Programs */
  GArray           *program_handles;
  
  /* Relying on glext.h to define these */
  COGL_PFNGLGENRENDERBUFFERSEXTPROC                pf_glGenRenderbuffersEXT;
  COGL_PFNGLBINDRENDERBUFFEREXTPROC                pf_glBindRenderbufferEXT;
  COGL_PFNGLRENDERBUFFERSTORAGEEXTPROC             pf_glRenderbufferStorageEXT;
  COGL_PFNGLGENFRAMEBUFFERSEXTPROC                 pf_glGenFramebuffersEXT;
  COGL_PFNGLBINDFRAMEBUFFEREXTPROC                 pf_glBindFramebufferEXT;
  COGL_PFNGLFRAMEBUFFERTEXTURE2DEXTPROC            pf_glFramebufferTexture2DEXT;
  COGL_PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC         pf_glFramebufferRenderbufferEXT;
  COGL_PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC          pf_glCheckFramebufferStatusEXT;
  COGL_PFNGLDELETEFRAMEBUFFERSEXTPROC              pf_glDeleteFramebuffersEXT;
  COGL_PFNGLBLITFRAMEBUFFEREXTPROC                 pf_glBlitFramebufferEXT;
  COGL_PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC  pf_glRenderbufferStorageMultisampleEXT;
  
  COGL_PFNGLCREATEPROGRAMOBJECTARBPROC             pf_glCreateProgramObjectARB;
  COGL_PFNGLCREATESHADEROBJECTARBPROC              pf_glCreateShaderObjectARB;
  COGL_PFNGLSHADERSOURCEARBPROC                    pf_glShaderSourceARB;
  COGL_PFNGLCOMPILESHADERARBPROC                   pf_glCompileShaderARB;
  COGL_PFNGLATTACHOBJECTARBPROC                    pf_glAttachObjectARB;
  COGL_PFNGLLINKPROGRAMARBPROC                     pf_glLinkProgramARB;
  COGL_PFNGLUSEPROGRAMOBJECTARBPROC                pf_glUseProgramObjectARB;
  COGL_PFNGLGETUNIFORMLOCATIONARBPROC              pf_glGetUniformLocationARB;
  COGL_PFNGLDELETEOBJECTARBPROC                    pf_glDeleteObjectARB;
  COGL_PFNGLGETINFOLOGARBPROC                      pf_glGetInfoLogARB;
  COGL_PFNGLGETOBJECTPARAMETERIVARBPROC            pf_glGetObjectParameterivARB;
  COGL_PFNGLUNIFORM1FARBPROC                       pf_glUniform1fARB;
  
} CoglContext;

CoglContext *
_cogl_context_get_default ();

/* Obtains the context and returns retval if NULL */
#define _COGL_GET_CONTEXT(ctxvar, retval) \
CoglContext *ctxvar = _cogl_context_get_default (); \
if (ctxvar == NULL) return retval;

#define NO_RETVAL 

#endif /* __COGL_CONTEXT_H */

/*
 * Clutter COGL
 *
 * A basic GL/GLES Abstraction/Utility Layer
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2008 OpenedHand
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

  ;
const char cogl_fixed_fragment_shader_start[] =
  "\n"
  "/* There is no default precision for floats in fragment shaders in\n"
  "   GLES 2 so we need to define one */\n"
  "precision mediump float;\n"
  "\n"
  "/* Inputs from the vertex shader */\n"
  "varying vec4       frag_color;\n"
  "varying vec2       tex_coord;\n"
  "varying float      fog_amount;\n"
  "\n"
  "/* Texturing options */\n"
  "uniform sampler2D  texture_unit;\n"
  "\n"
  "/* Fogging options */\n"
  "uniform vec4       fog_color;\n"
  "\n"
  "/* Alpha test options */\n"
  "uniform float      alpha_test_ref;\n"
  "\n"
  "void\n"
  "main (void)\n"
  "{\n"
  ;
const char cogl_fixed_fragment_shader_texture_alpha_only[] =
  "\n"
  "  /* If the texture only has an alpha channel (eg, with the textures\n"
  "     from the pango renderer) then the RGB components will be\n"
  "     black. We want to use the RGB from the current color in that\n"
  "     case */\n"
  "  gl_FragColor = frag_color;\n"
  "  gl_FragColor.a *= texture2D (texture_unit, tex_coord).a;\n"
  "\n"
  ;
const char cogl_fixed_fragment_shader_texture[] =
  "\n"
  "  /* This pointless extra variable is needed to work around an\n"
  "     apparent bug in the PowerVR drivers. Without it the alpha\n"
  "     blending seems to stop working */\n"
  "  vec4 frag_color_copy = frag_color;\n"
  "  gl_FragColor = frag_color_copy * texture2D (texture_unit, tex_coord);\n"
  "\n"
  ;
const char cogl_fixed_fragment_shader_solid_color[] =
  "  gl_FragColor = frag_color;\n"
  "\n"
  ;
const char cogl_fixed_fragment_shader_fog[] =
  "\n"
  "  /* Mix the calculated color with the fog color */\n"
  "  gl_FragColor.rgb = mix (fog_color.rgb, gl_FragColor.rgb, fog_amount);\n"
  "\n"
  "  /* Alpha testing */\n"
  "\n"
  ;
const char cogl_fixed_fragment_shader_alpha_never[] =
  "  discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_less[] =
  "  if (gl_FragColor.a >= alpha_test_ref)\n"
  "    discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_equal[] =
  "  if (gl_FragColor.a != alpha_test_ref)\n"
  "    discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_lequal[] =
  "  if (gl_FragColor.a > alpha_test_ref)\n"
  "    discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_greater[] =
  "  if (gl_FragColor.a <= alpha_test_ref)\n"
  "    discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_notequal[] =
  "  if (gl_FragColor.a == alpha_test_ref)\n"
  "    discard;\n"
  ;
const char cogl_fixed_fragment_shader_alpha_gequal[] =
  "  if (gl_FragColor.a < alpha_test_ref)\n"
  "    discard;\n"
  "\n"
  ;
const char cogl_fixed_fragment_shader_end[] =
  "}\n"
  ;

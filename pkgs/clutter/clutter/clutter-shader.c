/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *             Øyvind Kolås   <pippin@o-hand.com>
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

/**
 * SECTION:clutter-shader
 * @short_description: Programmable pipeline abstraction
 *
 * #ClutterShader is an object providing an abstraction over the
 * OpenGL programmable pipeline. By using #ClutterShader<!-- -->s is
 * possible to override the drawing pipeline by using small programs
 * also known as "shaders".
 *
 * #ClutterShader is available since Clutter 0.6
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#include "cogl/cogl.h"

#include "clutter-debug.h"
#include "clutter-private.h"
#include "clutter-shader.h"

static GList *clutter_shaders_list = NULL;

#define CLUTTER_SHADER_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj),  \
   CLUTTER_TYPE_SHADER, ClutterShaderPrivate))

typedef enum {
  CLUTTER_VERTEX_SHADER,
  CLUTTER_FRAGMENT_SHADER
} ClutterShaderType;

struct _ClutterShaderPrivate
{
  guint       compiled         : 1; /* Shader is bound to the GL context */
  guint       is_enabled       : 1;

  guint       vertex_is_glsl   : 1;
  guint       fragment_is_glsl : 1;

  gchar      *vertex_source;        /* GLSL source for vertex shader */
  gchar      *fragment_source;      /* GLSL source for fragment shader */

  CoglHandle  program;

  CoglHandle  vertex_shader;
  CoglHandle  fragment_shader;
};

enum 
{
  PROP_0,

  PROP_VERTEX_SOURCE,
  PROP_FRAGMENT_SOURCE,
  PROP_COMPILED,
  PROP_ENABLED
};

G_DEFINE_TYPE (ClutterShader, clutter_shader, G_TYPE_OBJECT);

G_CONST_RETURN gchar *clutter_shader_get_source (ClutterShader      *shader,
                                                 ClutterShaderType   type);

static void
clutter_shader_finalize (GObject *object)
{
  ClutterShader        *shader;
  ClutterShaderPrivate *priv;

  shader = CLUTTER_SHADER (object);
  priv   = shader->priv;

  clutter_shader_release (shader);

  clutter_shaders_list = g_list_remove (clutter_shaders_list, object);

  g_free (priv->fragment_source);
  g_free (priv->vertex_source);

  G_OBJECT_CLASS (clutter_shader_parent_class)->finalize (object);
}

static void
clutter_shader_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClutterShader *shader = CLUTTER_SHADER(object);

  switch (prop_id)
    {
    case PROP_VERTEX_SOURCE:
      clutter_shader_set_vertex_source (shader, 
					g_value_get_string (value), -1);
      break;
    case PROP_FRAGMENT_SOURCE:
      clutter_shader_set_fragment_source (shader, 
					  g_value_get_string (value), -1);
      break;
    case PROP_ENABLED:
      clutter_shader_set_is_enabled (shader, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_shader_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClutterShader        *shader;
  ClutterShaderPrivate *priv;

  shader = CLUTTER_SHADER(object);
  priv = shader->priv;

  switch (prop_id)
    {
    case PROP_VERTEX_SOURCE:
      g_value_set_string (value, priv->vertex_source);
      break;
    case PROP_FRAGMENT_SOURCE:
      g_value_set_string (value, priv->fragment_source);
      break;
    case PROP_COMPILED:
      g_value_set_boolean (value, priv->compiled);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, priv->is_enabled);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GObject *
clutter_shader_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObjectClass *parent_class;
  GObject *object;

  parent_class = G_OBJECT_CLASS (clutter_shader_parent_class);
  object = parent_class->constructor (type, n_params, params);

  /* add this instance to the global list of shaders */
  clutter_shaders_list = g_list_prepend (clutter_shaders_list, object);

  return object;
}


static void
clutter_shader_class_init (ClutterShaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize      = clutter_shader_finalize;
  object_class->set_property  = clutter_shader_set_property;
  object_class->get_property  = clutter_shader_get_property;
  object_class->constructor   = clutter_shader_constructor;

  g_type_class_add_private (klass, sizeof (ClutterShaderPrivate));

  /**
   * ClutterShader:vertex-source:
   *
   * GLSL source code for the vertex shader part of the shader program, if any.
   *
   * Since: 0.6
   */
  g_object_class_install_property 
    (object_class,
     PROP_VERTEX_SOURCE,
     g_param_spec_string ("vertex-source",
			  "Vertex Source",
			  "Source of vertex shader",
			  NULL,
			  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterShader:fragment-source:
   *
   * GLSL source code for the fragment shader part of the shader program.
   *
   * Since: 0.6
   */
  g_object_class_install_property 
    (object_class,
     PROP_FRAGMENT_SOURCE,
     g_param_spec_string ("fragment-source",
			  "Fragment Source",
			  "Source of fragment shader",
			  NULL,
			  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterShader:compiled:
   *
   * Whether the shader is compiled and linked, ready for use
   * in the GL context.
   *
   * Since: 0.8
   */
  g_object_class_install_property 
    (object_class,
     PROP_COMPILED,
     g_param_spec_boolean ("compiled",
			   "Compiled",
			   "Whether the shader is compiled and linked",
			   FALSE,
			   CLUTTER_PARAM_READABLE));
  /**
   * ClutterShader:enabled:
   *
   * Whether the shader is currently used in the GL rendering pipeline.
   *
   * Since: 0.6
   */
  g_object_class_install_property 
    (object_class,
     PROP_ENABLED,
     g_param_spec_boolean ("enabled",
			   "Enabled",
			   "Whether the shader is enabled",
			   FALSE,
			   CLUTTER_PARAM_READWRITE));
}

static void
clutter_shader_init (ClutterShader *self)
{
  ClutterShaderPrivate *priv;

  priv = self->priv = CLUTTER_SHADER_GET_PRIVATE (self);

  priv->compiled = FALSE;

  priv->vertex_source = NULL;
  priv->fragment_source = NULL;

  priv->program = COGL_INVALID_HANDLE;
  priv->vertex_shader = COGL_INVALID_HANDLE;
  priv->fragment_shader = COGL_INVALID_HANDLE;
}

/**
 * clutter_shader_new:
 *
 * Create a new #ClutterShader instance.
 *
 * Return value: a new #ClutterShader.
 *
 * Since: 0.6
 */
ClutterShader *
clutter_shader_new (void)
{
  return g_object_new (CLUTTER_TYPE_SHADER, NULL);
}


/**
 * clutter_shader_set_fragment_source:
 * @shader: a #ClutterShader
 * @data: GLSL source code.
 * @length: length of source buffer (currently ignored)
 *
 * Sets the GLSL source code to be used by a #ClutterShader for the fragment
 * program.
 *
 * Since: 0.6
 */
void
clutter_shader_set_fragment_source (ClutterShader      *shader,
                                    const gchar        *data,
                                    gssize              length)
{
  ClutterShaderPrivate *priv;
  gboolean is_glsl;

  /* FIXME: do not ignore length, since we are exposing it in the API */

  if (shader == NULL)
    g_error ("quack!");

  g_return_if_fail (CLUTTER_IS_SHADER (shader));
  g_return_if_fail (data != NULL);

  priv = shader->priv;

  /* release shader if bound when changing the source, the shader will
   * automatically be rebound on the next use.
   */
  if (clutter_shader_is_compiled (shader))
    clutter_shader_release (shader);

  is_glsl = !g_str_has_prefix (data, "!!ARBfp");

  g_free (priv->fragment_source);

  CLUTTER_NOTE (SHADER, "setting fragment shader (GLSL:%s, len:%" 
		G_GSSIZE_FORMAT ")",
                is_glsl ? "yes" : "no",
                length);

  priv->fragment_source = g_strdup (data);
  priv->fragment_is_glsl = is_glsl;
}

/**
 * clutter_shader_set_vertex_source:
 * @shader: a #ClutterShader
 * @data: GLSL source code.
 * @length: length of source buffer (currently ignored)
 *
 * Sets the GLSL source code to be used by a #ClutterShader for the vertex
 * program.
 *
 * Since: 0.6
 */
void
clutter_shader_set_vertex_source (ClutterShader      *shader,
                                  const gchar        *data,
                                  gssize              length)
{
  ClutterShaderPrivate *priv;
  gboolean is_glsl;

  if (shader == NULL)
    g_error ("quack!");

  g_return_if_fail (CLUTTER_IS_SHADER (shader));
  g_return_if_fail (data != NULL);

  priv = shader->priv;

  /* release shader if bound when changing the source, the shader will
   * automatically be rebound on the next use.
   */
  if (clutter_shader_is_compiled (shader))
    clutter_shader_release (shader);

  is_glsl = !g_str_has_prefix (data, "!!ARBvp");

  g_free (priv->vertex_source);

  CLUTTER_NOTE (SHADER, "setting vertex shader (GLSL:%s, len:%" 
		G_GSSIZE_FORMAT ")",
                is_glsl ? "yes" : "no",
                length);

  priv->vertex_source = g_strdup (data);
  priv->vertex_is_glsl = is_glsl;
}

static gboolean
bind_glsl_shader (ClutterShader  *self,
                  GError        **error)
{
  ClutterShaderPrivate *priv;
  priv = self->priv;

  priv->program = cogl_create_program ();

  if (priv->vertex_is_glsl && priv->vertex_source != COGL_INVALID_HANDLE)
    {
      GLint compiled = CGL_FALSE;

      priv->vertex_shader = cogl_create_shader (CGL_VERTEX_SHADER);

      cogl_shader_source (priv->vertex_shader, priv->vertex_source);
      cogl_shader_compile (priv->vertex_shader);

      cogl_shader_get_parameteriv (priv->vertex_shader,
                                   CGL_OBJECT_COMPILE_STATUS,
                                   &compiled);
      if (compiled != CGL_TRUE)
        {
          gchar error_buf[512];

          cogl_shader_get_info_log (priv->vertex_shader, 512, error_buf);

          g_set_error (error, CLUTTER_SHADER_ERROR,
                       CLUTTER_SHADER_ERROR_COMPILE,
                       "Vertex shader compilation failed: %s",
                       error_buf);

          return FALSE;
        }
      else
        cogl_program_attach_shader (priv->program, priv->vertex_shader);
    }

  if (priv->fragment_is_glsl && priv->fragment_source != COGL_INVALID_HANDLE)
    {
      GLint compiled = CGL_FALSE;

      priv->fragment_shader = cogl_create_shader (CGL_FRAGMENT_SHADER);

      cogl_shader_source (priv->fragment_shader, priv->fragment_source);
      cogl_shader_compile (priv->fragment_shader);

      cogl_shader_get_parameteriv (priv->fragment_shader,
                                   CGL_OBJECT_COMPILE_STATUS,
                                   &compiled);
      if (compiled != CGL_TRUE)
        {
          gchar error_buf[512];

          cogl_shader_get_info_log (priv->fragment_shader, 512, error_buf);

          g_set_error (error, CLUTTER_SHADER_ERROR,
                       CLUTTER_SHADER_ERROR_COMPILE,
                       "Fragment shader compilation failed: %s",
                       error_buf);

          return FALSE;
        }
      else
        cogl_program_attach_shader (priv->program, priv->fragment_shader);
    }

  cogl_program_link (priv->program);

  return TRUE;
}

/**
 * clutter_shader_compile:
 * @shader: a #ClutterShader
 * @error: return location for a #GError, or %NULL
 *
 * Compiles and links GLSL sources set for vertex and fragment shaders for
 * a #ClutterShader. If the compilation fails and a #GError return location is
 * provided the error will contain the errors from the compiler, if any.
 *
 * Return value: returns TRUE if the shader was succesfully compiled.
 *
 * Since: 0.8
 */
gboolean
clutter_shader_compile (ClutterShader  *shader,
                        GError        **error)
{
  ClutterShaderPrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_SHADER (shader), FALSE);

  priv = shader->priv;

  if (priv->compiled)
    return priv->compiled;

  if ((priv->vertex_source != COGL_INVALID_HANDLE && !priv->vertex_is_glsl) ||
      (priv->fragment_source != COGL_INVALID_HANDLE && !priv->fragment_is_glsl))
    {
      /* XXX: Could remove this check, since we only advertise support for GLSL
       * shaders anyways. */
      g_set_error (error, CLUTTER_SHADER_ERROR,
                   CLUTTER_SHADER_ERROR_NO_ASM,
                   "ASM shaders not supported");
      priv->compiled = FALSE;
      return priv->compiled;
    }

  if (!clutter_feature_available (CLUTTER_FEATURE_SHADERS_GLSL))
    {
      g_set_error (error, CLUTTER_SHADER_ERROR,
                   CLUTTER_SHADER_ERROR_NO_GLSL,
                   "GLSL shaders not supported");
      priv->compiled = FALSE;
      return priv->compiled;
    }

  priv->compiled = bind_glsl_shader (shader, error);
  g_object_notify (G_OBJECT (shader), "compiled");

  return priv->compiled;
}

/**
 * clutter_shader_release:
 * @shader: a #ClutterShader
 *
 * Frees up any GL context resources held by the shader.
 *
 * Since: 0.6
 */
void
clutter_shader_release (ClutterShader *shader)
{
  ClutterShaderPrivate *priv;

  g_return_if_fail (CLUTTER_IS_SHADER (shader));

  priv = shader->priv;

  if (!priv->compiled)
    return;

  g_assert (priv->program != COGL_INVALID_HANDLE);

  if (priv->vertex_is_glsl && priv->vertex_shader != COGL_INVALID_HANDLE)
    cogl_shader_unref (priv->vertex_shader);

  if (priv->fragment_is_glsl && priv->fragment_shader != COGL_INVALID_HANDLE)
    cogl_shader_unref (priv->fragment_shader);

  if (priv->program != COGL_INVALID_HANDLE)
    cogl_program_unref (priv->program);

  priv->vertex_shader = COGL_INVALID_HANDLE;
  priv->fragment_shader = COGL_INVALID_HANDLE;
  priv->program = COGL_INVALID_HANDLE;
  priv->compiled = FALSE;

  g_object_notify (G_OBJECT (shader), "compiled");
}

/**
 * clutter_shader_is_compiled:
 * @shader: a #ClutterShader
 *
 * Checks whether @shader is is currently compiled, linked and bound
 * to the GL context.
 *
 * Return value: %TRUE if the shader is compiled, linked and ready for use.
 *
 * Since: 0.8
 */
gboolean
clutter_shader_is_compiled (ClutterShader *shader)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER (shader), FALSE);

  return shader->priv->compiled;
}

/**
 * clutter_shader_set_is_enabled:
 * @shader: a #ClutterShader
 * @enabled: The new state of the shader.
 *
 * Enables a shader. This function will attempt to compile and link
 * the shader, if it isn't already.
 *
 * When @enabled is %FALSE the default state of the GL pipeline will be
 * used instead.
 *
 * Since: 0.6
 */
void
clutter_shader_set_is_enabled (ClutterShader *shader,
                               gboolean       enabled)
{
  ClutterShaderPrivate *priv;

  g_return_if_fail (CLUTTER_IS_SHADER (shader));

  priv = shader->priv;

  if (priv->is_enabled != enabled)
    {
      GError *error = NULL;
      gboolean res;

      res = clutter_shader_compile (shader, &error);
      if (!res)
        {
          g_warning ("Unable to bind the shader: %s",
                     error ? error->message : "unknown error");
          if (error)
            g_error_free (error);

          return;
        }

      priv->is_enabled = enabled;

      if (priv->is_enabled)
        cogl_program_use (priv->program);
      else
        cogl_program_use (COGL_INVALID_HANDLE);

      g_object_notify (G_OBJECT (shader), "enabled");
    }
}

/**
 * clutter_shader_get_is_enabled:
 * @shader: a #ClutterShader
 *
 * Checks whether @shader is enabled.
 *
 * Return value: %TRUE if the shader is enabled.
 *
 * Since: 0.6
 */
gboolean
clutter_shader_get_is_enabled (ClutterShader *shader)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER (shader), FALSE);

  return shader->priv->is_enabled;
}

/**
 * clutter_shader_set_uniform_1f:
 * @shader: a #ClutterShader
 * @name: name of uniform in vertex or fragment program to set.
 * @value: the new value of the uniform.
 *
 * Sets a user configurable variable in the shader programs attached
 * to a #ClutterShader.
 *
 * Since: 0.6
 */
void
clutter_shader_set_uniform_1f (ClutterShader *shader,
                               const gchar   *name,
                               gfloat         value)
{
  ClutterShaderPrivate *priv;
  GLint                 location = 0;
  GLfloat               foo      = value;

  g_return_if_fail (CLUTTER_IS_SHADER (shader));

  priv = shader->priv;

  location = cogl_program_get_uniform_location (priv->program, name);
  cogl_program_uniform_1f (location, foo);
}

/*
 * _clutter_shader_release_all:
 *
 * Iterate through all #ClutterShaders and tell them to release GL context
 * related sources.
 *
 * Since: 0.6
 */
void
_clutter_shader_release_all (void)
{
  g_list_foreach (clutter_shaders_list,
                  (GFunc) clutter_shader_release,
                  NULL);
}


/**
 * clutter_shader_get_fragment_source:
 * @shader: a #ClutterShader
 *
 * Query the current GLSL fragment source set on @shader.
 *
 * Return value: the source of the fragment shader for this
 * ClutterShader object or %NULL. The returned string is owned by the
 * shader object and should never be modified or freed
 *
 * Since: 0.6
 */
G_CONST_RETURN gchar *
clutter_shader_get_fragment_source (ClutterShader *shader)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER (shader), NULL);
  return shader->priv->fragment_source;
}

/**
 * clutter_shader_get_vertex_source:
 * @shader: a #ClutterShader
 *
 * Query the current GLSL vertex source set on @shader.
 *
 * Return value: the source of the vertex shader for this
 * ClutterShader object or %NULL. The returned string is owned by the
 * shader object and should never be modified or freed
 *
 * Since: 0.6
 */
G_CONST_RETURN gchar *
clutter_shader_get_vertex_source (ClutterShader *shader)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER (shader), NULL);
  return shader->priv->vertex_source;
}

GQuark
clutter_shader_error_quark (void)
{
  return g_quark_from_static_string ("clutter-shader-error");
}

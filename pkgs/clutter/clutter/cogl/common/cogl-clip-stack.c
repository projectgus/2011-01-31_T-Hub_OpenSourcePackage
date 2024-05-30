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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cogl.h"
#include "cogl-clip-stack.h"

/* These are defined in the particular backend (float in GL vs fixed
   in GL ES) */
void _cogl_set_clip_planes (ClutterFixed x,
			    ClutterFixed y,
			    ClutterFixed width,
			    ClutterFixed height);
void _cogl_init_stencil_buffer (void);
void _cogl_add_stencil_clip (ClutterFixed x,
			     ClutterFixed y,
			     ClutterFixed width,
			     ClutterFixed height,
			     gboolean     first);
void _cogl_disable_clip_planes (void);
void _cogl_disable_stencil_buffer (void);
void _cogl_set_matrix (const ClutterFixed *matrix);

typedef struct _CoglClipStackEntry CoglClipStackEntry;

struct _CoglClipStackEntry
{
  /* The rectangle for this clip */
  ClutterFixed        x_offset;
  ClutterFixed        y_offset;
  ClutterFixed        width;
  ClutterFixed        height;

  /* The matrix that was current when the clip was set */
  ClutterFixed        matrix[16];
};

static GList *cogl_clip_stack_top = NULL;
static GList *cogl_clip_stack_bottom = NULL;
static int    cogl_clip_stack_depth = 0;

static void
_cogl_clip_stack_add (const CoglClipStackEntry *entry, int depth)
{
  int has_clip_planes = cogl_features_available (COGL_FEATURE_FOUR_CLIP_PLANES);

  /* If this is the first entry and we support clip planes then use
     that instead */
  if (depth == 1 && has_clip_planes)
    _cogl_set_clip_planes (entry->x_offset,
			   entry->y_offset,
			   entry->width,
			   entry->height);
  else
    _cogl_add_stencil_clip (entry->x_offset,
			    entry->y_offset,
			    entry->width,
			    entry->height,
			    depth == (has_clip_planes ? 2 : 1));
}

void
cogl_clip_set (ClutterFixed x_offset,
	       ClutterFixed y_offset,
	       ClutterFixed width,
	       ClutterFixed height)
{
  CoglClipStackEntry *entry = g_slice_new (CoglClipStackEntry);

  /* Make a new entry */
  entry->x_offset = x_offset;
  entry->y_offset = y_offset;
  entry->width = width;
  entry->height = height;

  cogl_get_modelview_matrix (entry->matrix);

  /* Add the entry to the current clip */
  _cogl_clip_stack_add (entry, ++cogl_clip_stack_depth);

  /* Store it in the stack */
  cogl_clip_stack_top = g_list_prepend (cogl_clip_stack_top, entry);
  if (cogl_clip_stack_bottom == NULL)
    cogl_clip_stack_bottom = cogl_clip_stack_top;
}

void
cogl_clip_unset (void)
{
  g_return_if_fail (cogl_clip_stack_top != NULL);

  /* Remove the top entry from the stack */
  g_slice_free (CoglClipStackEntry, cogl_clip_stack_top->data);
  cogl_clip_stack_top = g_list_delete_link (cogl_clip_stack_top,
					    cogl_clip_stack_top);
  if (cogl_clip_stack_top == NULL)
    cogl_clip_stack_bottom = NULL;
  cogl_clip_stack_depth--;

  /* Rebuild the clip */
  _cogl_clip_stack_rebuild (FALSE);
}

void
_cogl_clip_stack_rebuild (gboolean just_stencil)
{
  int has_clip_planes = cogl_features_available (COGL_FEATURE_FOUR_CLIP_PLANES);
  GList *node;
  int depth = 1;

  /* Disable clip planes if the stack is empty */
  if (has_clip_planes && cogl_clip_stack_depth < 1)
    _cogl_disable_clip_planes ();

  /* Disable the stencil buffer if there isn't enough entries */
  if (cogl_clip_stack_depth < (has_clip_planes ? 2 : 1))
    _cogl_disable_stencil_buffer ();

  /* Re-add every entry from the bottom of the stack up */
  for (node = cogl_clip_stack_bottom; node; node = node->prev, depth++)
    if (!just_stencil || !has_clip_planes || depth > 1)
      {
	const CoglClipStackEntry *entry = (CoglClipStackEntry *) node->data;
	cogl_push_matrix ();
	_cogl_set_matrix (entry->matrix);
	_cogl_clip_stack_add (entry, depth);
	cogl_pop_matrix ();
      }
}

void
_cogl_clip_stack_merge (void)
{
  GList *node = cogl_clip_stack_bottom;

  /* Merge the current clip stack on top of whatever is in the stencil
     buffer */
  if (node)
    {
      /* Skip the first entry if we have clipping planes */
      if (cogl_features_available (COGL_FEATURE_FOUR_CLIP_PLANES))
	node = node->prev;

      while (node)
	{
	  const CoglClipStackEntry *entry = (CoglClipStackEntry *) node->data;
	  cogl_push_matrix ();
	  _cogl_set_matrix (entry->matrix);
	  _cogl_clip_stack_add (entry, 3);
	  cogl_pop_matrix ();

	  node = node->prev;
	}
    }
}

/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006, 2007 OpenedHand
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

#ifndef _CLUTTER_EFFECT
#define _CLUTTER_EFFECT

#include <glib-object.h>
#include <clutter/clutter-timeline.h>
#include <clutter/clutter-alpha.h>
#include <clutter/clutter-behaviour.h>

G_BEGIN_DECLS

typedef void (*ClutterEffectCompleteFunc) (ClutterActor *actor,
					   gpointer       user_data);

#define CLUTTER_TYPE_EFFECT_TEMPLATE clutter_effect_template_get_type()

#define CLUTTER_EFFECT_TEMPLATE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_EFFECT_TEMPLATE, ClutterEffectTemplate))

#define CLUTTER_EFFECT_TEMPLATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_EFFECT_TEMPLATE, ClutterEffectTemplateClass))

#define CLUTTER_IS_EFFECT_TEMPLATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_EFFECT_TEMPLATE))

#define CLUTTER_IS_EFFECT_TEMPLATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_EFFECT_TEMPLATE))

#define CLUTTER_EFFECT_TEMPLATE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_EFFECT_TEMPLATE, ClutterEffectTemplateClass))

typedef struct {
  GObject parent;
} ClutterEffectTemplate;

typedef struct {
  GObjectClass parent_class;
} ClutterEffectTemplateClass;

GType clutter_effect_template_get_type (void);

ClutterEffectTemplate*
clutter_effect_template_new (ClutterTimeline *timeline, 
			     ClutterAlphaFunc alpha_func);

ClutterTimeline*
clutter_effect_fade (ClutterEffectTemplate     *template,
		     ClutterActor             *actor,
		     guint8                    start_opacity,
		     guint8                    end_opacity,
		     ClutterEffectCompleteFunc completed_func,
		     gpointer                  completed_data);

ClutterTimeline*
clutter_effect_move (ClutterEffectTemplate    *template,
		     ClutterActor             *actor,
		     const ClutterKnot        *knots,
		     guint                     n_knots,
		     ClutterEffectCompleteFunc completed_func,
		     gpointer                  completed_data);

ClutterTimeline*
clutter_effect_scale (ClutterEffectTemplate    *template,
		      ClutterActor             *actor,
		      gdouble                   scale_begin,
		      gdouble                   scale_end,
		      ClutterGravity            gravity,
		      ClutterEffectCompleteFunc completed_func,
		      gpointer                  completed_userdata);

G_END_DECLS

#endif /* _CLUTTER_EFFECT */

G_END_DECLS

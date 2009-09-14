#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-stage-egl.h"
#include "clutter-egl.h"
#include "clutter-backend-egl.h"

#include "../clutter-main.h"
#include "../clutter-feature.h"
#include "../clutter-color.h"
#include "../clutter-util.h"
#include "../clutter-event.h"
#include "../clutter-enum-types.h"
#include "../clutter-private.h"
#include "../clutter-debug.h"
#include "../clutter-units.h"
#include "../clutter-stage.h"
#include "../clutter-stage-window.h"

#include <fcntl.h> /* for open() */

static void clutter_stage_window_iface_init (ClutterStageWindowIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterStageEGL,
                         clutter_stage_egl,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_STAGE_WINDOW,
                                                clutter_stage_window_iface_init));

static void
clutter_stage_egl_show (ClutterStageWindow *stage_window,
                        gboolean            do_raise)
{
        ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (stage_window);
        clutter_actor_map (CLUTTER_ACTOR (stage_egl->wrapper));
}
static void
clutter_stage_egl_hide (ClutterStageWindow *stage_window)
{
        ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (stage_window);
        clutter_actor_unmap (CLUTTER_ACTOR (stage_egl->wrapper));
}
static void
clutter_stage_egl_unrealize (ClutterStageWindow *stage_window)
{
  ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (stage_window);

  CLUTTER_MARK();

  if (stage_egl->egl_surface)
    {
      eglDestroySurface (clutter_egl_display (), stage_egl->egl_surface);
      stage_egl->egl_surface = EGL_NO_SURFACE;
    }
}

static gboolean
clutter_stage_egl_realize (ClutterStageWindow *stage_window)
{
  ClutterStageEGL     *stage_egl = CLUTTER_STAGE_EGL (stage_window);
  ClutterBackendEGL   *backend_egl;
  EGLConfig            configs[2];
  EGLint               config_count;
  EGLBoolean           status;
  gboolean             is_offscreen;
  //clutter_debug_flags = CLUTTER_DEBUG_PAINT|CLUTTER_DEBUG_ACTOR|CLUTTER_DEBUG_GL|CLUTTER_DEBUG_BACKEND;
  CLUTTER_NOTE (BACKEND, "Realizing main stage");

  g_object_get (stage_egl->wrapper, "offscreen", &is_offscreen, NULL);

  backend_egl = CLUTTER_BACKEND_EGL (clutter_get_default_backend ());

  if (G_LIKELY (!is_offscreen))
    {
      EGLint cfg_attribs[] = { EGL_BUFFER_SIZE,     EGL_DONT_CARE,
			       EGL_RED_SIZE,        5,
			       EGL_GREEN_SIZE,      6,
			       EGL_BLUE_SIZE,       5,
			       EGL_DEPTH_SIZE,      16,
			       EGL_ALPHA_SIZE,      EGL_DONT_CARE,
			       EGL_STENCIL_SIZE,    2, 
#ifdef HAVE_COGL_GLES2
			       EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else /* HAVE_COGL_GLES2 */
			       EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
#endif /* HAVE_COGL_GLES2 */
			       EGL_NONE };

      status = eglGetConfigs (backend_egl->edpy,
			      configs, 
			      2, 
			      &config_count);

      if (status != EGL_TRUE)
        {
                g_critical ("eglGetConfigs failed %x",eglGetError());
          return FALSE;
        }

      status = eglChooseConfig (backend_egl->edpy,
				cfg_attribs,
				configs,
                                G_N_ELEMENTS (configs),
				&config_count);

      if (status != EGL_TRUE)
        {
          g_critical ("eglChooseConfig failed");
          return FALSE;
        }

      CLUTTER_NOTE (BACKEND, "Got %i configs", config_count); 

      if (stage_egl->egl_surface != EGL_NO_SURFACE)
        {
	  eglDestroySurface (backend_egl->edpy, stage_egl->egl_surface);
          stage_egl->egl_surface = EGL_NO_SURFACE;
        }

       if (backend_egl->egl_context)
         {
            eglDestroyContext (backend_egl->edpy, backend_egl->egl_context);
            backend_egl->egl_context = NULL;
         }

       stage_egl->egl_surface =
        eglCreateWindowSurface (backend_egl->edpy,
                                configs[0],
                                (NativeWindowType)NULL,
                                NULL);
      if (stage_egl->egl_surface == EGL_NO_SURFACE)
        {  /* AMD GPU driver need a valid fd to framebuffer device */
          stage_egl->egl_surface =
            eglCreateWindowSurface (backend_egl->edpy,
                                    configs[0],
                                    (NativeWindowType)open("/dev/fb0",O_RDWR),
                                    NULL);
        }
      
      if (stage_egl->egl_surface == EGL_NO_SURFACE)
        {
	  g_critical ("Unable to create an EGL surface");

          return FALSE;
        }

      if (G_UNLIKELY (backend_egl->egl_context == NULL))
        {
#ifdef HAVE_COGL_GLES2
	  static const EGLint attribs[3]
	    = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

          backend_egl->egl_context = eglCreateContext (backend_egl->edpy,
						       configs[0],
                                                       EGL_NO_CONTEXT,
                                                       attribs);
#else
          /* Seems some GLES implementations 1.x do not like attribs... */
          backend_egl->egl_context = eglCreateContext (backend_egl->edpy,
						       configs[0],
                                                       EGL_NO_CONTEXT,
                                                       NULL);
#endif

          if (backend_egl->egl_context == EGL_NO_CONTEXT)
            {
              g_critical ("Unable to create a suitable EGL context");

              return FALSE;
            }

          CLUTTER_NOTE (GL, "Created EGL Context");
        }

      CLUTTER_NOTE (BACKEND, "Setting context");

      /* eglnative can have only one stage */
      status = eglMakeCurrent (backend_egl->edpy,
                               stage_egl->egl_surface,
                               stage_egl->egl_surface,
                               backend_egl->egl_context);

      if (status != EGL_TRUE)
        {
          g_critical ("eglMakeCurrent failed");
          
          return FALSE;
        }

      eglQuerySurface (backend_egl->edpy,
		       stage_egl->egl_surface,
		       EGL_WIDTH,
		       &stage_egl->surface_width);

      eglQuerySurface (backend_egl->edpy,
		       stage_egl->egl_surface,
		       EGL_HEIGHT,
		       &stage_egl->surface_height);

      CLUTTER_NOTE (BACKEND, "EGL surface is %ix%i", 
		    stage_egl->surface_width,
                    stage_egl->surface_height);

      /* since we only have one size and it cannot change, we
       * just need to update the GL viewport now that we have
       * been realized
       */
      clutter_stage_ensure_viewport (CLUTTER_STAGE (stage_egl->wrapper));
      clutter_actor_queue_relayout (CLUTTER_ACTOR (stage_egl->wrapper));
    }
  else
    {
      g_warning ("EGL Backend does not yet support offscreen rendering\n");
      return FALSE;
    }
  return TRUE;
}

static void
clutter_stage_egl_get_geometry (ClutterStageWindow *stage_window,
                                ClutterGeometry    *geometry)
{
  ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (stage_window);
  
  geometry->width = stage_egl->surface_width;
  geometry->height = stage_egl->surface_height;

}
static void
clutter_stage_egl_dispose (GObject *gobject)
{
  G_OBJECT_CLASS (clutter_stage_egl_parent_class)->dispose (gobject);
}

static void
clutter_stage_egl_class_init (ClutterStageEGLClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = clutter_stage_egl_dispose;
}

static void
clutter_stage_egl_set_fullscreen (ClutterStageWindow *stage_window,
                                  gboolean            fullscreen)
{
  g_warning ("Stage of type '%s' do not support ClutterStage::set_fullscreen",
             G_OBJECT_TYPE_NAME (stage_window));
}

static ClutterActor *
clutter_stage_egl_get_wrapper (ClutterStageWindow *stage_window)
{
  return CLUTTER_ACTOR (CLUTTER_STAGE_EGL (stage_window)->wrapper);
}

static void
clutter_stage_window_iface_init (ClutterStageWindowIface *iface)
{
  iface->set_fullscreen = clutter_stage_egl_set_fullscreen;
  iface->set_title = NULL;
  iface->get_wrapper = clutter_stage_egl_get_wrapper;
  iface->get_geometry = clutter_stage_egl_get_geometry;
  iface->show = clutter_stage_egl_show;
  iface->hide = clutter_stage_egl_hide;
  iface->realize = clutter_stage_egl_realize;
  iface->unrealize = clutter_stage_egl_unrealize;
}

static void
clutter_stage_egl_init (ClutterStageEGL *stage)
{
}

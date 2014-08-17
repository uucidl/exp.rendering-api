#include <library/visuals/framebuffer.h>
#include "framebuffer.h"

static void activate (framebuffer_t* self)
{
        glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, self->name);
        OGL_TRACE;

        if (self->flags & FB_DEPTH) {
                glDrawBuffer(GL_NONE);
                OGL_TRACE;
                glReadBuffer(GL_NONE);
                OGL_TRACE;
        } else {
                glDrawBuffer (GL_COLOR_ATTACHMENT0_EXT);
                OGL_TRACE;
                glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
                OGL_TRACE;
        }

        glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        OGL_TRACE;
        glViewport(0, 0, self->texture.width, self->texture.height);
        self->active_p = 1;
}

static void deactivate (framebuffer_t* self)
{
        glPopAttrib();
        OGL_TRACE;
        glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, self->parent_name);
        OGL_TRACE;

        if (!self->parent_name) {
                glReadBuffer (GL_BACK);
                glDrawBuffer (GL_BACK);
        }
        self->active_p = 0;
}

static int is_complete()
{
        int ok = 0;
        GLenum status;
        status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
                ok = 1;
                break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                /* choose different formats */
                printf ("GL_FRAMEBUFFER_UNSUPPORTED\n");
                break;
        default:
                printf ("Other error\n");
                /* programming error; will fail on all hardware */
                break;
        }

        return ok;
}

static void* hook_world_allocate ()
{
        return NULL;
}

static void hook_world_free (void* world)
{
        (void) world;
}

static framebuffer_t* framebuffer_of_frame (display_frame_t frame)
{
        char* const sself = (char*) frame;

        return (framebuffer_t*) (sself - offsetof(framebuffer_t, frame));
}

static void hook_world_define(void* world, display_frame_t frame)
{
        framebuffer_t* self = framebuffer_of_frame (frame);

        (void) world;

        if (self->parent_framebuffer && self->parent_framebuffer->active_p) {
                deactivate(self->parent_framebuffer);
        } else {
                self->parent_framebuffer = NULL;
        }

        activate (self);
}

static void hook_world_commit(void* world, display_frame_t tframe)
{
        framebuffer_t* self = framebuffer_of_frame (tframe);

        (void) world;

        frame_t* frame = &self->frame;
        frame_stats_collect (&self->stats, frame, NULL, "framebuffer-frame");

        deactivate (self);

        if (self->parent_framebuffer) {
                activate(self->parent_framebuffer);
        }
}

extern void opengl_framebuffer_define_texture(framebuffer_t* self,
                int texture_flags, int texture_unit)
{
        int sizes[2];

        texture_flags |= TF_STREAMING;

        display_device_resolution_in_px(self->device, sizes);

        if (! (self->flags & FB_PRESERVE_RESOLUTION) ) {
                int i;
                for (i = 0; i < ARRAY_N(sizes); i++) {
                        sizes[i] = ceil_to_power_of_two(sizes[i]) / self->scale;
                }
        }

        if (self->flags & FB_DEPTH) {
                private_texture_define_with_format (&self->texture, texture_flags,
                                                    texture_unit, 2, sizes,
                                                    GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE);
        } else {
                private_texture_define (&self->texture, texture_flags, texture_unit, 2,
                                        sizes);

        }

}

extern void opengl_framebuffer_make (framebuffer_t* self)
{
        self->active_p = 0;
        self->parent_framebuffer = NULL;
        self->fbo_p = GLEW_EXT_framebuffer_object;
        self->parent_name = 0;
        self->depthrenderbuffer = 0;

        if (self->fbo_p) {
                glGenFramebuffersEXT(1, &self->name);
                glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, (GLint*) &self->parent_name);

                opengl_framebuffer_define_texture(self, 0, 0);

                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, self->name);
                OGL_TRACE;

                if (self->flags & FB_DEPTH) {
                        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                                  self->texture.target, self->texture.id, 0);
                        OGL_TRACE;

                        glDrawBuffer(GL_NONE);
                        OGL_TRACE;
                        glReadBuffer(GL_NONE);
                } else {
                        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                                  self->texture.target, self->texture.id, 0);

                        glGenRenderbuffersEXT(1, &self->depthrenderbuffer);
                }
                OGL_TRACE;

                if (!is_complete()) {
                        printf ("could not setup framebuffer extension.\n");
                        self->fbo_p = 0;
                } else {
                        self->hook_id = display_frame_add_hooks (&self->frame,
                                        hook_world_allocate,
                                        hook_world_free,
                                        hook_world_define,
                                        hook_world_commit);
                }

                if (0 != self->depthrenderbuffer) {
                        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, self->depthrenderbuffer);
                        OGL_TRACE;
                        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                                                 self->texture.twidth, self->texture.theight);
                        OGL_TRACE;
                        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                                     GL_RENDERBUFFER_EXT, self->depthrenderbuffer);
                        OGL_TRACE;
                        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
                        OGL_TRACE;
                }

                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, self->parent_name);
                OGL_TRACE;
        } else {
                printf ("ERROR: FBO extension unavailable!\n");
        }

        frame_stats_make (&self->stats);
}

extern void opengl_framebuffer_destroy (framebuffer_t* self)
{
        if (self->fbo_p) {
                glDeleteFramebuffersEXT(1, &self->name);
                display_frame_remove_hooks (&self->frame, self->hook_id);

                if (0 != self->depthrenderbuffer) {
                        glDeleteRenderbuffers(1, &self->depthrenderbuffer);
                }
        }

        frame_stats_destroy (&self->stats);
}

extern display_frame_t opengl_framebuffer_addf (framebuffer_t* self,
                display_frame_t original)
{
        frame_t* frame = &self->frame;
        frame_sharecontext(frame, (frame_t*) original);
        frame->tentative_ms = original->tentative_ms;

        if (frame_is_for_framebuffer(original)) {
                self->parent_framebuffer = framebuffer_of_frame(original);
        } else {
                self->parent_framebuffer = NULL;
        }

        return frame;
}

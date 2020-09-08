/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - m64p_vidext.h                                      *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This header file defines typedefs for function pointers to the core's
 * video extension functions.
 */

#if !defined(M64P_VIDEXT_H)
#define M64P_VIDEXT_H

#include "m64p_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* VidExt_Init()
 *
 * This function should be called from within the InitiateGFX() video plugin
 * function call. The default SDL implementation of this function simply calls
 * SDL_InitSubSystem(SDL_INIT_VIDEO). It does not open a rendering window or
 * switch video modes.
 */
typedef m64p_error (*ptr_VidExt_Init)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_Init(void);
#endif

/* VidExt_Quit()
 *
 * This function closes any open rendering window and shuts down the video
 * system. The default SDL implementation of this function calls
 * SDL_QuitSubSystem(SDL_INIT_VIDEO). This function should be called from
 * within the RomClose() video plugin function.
 */
typedef m64p_error (*ptr_VidExt_Quit)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_Quit(void);
#endif

/* VidExt_ListFullscreenModes()
 *
 * This function is used to enumerate the available resolutions for fullscreen
 * video modes. A pointer to an array is passed into the function, which is
 * then filled with resolution sizes.
 */
typedef m64p_error (*ptr_VidExt_ListFullscreenModes)(m64p_2d_size *, int *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_ListFullscreenModes(m64p_2d_size *, int *);
#endif

/* VidExt_ListFullscreenRates()
 *
 * This function is used to enumerate the available refresh rates for a fullscreen
 * video mode.
 * 
 */
typedef m64p_error (*ptr_VidExt_ListFullscreenRates)(m64p_2d_size, int *, int *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_ListFullscreenRates(m64p_2d_size, int *, int *);
#endif

/* VidExt_SetVideoMode()
 *
 * This function creates a rendering window or switches into a fullscreen
 * video mode. Any desired OpenGL attributes should be set before calling
 * this function.
 */
typedef m64p_error (*ptr_VidExt_SetVideoMode)(int, int, int, m64p_video_mode, m64p_video_flags);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_SetVideoMode(int, int, int, m64p_video_mode, m64p_video_flags);
#endif

/* VidExt_SetVideoModeWithRate()
 *
 * This function creates a rendering window or switches into a fullscreen
 * video mode. Any desired OpenGL attributes should be set before calling
 * this function.
 */
typedef m64p_error (*ptr_VidExt_SetVideoModeWithRate)(int, int, int, int, m64p_video_mode, m64p_video_flags);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_SetVideoModeWithRate(int, int, int, int, m64p_video_mode, m64p_video_flags);
#endif

/* VidExt_ResizeWindow()
 *
 * This function resizes the opengl rendering window to match the given size.
 */
typedef m64p_error (*ptr_VidExt_ResizeWindow)(int, int);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_ResizeWindow(int, int);
#endif

/* VidExt_SetCaption()
 *
 * This function sets the caption text of the emulator rendering window.
 */
typedef m64p_error (*ptr_VidExt_SetCaption)(const char *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_SetCaption(const char *);
#endif

/* VidExt_ToggleFullScreen()
 *
 * This function toggles between fullscreen and windowed rendering modes.
 */
typedef m64p_error (*ptr_VidExt_ToggleFullScreen)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_ToggleFullScreen(void);
#endif

/* VidExt_GL_GetProcAddress()
 *
 * This function is used to get a pointer to an OpenGL extension function. This
 * is only necessary on the Windows platform, because the OpenGL implementation
 * shipped with Windows only supports OpenGL version 1.1.
 */
typedef m64p_function (*ptr_VidExt_GL_GetProcAddress)(const char *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_function CALL VidExt_GL_GetProcAddress(const char *);
#endif

/* VidExt_GL_SetAttribute()
 *
 * This function is used to set certain OpenGL attributes which must be
 * specified before creating the rendering window with VidExt_SetVideoMode.
 */
typedef m64p_error (*ptr_VidExt_GL_SetAttribute)(m64p_GLattr, int);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_GL_SetAttribute(m64p_GLattr, int);
#endif

/* VidExt_GL_GetAttribute()
 *
 * This function is used to get the value of OpenGL attributes.  These values may
 * be changed when calling VidExt_SetVideoMode.
 */
typedef m64p_error (*ptr_VidExt_GL_GetAttribute)(m64p_GLattr, int *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_GL_GetAttribute(m64p_GLattr, int *);
#endif

/* VidExt_GL_SwapBuffers()
 *
 * This function is used to swap the front/back buffers after rendering an
 * output video frame.
 */
typedef m64p_error (*ptr_VidExt_GL_SwapBuffers)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL VidExt_GL_SwapBuffers(void);
#endif

/* VidExt_GL_GetDefaultFramebuffer()
 *
 * On some platforms (for instance, iOS) the default framebuffer object
 * depends on the surface being rendered to, and might be different from 0.
 * This function should be called after VidExt_SetVideoMode to retrieve the
 * name of the default FBO.
 * Calling this function may have performance implications
 * and it should not be called every time the default FBO is bound.
 */
typedef uint32_t (*ptr_VidExt_GL_GetDefaultFramebuffer)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT uint32_t CALL VidExt_GL_GetDefaultFramebuffer(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #define M64P_VIDEXT_H */


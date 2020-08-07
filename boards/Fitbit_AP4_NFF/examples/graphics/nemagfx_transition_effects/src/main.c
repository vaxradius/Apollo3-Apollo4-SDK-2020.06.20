//*****************************************************************************
//
//! @file main.c
//!
//! @brief NemaGFX example.
//! this example use one frame buffer demonstrate two picture trasition effect,
//! with Nema GPU support, the effect include
//! NEMA_TRANS_LINEAR_H,
//! NEMA_TRANS_CUBE_H,
//! NEMA_TRANS_INNERCUBE_H,
//! NEMA_TRANS_STACK_H,
//! NEMA_TRANS_LINEAR_V,
//! NEMA_TRANS_CUBE_V,
//! NEMA_TRANS_INNERCUBE_V,
//! NEMA_TRANS_STACK_V,
//! NEMA_TRANS_FADE,
//! NEMA_TRANS_FADE_ZOOM,
//! NEMA_TRANS_MAX,
//! NEMA_TRANS_NONE
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision v2.4.2-1241-g15e33e803 of the AmbiqSuite Development Package.
//
//*****************************************************************************

// -----------------------------------------------------------------------------
// Copyright (c) 2019 Think Silicon S.A.
// Think Silicon S.A. Confidential Proprietary
// -----------------------------------------------------------------------------
//     All Rights reserved - Unpublished -rights reserved under
//         the Copyright laws of the European Union
//
//  This file includes the Confidential information of Think Silicon S.A.
//  The receiver of this Confidential Information shall not disclose
//  it to any third party and shall protect its confidentiality by
//  using the same degree of care, but not less than a reasonable
//  degree of care, as the receiver uses to protect receiver's own
//  Confidential Information. The entire notice must be reproduced on all
//  authorised copies and copies may only be made to the extent permitted
//  by a licensing agreement from Think Silicon S.A..
//
//  The software is provided 'as is', without warranty of any kind, express or
//  implied, including but not limited to the warranties of merchantability,
//  fitness for a particular purpose and noninfringement. In no event shall
//  Think Silicon S.A. be liable for any claim, damages or other liability, whether
//  in an action of contract, tort or otherwise, arising from, out of or in
//  connection with the software or the use or other dealings in the software.
//
//
//                    Think Silicon S.A.
//                    http://www.think-silicon.com
//                    Patras Science Park
//                    Rion Achaias 26504
//                    Greece
// -----------------------------------------------------------------------------

#include <string.h>

#include "am_bsp.h"
#include "am_util_delay.h"
#ifdef ENABLE_DSI
    #include "am_devices_dsi_rm67162.h"
#else
    #include "am_devices_nemadc_rm67162.h"
#endif

#include "nema_core.h"
#include "nema_utils.h"
#include "nema_dc.h"
#include "nema_dc_mipi.h"

#include "nema_transitions.h"
#include "nema_utils.h"

#include "im00.rgba565.h"
#include "im01.rgba565.h"

#define RESX 256
#define RESY 256

static img_obj_t screen0 = {
                        {.size = im00_rgba565_len,
                         .base_virt = (void *)im00_rgba565,
                         .base_phys = (uintptr_t)im00_rgba565},
                         256, 256, -1, 0, (uint8_t)NEMA_RGB565, 0};
static img_obj_t screen1 = {
                        {.size = im01_rgba565_len,
                         .base_virt = (void *)im01_rgba565,
                         .base_phys = (uintptr_t)im01_rgba565},
                         256, 256, -1, 0, (uint8_t)NEMA_RGB565, 0};


#define FRAME_BUFFERS  1
static img_obj_t fb[FRAME_BUFFERS];
static nemadc_layer_t layer[3] = {{0}};

static void
load_objects(void)
{
    for (int i = 0; i < FRAME_BUFFERS; ++i)
    {
        fb[i].w = RESX;
        fb[i].h = RESY;
        fb[i].format = NEMA_RGBA8888;
        fb[i].stride = RESX*4;
        fb[i].bo = nema_buffer_create(fb[i].stride * fb[i].h);
        (void)nema_buffer_map(&fb[i].bo);

        layer[i].sizex = layer[i].resx = fb[i].w;
        layer[i].sizey = layer[i].resy = fb[i].h;
        layer[i].stride = fb[i].stride;
        layer[i].format = NEMADC_RGBA8888 | (nemadc_format_t)NEMADC_MODULATE_A;
        layer[i].blendmode = NEMADC_BL_SIMPLE;
        layer[i].alpha     = 0x80;
        layer[i].baseaddr_phys = fb[i].bo.base_phys;
        layer[i].baseaddr_virt = fb[i].bo.base_virt;

        // printf("FB: V:%p P:0x%08x\n", (void *)fb[i].bo.base_virt, fb[i].bo.base_phys);
    }

    screen0.bo = nema_buffer_create( im00_rgba565_len );
    memcpy( screen0.bo.base_virt, im00_rgba565, im00_rgba565_len);
    screen1.bo = nema_buffer_create( im01_rgba565_len );
    memcpy( screen1.bo.base_virt, im01_rgba565, im01_rgba565_len);

#ifdef NON_UNIFIED_MEMORY
    screen0.bo = nema_load_file("im00.rgba565", -1, 0);
    screen1.bo = nema_load_file("im01.rgba565", -1, 0);
#endif
}

static nema_cmdlist_t cl;
static int cur_fb = 0;

static void
swap_fb(void)
{
#if 0
   uint16_t* pixel = (uint16_t*)layer[cur_fb].baseaddr_virt;
    nemadc_wait_vsync();
    for (int i = 0; i < RESX*RESY; i++)
    {
      // swap R and B, then swap high and low bytes
      //*pixel = ((*pixel) << 11) | ((*pixel) >> 11) | (*pixel & 0x7E0);
      //*pixel = ((*pixel & 0xff) << 8) | ((*pixel & 0xff00) >> 8);
      *pixel = ((*pixel & 0x700) >> 8) | ((*pixel & 0x1f) << 3) | ((*pixel & 0xf800) >> 3) | ((*pixel & 0xe0) << 8);
      pixel++;
    }
    nemadc_set_layer(0, &layer[cur_fb]);
    cur_fb = (cur_fb+1)%FRAME_BUFFERS;
#else

    layer[cur_fb].startx = 0;
    layer[cur_fb].starty = 0;
    nemadc_set_layer(0, &layer[cur_fb]);

#ifdef MULTI_FB_TEST
    // This segment enables layers 1, 2, 3 (in addition to layer 0)
    // Layers 1, 2, 3 start x,y is moved to some offset
    // Layers 1, 2, 3 use global alpha and blended
    layer[cur_fb].startx = 32;
    layer[cur_fb].starty = 32;
    nemadc_set_layer(1, &layer[cur_fb]);
    layer[cur_fb].startx = 64;
    layer[cur_fb].starty = 64;
    nemadc_set_layer(2, &layer[cur_fb]);
    layer[cur_fb].startx = 96;
    layer[cur_fb].starty = 96;
    nemadc_set_layer(3, &layer[cur_fb]);
#endif

    cur_fb = (cur_fb+1)%FRAME_BUFFERS;
#ifdef ENABLE_DSI
    dsi_send_frame_single(NEMADC_OUTP_OFF);
#else
    nemadc_send_frame_single();
#endif
#endif
}

static int
init(void)
{
    int ret;

    //Initialize NemaGFX
    ret = nema_init();
    if (ret != 0)
    {
        return ret;
    }
    //Initialize Nema|dc
    ret = nemadc_init();
    if (ret != 0)
    {
        return ret;
    }
#ifdef ENABLE_DSI
    uint8_t ui8LanesNum = 1;
    uint8_t ui8DbiWidth = 8;
    if ( am_hal_dsi_initialize(ui8LanesNum, ui8DbiWidth) != 0 )
    {
        return -3;
    }
#endif

#ifdef ENABLE_SPI4
    uint16_t panel_resx = 390; //panel's max resolution
    uint16_t panel_resy = 390; //panel's max resolution
#endif
#ifdef ENABLE_QSPI
    uint16_t panel_resx = 360; //panel's max resolution
    uint16_t panel_resy = 360; //panel's max resolution
#endif
#ifdef ENABLE_DSI
    uint16_t panel_resx = 390; //panel's max resolution
    uint16_t panel_resy = 390; //panel's max resolution
#endif
    uint16_t minx, miny;

    //Set the display region to center
    if ( RESX > panel_resx )
    {
        minx = 0;   // set the minimum value to 0
    }
    else
    {
        minx = (panel_resx - RESX) / 2;
    }

    if ( RESY > panel_resy )
    {
        miny = 0;   // set the minimum value to 0
    }
    else
    {
        miny = (panel_resy - RESY) / 2;
    }

    //Initialize the display
#ifdef ENABLE_SPI4
    am_devices_nemadc_rm67162_init(MIPICFG_SPI4, MIPICFG_1RGB565_OPT0, /*MIPICFG_1RGB332_OPT0,*/ RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_QSPI
    am_devices_nemadc_rm67162_init( MIPICFG_QSPI | MIPICFG_SPI4, MIPICFG_4RGB565_OPT0, /*MIPICFG_1RGB332_OPT0,*/ RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_DSI
    am_devices_dsi_rm67162_init(MIPICFG_8RGB888_OPT0, RESX, RESY, minx, miny);
#endif

    load_objects();

    //Create Command Lists
    cl  = nema_cl_create();

    //Bind Command List
    nema_cl_bind(&cl);

    //Set Clipping Rectangle
    nema_set_clip(0, 0, RESX, RESY);

    nema_bind_tex(NEMA_TEX1, screen0.bo.base_phys,  \
        screen0.w, screen0.h, (nema_tex_format_t)(screen0.format), screen0.stride, (nema_tex_mode_t)(NEMA_FILTER_BL | NEMA_TEX_BORDER));
    nema_bind_tex(NEMA_TEX2, screen1.bo.base_phys,  \
        screen1.w, screen1.h, (nema_tex_format_t)(screen1.format), screen1.stride, (nema_tex_mode_t)(NEMA_FILTER_BL | NEMA_TEX_BORDER));

    nema_set_tex_color(0);

    nema_cl_submit(&cl);
    nema_cl_wait(&cl);

    return 0;
}

static nema_transition_t effect = NEMA_TRANS_LINEAR_H;

static void display(float step)
{
   // printf("Display: %f\n", step);

    nema_cl_rewind(&cl);
    //Bind Framebuffer
    nema_bind_dst_tex(layer[cur_fb].baseaddr_phys, fb[0].w, fb[0].h, (nema_tex_format_t)(fb[0].format), fb[0].stride);

#if 0
    nema_set_blend(NEMA_BL_SIMPLE, NEMA_TEX0, NEMA_TEX1, NEMA_NOTEX);
    static int off_x = -160;
    nema_blit(off_x, 0);

#else
    //nema_bind_src_tex(screen0.bo.base_phys,
    //    screen0.w, screen0.h, screen0.format, screen0.stride, NEMA_FILTER_BL | NEMA_TEX_BORDER);
    //nema_transition(NEMA_TRANS_LINEAR_H, NEMA_TEX1, NEMA_TEX1, NEMA_BL_SRC, step, fb[0].w, fb[0].h);
    nema_transition(effect, NEMA_TEX1, NEMA_TEX2, NEMA_BL_SRC, step, fb[0].w, fb[0].h);
#endif
    nema_cl_submit(&cl);
    (void)nema_cl_wait(&cl);

    swap_fb();
}

#define ANIMATION_STEP_0_1 0.02f

static void next_effect(void)
{
    effect = (nema_transition_t)(((int)effect + 1) % NEMA_TRANS_MAX);
    // printf("%s\n", transition_str[effect]);
}

static void loop(void)
{

    float step = 0.f;
    float step_step = ANIMATION_STEP_0_1;

    while (1)
    {
        display(step);

        if (step <= 0.f)
        {
            step = 0.f;
            step_step = ANIMATION_STEP_0_1;
            next_effect();
        }
        else if (step >= 1.f)
        {
            step = 1.f;
            step_step = -ANIMATION_STEP_0_1;
            next_effect();
        }

        step += step_step;

        if (step > 0.30f)
        {
            //  step = 0.f;
        }
        nema_calculate_fps();
    }
}

int transition_effects(void)
{
    int ret = init();
    if (ret)
    {
        return ret;
    }

    loop();

    nema_cl_destroy(&cl);

    return 0;
}


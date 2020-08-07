//*****************************************************************************
//
//! @file main.c
//!
//! @brief NemaGFX example.
//! In computer graphics, a color gradient specifies a range of position-dependent
//! colors, usually used to fill a region. For example, many window managers
//! allow the screen background to be specified as a gradient. The colors
//! produced by a gradient vary continuously with the position, producing smooth
//! color transitions.

//!
//! AM_DEBUG_PRINTF
//! If enabled, debug messages will be sent over ITM.
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
#include "am_bsp.h"
#include "nema_core.h"
#include "nema_utils.h"
#include "am_hal_global.h"
#include "string.h"

#ifndef DONT_USE_NEMADC
#include "nema_dc.h"
#include "nema_dc_mipi.h"
#endif

#ifdef ENABLE_DSI
    #include "am_devices_dsi_rm67162.h"
#else
    #include "am_devices_nemadc_rm67162.h"
#endif
#include "nema_programHW.h"

static const unsigned long OWHITE    = 0xFFFFFFFF;
static const unsigned long BLACK     = 0x00000000;

#define OPAQUE(color)         ((color) | 0xFF000000 )
#define TRANSP(alpha, color)  ((color & 0x00FFFFFF) | ((alpha & 0xff) << 24) )

#define RESX 256
#define RESY 256

AM_SHARED_RW nema_cmdlist_t cl0;
AM_SHARED_RW nema_cmdlist_t context_cl;
AM_SHARED_RW nema_cmdlist_t clearblack_cl;
AM_SHARED_RW nema_cmdlist_t clearwhite_cl;


nema_buffer_t fb_bo  = {0};
nema_buffer_t zbuffer_bo  = {0};

nemadc_layer_t dc_layer = {(void *)0, 0, RESX, RESY, RESX*2, 0, 0, RESX, RESY, 0xff, NEMADC_BL_SRC, 0, NEMADC_RGB565, 0, 0, 0, 0, 0};

void suite_init(void)
{
    fb_bo = nema_buffer_create(RESX*RESY*4);
    nema_buffer_map(&fb_bo);

    dc_layer.baseaddr_phys = fb_bo.base_phys;
    dc_layer.baseaddr_virt = fb_bo.base_virt;
    memset(dc_layer.baseaddr_virt, 0, RESX*RESY*4);

    zbuffer_bo = nema_buffer_create(RESX*RESY*4);
    nema_buffer_map(&zbuffer_bo);

    context_cl = nema_cl_create();
    nema_cl_bind(&context_cl);
    nema_set_const_reg(0, 0);
    nema_enable_gradient(0);
    nema_enable_depth(0);


    nema_bind_dst_tex(fb_bo.base_phys, RESX, RESY, NEMA_RGB565, -1);
    nema_bind_depth_buffer(zbuffer_bo.base_phys, RESX, RESY);

    nema_set_clip(0, 0, RESX, RESY);

    nema_cl_unbind();

    //Clear Black CL
    //---------------------------------------------------------------------
    clearblack_cl = nema_cl_create_sized(256);
    nema_cl_bind(&clearblack_cl);
    nema_cl_branch(&context_cl);
    nema_set_matrix_translate(0, 0);
    nema_clear(BLACK);
    nema_cl_unbind();

    //Clear White CL
    //---------------------------------------------------------------------
    clearwhite_cl = nema_cl_create_sized(256);
    nema_cl_bind(&clearwhite_cl);
    nema_cl_branch(&context_cl);
    nema_set_matrix_translate(0, 0);
    nema_clear(OWHITE);
    nema_cl_unbind();
}

int fillrect_grad()
{
    int x1, y1;
    color_var_t col0, col1, col2;

    //---------------------------------------------------------------------

    cl0 = nema_cl_create();
    //---------------------------------------------------------------------
    nema_cl_bind(&cl0);
    nema_cl_branch(&context_cl);
    //---------------------------------------------------------------------

    nema_enable_gradient(1);

    nema_set_blend_fill ( NEMA_BL_SRC );
    // Col 1
    // NEMA_P_RAST_EMULATION;
    x1 = 10; y1 = 10;
    col0.r = 0; col0.g = 0; col0.b = 0; col0.a = 0;
    col1.r = 400; col1.g = 400; col1.b = 400; col1.a = 400; // x gradient
    col2.r = 100; col2.g = 100; col2.b = 100; col2.a = 100; // y gradient
    nema_interpolate_rect_colors(x1, y1, RESX, RESY, &col0, &col1, &col2);
    nema_fill_rect(x1, y1, RESX, RESY, 0);

    nema_cl_submit(&cl0);
    nema_cl_wait(&cl0);
    nemadc_set_layer(0, &dc_layer);
#ifdef ENABLE_DSI
    dsi_send_frame_single(NEMADC_OUTP_OFF);
#else
    nemadc_send_frame_single();
#endif

    nema_enable_gradient(0);

    nema_cl_unbind();
    nema_cl_destroy(&cl0);

    return 0;
}

int suite_run(void)
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
        minx = 0; // set the minimum value to 0
    }
    else
    {
        minx = (panel_resx - RESX) / 2;
    }

    if ( RESY > panel_resy )
    {
        miny = 0; // set the minimum value to 0
    }
    else
    {
        miny = (panel_resy - RESY) / 2;
    }

#ifdef ENABLE_SPI4
    am_devices_nemadc_rm67162_init(MIPICFG_SPI4, MIPICFG_1RGB565_OPT0, RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_QSPI
    am_devices_nemadc_rm67162_init(MIPICFG_QSPI | MIPICFG_SPI4, MIPICFG_4RGB565_OPT0, RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_DSI
    am_devices_dsi_rm67162_init(MIPICFG_8RGB888_OPT0, RESX, RESY, minx, miny);
#endif

    suite_init();

    fillrect_grad();

    return 0;
}


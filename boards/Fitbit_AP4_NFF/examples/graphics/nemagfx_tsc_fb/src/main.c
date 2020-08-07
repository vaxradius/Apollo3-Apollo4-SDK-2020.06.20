//*****************************************************************************
//
//! @file main.c
//!
//! @brief NemaGFX example.
//! Nemagfx_tsc_fb is a demo of TSC frame-buffer compression. The program uses
//! TSC4-compressed frame-buffer during run-time. It saves frame-buffer space
//! in RAM in a scale of 1:4 also.The demo use example NEMADC_TSC4 frame buffer
//! shows a 400x400 TSC4 image on the screen, it will significantly save RAM use.
//! Note:  the width and height of the frame-buffer should be 4-pixels aligned

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

#ifndef DONT_USE_NEMADC
#include "nema_dc.h"
#include "nema_dc_mipi.h"
#endif

#include "oli_400x400_tsc4.h"
#ifdef ENABLE_DSI
    #include "am_devices_dsi_rm67162.h"
#else
    #include "am_devices_nemadc_rm67162.h"
#endif

#define RESX 390
#define RESY 390

static img_obj_t fb = {{0}, RESX, RESY, -1, 0, NEMA_TSC4, 0};
img_obj_t TSi_logo = {{0}, 400, 400, -1, 0, NEMA_TSC4, 0};

#ifndef DONT_USE_NEMADC
nemadc_layer_t dc_layer = {(void *)0, 0, RESX, RESY, -1, 0, 0, RESX, RESY, 0xff, NEMADC_BL_SRC, 0, NEMADC_TSC4, 0, 0, 0, 0, 0};
#endif

void
load_objects(void)
{
    //Load memory objects
    fb.bo = nema_buffer_create(fb.stride*fb.h);
    nema_buffer_map(&fb.bo);
    printf("FB: V:%p P:0x%08x\n", (void *)fb.bo.base_virt, fb.bo.base_phys);

#ifndef DONT_USE_NEMADC
    dc_layer.baseaddr_phys = fb.bo.base_phys;
    dc_layer.baseaddr_virt = fb.bo.base_virt;
#endif

    printf("FB: V:%p P:0x%08x\n", (void *)fb.bo.base_virt, fb.bo.base_phys);

    TSi_logo.bo.base_phys = (uintptr_t)(oli_400x400_tsc4);
    TSi_logo.bo.base_virt = (void*)(TSi_logo.bo.base_phys);
}

AM_SHARED_RW nema_cmdlist_t cl;
int tsc4_image_show()
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

    /*     MIPICFG_1RGB332_OPT0 */
    /*     MIPICFG_8RGB565_OPT0 */
    /*     MIPICFG_8RGB888_OPT0 */

#ifdef ENABLE_SPI4
    am_devices_nemadc_rm67162_init(MIPICFG_SPI4, MIPICFG_1RGB565_OPT0, RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_QSPI
    am_devices_nemadc_rm67162_init( MIPICFG_QSPI | MIPICFG_SPI4, MIPICFG_4RGB565_OPT0, RESX, RESY, minx, miny);
#endif
#ifdef ENABLE_DSI
    am_devices_dsi_rm67162_init(MIPICFG_8RGB888_OPT0, RESX, RESY, minx, miny);
#endif

    load_objects();

    cl = nema_cl_create();

    nema_cl_bind(&cl);

    nema_bind_dst_tex(fb.bo.base_phys, fb.w, fb.h, (nema_tex_format_t)(fb.format), fb.stride);
    nema_bind_src_tex(TSi_logo.bo.base_phys, TSi_logo.w, TSi_logo.h, (nema_tex_format_t)(TSi_logo.format), -1, NEMA_FILTER_BL);

    nema_set_clip(0, 0, RESX, RESY);
    nema_set_blend_blit(NEMA_BL_SRC);

    nema_blit(0, 0);
    nema_set_blend_fill(NEMA_BL_SRC);

    nema_cl_unbind();
    nema_cl_submit(&cl);
    nemadc_set_layer(0, &dc_layer);
#ifdef ENABLE_DSI
    dsi_send_frame_single(NEMADC_OUTP_OFF);
#else
    nemadc_send_frame_single();
#endif
    nema_cl_wait(&cl);

    nema_cl_destroy(&cl);

    return 0;
}


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

#ifndef TSI_MALLOC_H__
#define TSI_MALLOC_H__

#define tsi_malloc_init(base_virt, base_phys, size, reset) \
		tsi_malloc_init_pool(0, base_virt, base_phys, size, reset)

#define tsi_malloc(size) tsi_malloc_pool(0, size)

int   tsi_malloc_init_pool(	int pool,
						   	void *base_virt,
						   	uintptr_t base_phys,
						   	int size,
						   	int reset);
void *tsi_malloc_pool(int pool, int size);
void  tsi_free(void *ptr);
uintptr_t tsi_virt2phys(void *addr);

#endif

//*****************************************************************************
//
//  am_hal_security.h
//! @file
//!
//! @brief Functions for security functions
//!
//! @addtogroup
//! @ingroup apollo3hal
//! @{
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

#ifndef AM_HAL_SECURITY_H
#define AM_HAL_SECURITY_H

//
// Location of SBL install address for this device
//
// Important Note:
//  Some caution should be observed when using AM_HAL_SBL_ADDRESS as an address.
//   GCC considers use of this address to be a NULL pointer.  When compiled with
//   high optimization (-O3) and used to read the location with, for example,
//   code such as *((volatile uint32_t *)(AM_HAL_SBL_ADDRESS)), GCC will insert
//   an instruction it calls "UDF" (undefined), op-code 0xDEFF, which will cause
//   a fault on execution to trap the "invalid" null-ptr usage.
//  This does not appear to be an issue with IAR and Keil ARM5.
//  It is likely an issue with Keil ARM6.
//
#define AM_HAL_SBL_ADDRESS                  0x00000000


#if 0
// TODO
// Need to locate the size definitions elsewhere
// TODO
// Need to update macros for all internal SRAM
//*****************************************************************************
//
// Some helpful SRAM values and macros.
//
//*****************************************************************************
#define AM_HAL_SRAM_SIZE                  (384 * 1024) // TODO
#define AM_HAL_SRAM_LARGEST_VALID_ADDR    (SRAM_BASEADDR + AM_HAL_FLASH_SRAM_SIZE - 1)

//*****************************************************************************
//
// Some helpful flash values and macros.
//
//*****************************************************************************
#define AM_HAL_MRAM_SIZE                   ( 1024 * 1024 )
#define AM_HAL_MRAM_LARGEST_VALID_ADDR     ( MRAM_BASEADDR + AM_HAL_MRAM_SIZE - 1 )
//
// Macros to determine whether a given address is a valid internal
// flash or SRAM address.
//
#define ISADDRSRAM(x)       ((x >= SRAM_BASEADDR)  &&      \
                             (x <= (AM_HAL_SRAM_LARGEST_VALID_ADDR & ~0x3)))
#if FLASH_BASEADDR == 0x0
#define ISADDRFLASH(x)      (x <= (AM_HAL_MRAM_LARGEST_VALID_ADDR & ~0x3))
#else
#define ISADDRFLASH(x)      ((x >= FLASH_BASEADDR)       &&      \
                             (x <= (AM_HAL_MRAM_LARGEST_VALID_ADDR & ~0x3)))
#endif
#endif

typedef struct
{
    bool     bInfo0Valid;
    uint32_t info0Version;
    uint32_t sblVersion;
    uint32_t sblVersionAddInfo;
} am_hal_security_info_t;

// LOCK Definitions
typedef enum
{
    AM_HAL_SECURITY_LOCKTYPE_CUSTOMER       = 0x1,
    AM_HAL_SECURITY_LOCKTYPE_RECOVERY       = 0x9D,
} am_hal_security_locktype_t;

typedef union
{
    uint32_t keyword[4];
    struct
    {
        uint32_t key0;
        uint32_t key1;
        uint32_t key2;
        uint32_t key3;
    } keys;
} am_hal_security_128bkey_t;

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! @brief  Get Device Security Info
//!
//! @param  pSecInfo -  Pointer to structure for returned security info
//!
//! This will retrieve the security information for the device
//!
//! @return Returns AM_HAL_STATUS_SUCCESS on success
//
//*****************************************************************************
uint32_t am_hal_security_get_info(am_hal_security_info_t *pSecInfo);

//*****************************************************************************
//
//! @brief  Set the key for specified lock
//!
//! @param  lockType - The lock type to be operated upon
//! @param  pKey -  Pointer to 128b key value
//!
//! This will program the lock registers for the specified lock and key
//!
//! @return Returns AM_HAL_STATUS_SUCCESS on success
//
//*****************************************************************************
uint32_t am_hal_security_set_key(am_hal_security_locktype_t lockType, am_hal_security_128bkey_t *pKey);

//*****************************************************************************
//
//! @brief  Get the current status of the specified lock
//!
//! @param  lockType - The lock type to be operated upon
//! @param  pbUnlockStatus -  Pointer to return variable with lock status
//!
//! This will get the lock status for specified lock
//! Note that except for customer lock, other locks are self-locking on status read
//!
//! @return Returns AM_HAL_STATUS_SUCCESS on success
//
//*****************************************************************************
uint32_t am_hal_security_get_lock_status(am_hal_security_locktype_t lockType, bool *pbUnlockStatus);

//*****************************************************************************
//
//! @brief  Compute CRC32 for a specified payload
//!
//! @param  startAddr - The start address of the payload
//! @param  sizeBytes - The length of payload in bytes
//! @param  pCrc -  Pointer to return computed CRC
//!
//! This will use the hardware engine to compute CRC32 on an arbitrary data payload
//!
//! @return Returns AM_HAL_STATUS_SUCCESS on success
//
//*****************************************************************************
uint32_t am_hal_crc32(uint32_t startAddr, uint32_t sizeBytes, uint32_t *pCrc);

#ifdef __cplusplus
}
#endif

#endif // AM_HAL_SECURITY_H

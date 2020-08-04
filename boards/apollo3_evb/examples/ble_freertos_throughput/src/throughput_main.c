// ****************************************************************************
//
//  throughput_main.c
//! @file
//!
//! @brief Ambiq Micro's demonstration of throughput example.
//!
//! @{
//
// ****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2019, Ambiq Micro
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
// This is part of revision v2.2.0-6-g9329ccc58 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include <stdbool.h>

#include <string.h>
#include "wsf_types.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "app_api.h"
#include "app_ui.h"
#include "svc_core.h"
#include "throughput_api.h"
#include "svc_throughput.h"
#include "hci_api.h"
#include "am_util.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/*! WSF message event starting value */
#define THROUGHPUT_MSG_START               0xA0
#define READ_REM_FEA_TIMEOUT_MS      (2000)


/*! WSF message event enumeration */
enum
{
    THROUGHPUT_SEND_DATA_TIMER_IND = THROUGHPUT_MSG_START,  /*! data sending timer expired */
    READ_REM_FEATRUE_REQ_EVT
};

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! Application message type */
typedef union
{
    wsfMsgHdr_t     hdr;
    dmEvt_t         dm;
    attsCccEvt_t    ccc;
    attEvt_t        att;
} tpMsg_t;


/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for advertising */
static const appAdvCfg_t tpAdvCfg =
{
    {    0,     0,     0},                  /*! Advertising durations in ms */
    {  800,   800,     0}                   /*! Advertising intervals in 0.625 ms units */
};

/*! configurable parameters for slave */
static const appSlaveCfg_t tpSlaveCfg =
{
    THROUGHPUT_CONN_MAX,                           /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t tpSecCfg =
{
    0,//DM_AUTH_BOND_FLAG,                      /*! Authentication and bonding flags */
    0,                                      /*! Initiator key distribution flags */
    DM_KEY_DIST_LTK,                        /*! Responder key distribution flags */
    FALSE,                                  /*! TRUE if Out-of-band pairing data is present */
    FALSE                                   /*! TRUE to initiate security upon connection */
};

/*! configurable parameters for throughput connection parameter update */
static appUpdateCfg_t tpUpdateCfg =
{
    0,                                /*! Connection idle period in ms before attempting
                                              connection parameter update; set to zero to disable */
    (7.5 / 1.25),                         /*! 15 ms */
    (15 / 1.25),                         /*! 30 ms */
    4,                                   /*! Connection latency */
    (6000 / 10),                         /*! Supervision timeout in 10ms units */
    5                                    /*! Number of update attempts before giving up */
};


/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
uint8_t tpAdvDataDisc[] =
{
    /*! flags */
    2,                                      /*! length */
    DM_ADV_TYPE_FLAGS,                      /*! AD type */
    DM_FLAG_LE_GENERAL_DISC |               /*! flags */
    DM_FLAG_LE_BREDR_NOT_SUP,


    /*! device name */
    15,                                     /*! length */
    DM_ADV_TYPE_LOCAL_NAME,                 /*! AD type */
    'a',
    'm',
    'b',
    'i',
    'q',
    '-',
    'B',
    'L',
    'E',
    '-',
    't',
    'e',
    's',
    't',

    /*! service UUID list */
    3,                                      /*! length */
    DM_ADV_TYPE_16_UUID,                    /*! AD type */
    UINT16_TO_BYTES(ATT_UUID_DEVICE_INFO_SERVICE)

};

/*! scan data, discoverable mode */
static const uint8_t tpScanDataDisc[] =
{
    /*! tx power */
    2,                                      /*! length */
    DM_ADV_TYPE_TX_POWER,                   /*! AD type */
    0,                                      /*! tx power */

};

/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/

/*! enumeration of client characteristic configuration descriptors */
enum
{
    THROUGHPUT_GATT_SC_CCC_IDX,             /*! GATT service, service changed characteristic */
    THROUGHPUT_TX_CCC_IDX,                  /*! throughput service, tx characteristic */
    THROUGHPUT_NUM_CCC_IDX
};

/*! client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t tpCccSet[THROUGHPUT_NUM_CCC_IDX] =
{
    /* cccd handle          value range               security level */
    {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},   /* THROUGHPUT_GATT_SC_CCC_IDX */
    {THROUGHPUT_TX_CH_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE}    /* THROUGHPUT_TX_CCC_IDX */
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
#define TX_PACKET_CNT_MAX    65535
#define TX_DATA_TIMER_MS     100
#define UPLINK_DATA_CNT      512
#define BIDIRCT_PKT_CNT_MAX  601
#define UPLINK_FLAG          0xAA
#define DOWNLINK_FLAG        0x5A
#define MTU_REQ_FLAG         0xFE
#define BIDIRECT_FLAG        0x88

/* Control block */
static struct
{
    dmConnId_t              connId;
    wsfHandlerId_t          handlerId;
    wsfTimer_t              sendDataTimer;  // timer for sending data
}throughputCb;

bool        sendDataFlag = false;
uint8_t     uplinkData[UPLINK_DATA_CNT] = {0, 0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
uint16_t    txIdx = 0;
uint16_t    preRxIdx=0, rxIdx = 0;
uint16_t    maxTransPacket = TX_PACKET_CNT_MAX;
uint8_t     g_mtu;

/*************************************************************************************************/
/*!
 *  \fn     throughputDmCback
 *
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputDmCback(dmEvt_t *pDmEvt)
{
    dmEvt_t *pMsg;

    if ((pMsg = WsfMsgAlloc(sizeof(dmEvt_t))) != NULL)
    {
        memcpy(pMsg, pDmEvt, sizeof(dmEvt_t));
        WsfMsgSend(throughputCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     throughputAttCback
 *
 *  \brief  Application ATT callback.
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputAttCback(attEvt_t *pEvt)
{
    attEvt_t *pMsg;

    if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
    {
        memcpy(pMsg, pEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t *) (pMsg + 1);
        memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
        WsfMsgSend(throughputCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     throughputCccCback
 *
 *  \brief  Application ATTS client characteristic configuration callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
 static void throughputCccCback(attsCccEvt_t *pEvt)
{
    attsCccEvt_t  *pMsg;
    appDbHdl_t    dbHdl;

    /* if CCC not set from initialization and there's a device record */
    if ((pEvt->handle != ATT_HANDLE_NONE) &&
        ((dbHdl = AppDbGetHdl((dmConnId_t) pEvt->hdr.param)) != APP_DB_HDL_NONE))
    {
        /* store value in device database */
        AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
    }

    if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
    {
        memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
        WsfMsgSend(throughputCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     throughputProcCccState
 *
 *  \brief  Process CCC state change.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputProcCccState(tpMsg_t *pMsg)
{
    APP_TRACE_INFO3("ccc state ind value=%d,handle=%d,idx=%d", pMsg->ccc.value, pMsg->ccc.handle, pMsg->ccc.idx);

    if (pMsg->ccc.idx == THROUGHPUT_TX_CCC_IDX)
    {
        if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
        {
            // notify enabled
            throughputCb.connId = (dmConnId_t) pMsg->ccc.hdr.param;
        }
        else
        {
            // notify disabled
            throughputCb.connId = DM_CONN_ID_NONE;
        }
        return;
    }
}

/*************************************************************************************************/
/*!
 *  \fn     throughputSetup
 *
 *  \brief  Set up advertising and other procedures that need to be performed after
 *          device reset.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputSetup(tpMsg_t *pMsg)
{
    /* set advertising and scan response data for discoverable mode */
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(tpAdvDataDisc), (uint8_t *) tpAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(tpScanDataDisc), (uint8_t *) tpScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, sizeof(tpAdvDataDisc), (uint8_t *) tpAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(tpScanDataDisc), (uint8_t *) tpScanDataDisc);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    AppAdvStart(APP_MODE_AUTO_INIT);
}

/*************************************************************************************************/
/*!
 *  \fn     throughputBtnCback
 *
 *  \brief  Button press callback.
 *
 *  \param  btn    Button press.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputBtnCback(uint8_t btn)
{
    dmConnId_t      connId = AppConnIsOpen();

    /* button actions when connected */
    if (connId != DM_CONN_ID_NONE)
    {
        switch (btn)
        {
            case APP_UI_BTN_1_SHORT:

            break;


            case APP_UI_BTN_1_MED:
            break;

            case APP_UI_BTN_1_LONG:
            break;

            case APP_UI_BTN_2_SHORT:
            break;

            default:
            break;
        }
    }
}

static void throughputSendData(uint8_t connId)
{
    uplinkData[0] = txIdx & 0xff;
    uplinkData[1] = (txIdx >> 8) & 0xff;

    if ((sendDataFlag)&&(txIdx < maxTransPacket))
    {
        AttsHandleValueNtf(connId, THROUGHPUT_TX_HDL, g_mtu-3, uplinkData);

        /*Device stops sending packages while device sends 65535 packages.*/
        if (txIdx >= TX_PACKET_CNT_MAX)
        {
            APP_TRACE_INFO0("Device sends 65535 packages done");
            txIdx = 0;
        }

        if( ((txIdx & 0x3ff) == 0) && (txIdx>>10 != 0))
        {
            APP_TRACE_INFO1("Device tx %d packages", txIdx);
        }

        txIdx++;
    }
}


void sendDataTimerExpired(uint8_t connId)
{
    throughputSendData(connId);
}

/*************************************************************************************************/
/*!
 *  \fn     throughputProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void throughputProcMsg(tpMsg_t *pMsg)
{
    uint8_t uiEvent = APP_UI_NONE;

    switch(pMsg->hdr.event)
    {
        case THROUGHPUT_SEND_DATA_TIMER_IND:
            sendDataTimerExpired(throughputCb.connId);
        break;

        case ATTS_HANDLE_VALUE_CNF:
        if (pMsg->hdr.status == ATT_SUCCESS)
        {
            /*send next package afer last package has been sent done*/
            throughputSendData((dmConnId_t) pMsg->hdr.param);
        }
        else
        {
            // APP_TRACE_INFO1("ATTS_HANDLE_VALUE_CNF status %d", pMsg->hdr.status);
        }
        break;

        case ATTS_CCC_STATE_IND:
            throughputProcCccState(pMsg);
        break;

        case ATT_MTU_UPDATE_IND:
            APP_TRACE_INFO1("Negotiated MTU = %d", ((attEvt_t *)pMsg)->mtu);
            g_mtu=((attEvt_t *)pMsg)->mtu;

        break;

        case DM_RESET_CMPL_IND:
            AttsCalculateDbHash();
            DmSecGenerateEccKeyReq();

            uiEvent = APP_UI_RESET_CMPL;
        break;

        case DM_ADV_SET_START_IND:
          uiEvent = APP_UI_ADV_SET_START_IND;
          break;

        case DM_ADV_START_IND:
            uiEvent = APP_UI_ADV_START;
        break;

        case DM_ADV_STOP_IND:
            uiEvent = APP_UI_ADV_STOP;
        break;

        case DM_CONN_OPEN_IND:
            txIdx = 0;
            rxIdx = 0;
            preRxIdx = 0;
            sendDataFlag = false;

            g_mtu=AttGetMtu(1);
            DmReadRemoteFeatures(1);

            uiEvent = APP_UI_CONN_OPEN;

        break;

        case DM_REMOTE_FEATURES_IND:
        {
            hciEvt_t *pEvent = (hciEvt_t *)pMsg;
            uint8_t data_len_ext = pEvent->leReadRemoteFeatCmpl.features[0]&HCI_LE_SUP_FEAT_DATA_LEN_EXT;
            APP_TRACE_INFO2("\r\nDM_REMOTE_FEATURES_IND remote feature: 0x%x  support DLE:%x", pEvent->leReadRemoteFeatCmpl.features[0],
            data_len_ext);
            if(data_len_ext>0)
            {
                DmConnSetDataLen(1, 251, 0x848);
            }
            else
            {
                APP_TRACE_INFO0("\r\nDM_REMOTE_FEATURES_IND:  Can not support data length extend.");
            }
        }
        break;

       case DM_CONN_DATA_LEN_CHANGE_IND:
            APP_TRACE_INFO3("DM_CONN_DATA_LEN_CHANGE_IND:status= %d, max RX len= %d, max TX len= %d", pMsg->dm.dataLenChange.hdr.status, pMsg->dm.dataLenChange.maxRxOctets, pMsg->dm.dataLenChange.maxTxOctets);
            //set Mtu to 247
            AttcMtuReq(1, 247);
            break;

        case DM_CONN_CLOSE_IND:
        {
            hciDisconnectCmplEvt_t *evt = (hciDisconnectCmplEvt_t*) pMsg;


            throughputCb.connId = DM_CONN_ID_NONE;

            APP_TRACE_INFO1(">>> Connection closed reason 0x%x <<<", evt->reason);

            uiEvent = APP_UI_CONN_CLOSE;

        }
        break;

        case DM_PHY_UPDATE_IND:
          APP_TRACE_INFO2("DM_PHY_UPDATE_IND RX: %d, TX: %d", pMsg->dm.phyUpdate.rxPhy, pMsg->dm.phyUpdate.txPhy);
          break;

        case DM_CONN_UPDATE_IND:
        {
            hciLeConnUpdateCmplEvt_t *evt = (hciLeConnUpdateCmplEvt_t*) pMsg;

            (void)evt;

            APP_TRACE_INFO1("connection update status = 0x%x", evt->status);
            APP_TRACE_INFO1("handle = 0x%x", evt->handle);
            APP_TRACE_INFO1("connInterval = 0x%x", evt->connInterval);
            APP_TRACE_INFO1("connLatency = 0x%x", evt->connLatency);
            APP_TRACE_INFO1("supTimeout = 0x%x", evt->supTimeout);
        }
        break;

        case DM_SEC_PAIR_CMPL_IND:
            uiEvent = APP_UI_SEC_PAIR_CMPL;
        break;

        case DM_SEC_PAIR_FAIL_IND:
            uiEvent = APP_UI_SEC_PAIR_FAIL;
        break;

        case DM_SEC_ENCRYPT_IND:
            uiEvent = APP_UI_SEC_ENCRYPT;
        break;

        case DM_SEC_ENCRYPT_FAIL_IND:
            uiEvent = APP_UI_SEC_ENCRYPT_FAIL;
        break;

        case DM_SEC_AUTH_REQ_IND:
            AppHandlePasskey(&pMsg->dm.authReq);
        break;

        case DM_SEC_ECC_KEY_IND:
            DmSecSetEccKey(&pMsg->dm.eccMsg.data.key);
            throughputSetup(pMsg);
        default:
        break;
    }

    if (uiEvent != APP_UI_NONE)
    {
        AppUiAction(uiEvent);
    }
}


uint8_t throughputWriteCb(dmConnId_t connId, uint16_t handle, uint8_t operation,
                                    uint16_t offset, uint16_t len, uint8_t *pValue,
                                    attsAttr_t *pAttr)
{
    if (len >= 1)
    {
        rxIdx = (pValue[1]<<8) | pValue[0];

        // check if the Packet is an command packet to start uplink/downlink/bidrection test.
        if(rxIdx == 1)
        {
            if(pValue[3] == DOWNLINK_FLAG)
            {
                //starting trans of downlink
                txIdx = 0;
                preRxIdx = 1;
                maxTransPacket = 0;
                sendDataFlag = false;

                APP_TRACE_INFO0("Got downlink trans command  and starting downlink trans\r\n");
                return ATT_SUCCESS;
            }
            else if(pValue[3] == MTU_REQ_FLAG)
            {
                //send MTU to APP

                sendDataFlag = false;
                txIdx = 0;
                preRxIdx = 0;
                maxTransPacket = 0;

                memset(uplinkData, MTU_REQ_FLAG, UPLINK_DATA_CNT);
                uplinkData[6] = g_mtu;
                uplinkData[7] = g_mtu;
                uplinkData[8] = g_mtu;

                sendDataFlag = true;
                maxTransPacket = 1;

                am_util_delay_ms(50);
                throughputSendData(connId);

                sendDataFlag = false;
                txIdx = 0;
                preRxIdx = 0;

                APP_TRACE_INFO0("Got MTU cmd from APP, send Local MTU to APP");
            }
            else if((pValue[3] == BIDIRECT_FLAG))
            {
                //starting trans of bidrection

                txIdx = 1;
                preRxIdx=0;
                sendDataFlag = true;
                maxTransPacket = BIDIRCT_PKT_CNT_MAX;

                memset(uplinkData, UPLINK_FLAG, UPLINK_DATA_CNT);

                WsfTimerStartMs(&throughputCb.sendDataTimer, TX_DATA_TIMER_MS);
                APP_TRACE_INFO0("Got bidirection trans cmd and start bidrection trans\r\n");
            }
            else
            {
                //starting  trans of uplink

                txIdx = 1;
                preRxIdx = 0;
                sendDataFlag = true;
                maxTransPacket = TX_PACKET_CNT_MAX;

                memset(uplinkData, UPLINK_FLAG, 256);

                WsfTimerStartMs(&throughputCb.sendDataTimer, TX_DATA_TIMER_MS);
                APP_TRACE_INFO0("Got uplink trans cmd and start Uplink trans\r\n");
            }
        }
        if (rxIdx > preRxIdx)
        {
            if (rxIdx - preRxIdx == 1)
            {
                // Device stops sending packages while device received 65535 packages.
                if (rxIdx >= TX_PACKET_CNT_MAX)
                {
                    preRxIdx = 0;
                    rxIdx = 0;
                    APP_TRACE_INFO0("Device received 65535 packages done");
                }
            }
            else
            {
                APP_TRACE_INFO2("Device received error package index, last index is %d, now is %d!", preRxIdx, rxIdx);
            }
        }
        else
        {
            APP_TRACE_INFO2("Device received error package index, last index is %d, now is %d!!!", preRxIdx, rxIdx);
        }

        /*update previous package index*/
        preRxIdx = rxIdx;
    }
    else
    {
        APP_TRACE_INFO1("Device received error len, len is %d bytes,it should be more than MTU!!!\r\n", len);
    }

    return ATT_SUCCESS;
}


/*************************************************************************************************/
/*!
 *  \fn     ThroughputHandlerInit
 *
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ThroughputHandlerInit(wsfHandlerId_t handlerId)
{
    APP_TRACE_INFO0("ThroughputHandlerInit");

    throughputCb.handlerId = handlerId;
    throughputCb.connId = DM_CONN_ID_NONE;

    /* Set configuration pointers */
    pAppAdvCfg = (appAdvCfg_t *) &tpAdvCfg;

    throughputCb.sendDataTimer.handlerId = handlerId;
    throughputCb.sendDataTimer.msg.event = THROUGHPUT_SEND_DATA_TIMER_IND;
    pAppSlaveCfg = (appSlaveCfg_t *) &tpSlaveCfg;
    pAppSecCfg = (appSecCfg_t *) &tpSecCfg;
    pAppUpdateCfg = (appUpdateCfg_t *) &tpUpdateCfg;

    /* Initialize application framework */
    AppSlaveInit();
}

/*************************************************************************************************/
/*!
 *  \fn     ThroughputHandler
 *
 *  \brief  WSF event handler for application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ThroughputHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (pMsg != NULL)
    {
        if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
        {
            /* process advertising and connection-related messages */
            AppSlaveProcDmMsg((dmEvt_t *) pMsg);

            /* process security-related messages */
            AppSlaveSecProcDmMsg((dmEvt_t *) pMsg);
        }

        /* perform profile and user interface-related operations */
        throughputProcMsg((tpMsg_t *) pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     ThroughputStart
 *
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ThroughputStart(void)
{
    /* Register for stack callbacks */
    DmRegister(throughputDmCback);
    DmConnRegister(DM_CLIENT_ID_APP, throughputDmCback);
    AttRegister(throughputAttCback);
    AttConnRegister(AppServerConnCback);
    AttsCccRegister(THROUGHPUT_NUM_CCC_IDX, (attsCccSet_t *) tpCccSet, throughputCccCback);

    /* Register for app framework callbacks */
    AppUiBtnRegister(throughputBtnCback);

    /* Initialize attribute server database */
    SvcCoreAddGroup();
    SvcThroughputCbackRegister(NULL, throughputWriteCb);
    SvcThroughputAddGroup();

    /* Reset the device */
    DmDevReset();
}
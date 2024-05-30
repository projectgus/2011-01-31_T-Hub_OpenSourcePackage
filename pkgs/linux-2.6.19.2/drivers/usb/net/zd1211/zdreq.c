/* src/zdreq.c
 *
 *
 *
 * Copyright (C) 2004 ZyDAS Inc.  All Rights Reserved.
 * --------------------------------------------------------------------
 *
 *
 *
 *   The contents of this file are subject to the Mozilla Public
 *   License Version 1.1 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.mozilla.org/MPL/
 *
 *   Software distributed under the License is distributed on an "AS
 *   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *   implied. See the License for the specific language governing
 *   rights and limitations under the License.
 *
 *   Alternatively, the contents of this file may be used under the
 *   terms of the GNU Public License version 2 (the "GPL"), in which
 *   case the provisions of the GPL are applicable instead of the
 *   above.  If you wish to allow the use of your version of this file
 *   only under the terms of the GPL and not to allow others to use
 *   your version of this file under the MPL, indicate your decision
 *   by deleting the provisions above and replace them with the notice
 *   and other provisions required by the GPL.  If you do not delete
 *   the provisions above, a recipient may use your version of this
 *   file under either the MPL or the GPL.
 *
 * -------------------------------------------------------------------- */

#include "zd1205.h"
#include "zdhw.h"
#include "zdhci.h"
#include "zddebug.h"
#include "zdreq.h"
#include "zdsm.h"
#include "zdbuf.h"
#ifdef PRODUCTION
extern zd_80211Obj_t dot11Obj;
extern U8 mMacMode;
extern void zd1205_CollectHwTally(struct zd1205_private *macp);
//extern void zd1205_SoftwareReset(struct zd1205_private *macp);

/* In fact, these global variables should be avoided. The should be
 * removed to the context of adapter.
 */

//extern U8 IntValue[14];
int zdproduction_ioctl(struct zd1205_private *macp, struct zd_point *p)
{
    NDIS_STATUS status;
    oid_wrap_t *param;
    int ret = 0;

    printk(KERN_ERR "zdproduction_ioctl, length: %d\n", p->length);

    if (p->length < sizeof(oid_wrap_t) || !p->pointer)
    {
        printk(KERN_ERR "Something Wrong,%d,%d\n",p->length,sizeof(oid_wrap_t));
        return -EINVAL;
    }

    param = (oid_wrap_t *) kmalloc(p->length, GFP_ATOMIC);

    if (param == NULL)
    {
        printk(KERN_ERR "Buf is NULL\n");
        return -ENOMEM;
    }

    if (copy_from_user(param, p->pointer, p->length))
    {
        printk(KERN_ERR "copy_from_user error\n");
        ret = -EFAULT;
        goto out;
    }

/* Dump the content */
#if 0
    ZDPRODUCTDBG("\r\n");
    {
        int ii;
        u8 *pp = (u8 *) param;

        for(ii = 0; ii < p->length;)
        {
            ZDPRODUCTDBG("0x%02x ", pp[ii]);

            if((++ii % 16) == 0)
                ZDPRODUCTDBG("\r\n");
        }
        ZDPRODUCTDBG("\r\n");
    }
#endif

/* Dump the data structure of UDP request. */
    ZDPRODUCTDBG("request: 0x%04x\n", le16_to_cpu(param->request));
    ZDPRODUCTDBG("seq: 0x%04x\n", le16_to_cpu(param->seq));
    ZDPRODUCTDBG("oid: 0x%08x\n", le32_to_cpu(param->u.info.oid));
    ZDPRODUCTDBG("status: 0x%08x\n", le32_to_cpu(param->u.info.status));
    ZDPRODUCTDBG("length: 0x%08x\n", le32_to_cpu(param->u.info.length));

/* Query command */
    if (le16_to_cpu(param->request) == CMD_QUERY_INFORMATION)
    {
        NDIS_OID Oid = le32_to_cpu(param->u.info.oid);
        PVOID InformationBuffer = param->u.info.data;
        ULONG InformationBufferLength = le32_to_cpu(param->u.info.length);
        ULONG BytesWritten;
        ULONG BytesNeeded;

        status = ZD1205EM_Custom_QueryInformation((PVOID) macp,
            Oid,
            InformationBuffer,
            InformationBufferLength,
            &BytesWritten,
            &BytesNeeded);

        param->u.info.status = cpu_to_le32(status);
        if (status == NDIS_STATUS_SUCCESS)
        {
/* Update information */
            param->u.info.length = cpu_to_le32(BytesWritten);
            p->length = ZD_GENERIC_OID_HDR_LEN + BytesWritten;
            ret = 1;
        }
        else
        {
/* If the status is not NDIS_STATUS_SUCCESS,
   we don't change the length field. */
            p->length = ZD_GENERIC_OID_HDR_LEN + sizeof(ZD_RD_STRUCT);
            ret = 1;
        }
    }
    else if (le16_to_cpu(param->request) == CMD_SET_INFORMATION)
    {
        NDIS_OID Oid = le32_to_cpu(param->u.info.oid);
        PVOID InformationBuffer = param->u.info.data;
        ULONG InformationBufferLength = le32_to_cpu(param->u.info.length);
        ULONG BytesWritten;
        ULONG BytesNeeded;

        status = ZD1205EM_Custom_SetInformation((PVOID) macp,
            Oid,
            InformationBuffer,
            InformationBufferLength,
            &BytesWritten,
            &BytesNeeded);

        param->u.info.status = cpu_to_le32(status);

/* Update information */
        p->length = ZD_GENERIC_OID_HDR_LEN;
        ret = 1;
    }

    if (ret == 1)
    {
        if (copy_to_user(p->pointer, param, p->length))
        {
            ret = -EFAULT;
            goto out;
        }
        else
            ret = 0;
    }

    out:
    kfree(param);
    return ret;
}

#if 0 //Replace by SoftwareReset in zdmain.c
void SoftwareReset(zd1205_private_t *macp)
{
    void *reg = macp->regp;
    U32 tmpvalue;

    acquire_ctrl_of_phy_req(reg);
    dot11Obj.SetReg(reg,ZD1205_PHY_END, 0x8);
    release_ctrl_of_phy_req(reg);

    tmpvalue = dot11Obj.GetReg(reg, PS_Ctrl);
    dot11Obj.SetReg(reg, PS_Ctrl, (tmpvalue | BIT_5));

    acquire_ctrl_of_phy_req(reg);
    dot11Obj.SetReg(reg, ZD1205_PHY_END, 0x0);
    release_ctrl_of_phy_req(reg);

/* Recycle Tx process */
    ZD1205_recycle_tx(macp);

/* Recycle Rx process */
    ZD1205_recycle_rx(macp);
}
#endif


UCHAR Rate_Convert_to_MS(UCHAR rate_in_drv)
{
    UCHAR rate;

    switch (rate_in_drv)
    {
        case RATE_1M:
            rate = 2;                             // 1M
            break;
        case RATE_2M:
            rate = 4;                             // 2M
            break;
        case RATE_5M:
            rate = 11;                            // 5.5M
            break;
        case RATE_11M:
            rate = 22;                            // 11M
            break;
        case RATE_6M:
            rate = 12;                            // 6M
            break;
        case RATE_9M:
            rate = 18;                            // 9M
            break;
        case RATE_12M:
            rate = 24;                            // 12M
            break;
        case RATE_18M:
            rate = 36;                            // 18M
            break;
        case RATE_24M:
            rate = 48;                            // 24M
            break;
        case RATE_36M:
            rate = 72;                            // 36M
            break;
        case RATE_48M:
            rate = 96;                            // 48M
            break;
        case RATE_54M:
            rate = 108;                           // 54M
            break;
        default:
            rate = 108;                           // 54M
            break;
    }

    return rate;
}


UCHAR Rate_Convert(UCHAR rate)
{
    switch(rate)
    {
        case 2:     return 0;                     // 1M
        case 4:     return 1;                     // 2M
        case 11:    return 2;                     // 5.5M
        case 22:    return 3;                     // 11M
        case 12:    return 4;                     // 6M
        case 18:    return 5;                     // 9M
        case 24:    return 6;                     // 12M
        case 36:    return 7;                     // 18M
        case 48:    return 8;                     // 24M
        case 72:    return 9;                     // 36M
        case 96:    return 0xa;                   // 48M
        case 108:   return 0xb;                   // 54M
        default:    return 0xb;                   // 54M
    }
}


void Reset_Tally(zd1205_private_t *macp)
{
    void *reg = macp->regp;

    macp->txUnicastFrm = 0;
    macp->txMulticastFrm = 0;
    macp->txUnicastOctets = 0;
    macp->txMulticastOctets = 0;
    macp->retryFailCnt = 0;
    macp->hwTotalTxFrm = 0;
    macp->hwRetryCnt = 0;
    macp->hwUnderrunCnt = 0;
    macp->DriverRxFrmCnt = 0;
    macp->rxUnicastFrm = 0;
    macp->rxMulticastFrm = 0;
    macp->rxCnt = 0;
    macp->rxUnicastOctets = 0;
    macp->rxMulticastOctets = 0;
    macp->invalid_frame_good_crc = 0;
    macp->ErrShortFrmCnt = 0;
    macp->ErrLongFrmCnt = 0;
    macp->rxBroadcastFrm = 0;
    macp->rxBroadcastOctets = 0;
    macp->rx11bDataFrame = 0;
    macp->rxOFDMDataFrame = 0;
    macp->rxNeedFrag = 0;
    macp->rxMgtFrm = 0;

    macp->hwTotalRxFrm = 0;
    macp->hwCRC16Cnt = 0;
    macp->hwCRC32Cnt = 0;
    macp->hwDecrypErr_UNI = 0;
    macp->hwDecrypErr_Mul = 0;
    macp->hwRxFIFOOverrun = 0;
/* Debug counter */
    macp->rxDiscardByNotPIBSS = 0;
    macp->rxDiscardByAllocateBuf = 0;

/* Read clear those hardware counters */
    dot11Obj.GetReg(reg, TotalRxFrm);
    dot11Obj.GetReg(reg, CRC32Cnt);
    dot11Obj.GetReg(reg, CRC16Cnt);
    dot11Obj.GetReg(reg, DecrypErr_UNI);
    dot11Obj.GetReg(reg, DecrypErr_Mul);
    dot11Obj.GetReg(reg, RxFIFOOverrun);
    dot11Obj.GetReg(reg, TotalTxFrm);
    dot11Obj.GetReg(reg, UnderrunCnt);
    dot11Obj.GetReg(reg, RetryCnt);
}


NDIS_STATUS ZD1205EM_Custom_QueryInformation(
PVOID NDIS_HANDLE,
NDIS_OID Oid,
PVOID InformationBuffer,
ULONG InformationBufferLength,
PULONG BytesWritten,
PULONG BytesNeeded
)
{
    NDIS_STATUS Status;
    ULONG GenericUlong;
    ULONG IoAddress;
//ULONG IoValue;
//PVOID MoveSource = (PVOID) (&GenericUlong);
//ULONG MoveBytes = sizeof(GenericUlong);

    zd1205_private_t *macp = (zd1205_private_t *) NDIS_HANDLE;
    void *reg = macp->regp;

    NDIS_802_11_CONFIGURATION  *pConfiguration;
    ZD_CUSTOM_STRUCT *pZDCustom;
    ZD_RD_STRUCT *pZDRD;
    RID_STRUCT *rid_p;
    LOCAL_TALLY_STRUCT *pLocalTally;

    U8 macmode;
    int i;

    ZDPRODUCTDBG("****** ZD1205EM_Custom_QueryInformation ******\r\n");

/* DUMP InformationBuffer */
#if 0
    {
        int ii;
        UCHAR *p = (UCHAR *) InformationBuffer;

        for(ii = 0; ii < InformationBufferLength;)
        {
            ZDPRODUCTDBG("0x%02x ", p[ii]);

            if ((++ii % 16) == 0)
                ZDPRODUCTDBG("\n");
        }

    }
#endif

    Status = NDIS_STATUS_SUCCESS;

    switch(Oid)
    {
        case OID_GEN_MEDIA_CONNECT_STATUS:
            printk(KERN_ERR "OID_GEN_MEDIA_CONNECT_STATUS\n");
//if (macp->adapterReady == TRUE)
            GenericUlong = cpu_to_le32(NdisMediaStateConnected);
//else
//	GenericUlong = NdisMediaStateDisconnected;

            memcpy(InformationBuffer, &GenericUlong, sizeof(GenericUlong));
            *BytesWritten = sizeof(GenericUlong);
            *BytesNeeded = 0;
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_GEN_DRIVER_VERSION:
            ZDPRODUCTDBG("OID_GEN_DRIVER_VERSION\r\n");

            GenericUlong = cpu_to_le32(ZD1205_DRIVER_VERSION);
            memcpy(InformationBuffer, &GenericUlong, sizeof(GenericUlong));
            *BytesWritten = sizeof(GenericUlong);
            *BytesNeeded = 0;
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_802_11_DESIRED_RATES:
            ZDPRODUCTDBG("OID_802_11_DESIRED_RATES\r\n");

            if (InformationBuffer == 0 || ((UINT)InformationBufferLength < macp->AllowRateArrayCount))
            {
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                *BytesWritten = 0;
                *BytesNeeded = macp->AllowRateArrayCount;
                break;
            }

            ZDPRODUCTDBG("OID_802_11_DESIRED_RATES %d\r\n", macp->cardSetting.FixedRate);

//for(i = 0; i < macp->AllowRateArrayCount; i++){
            *((PUCHAR)InformationBuffer) = Rate_Convert_to_MS(macp->cardSetting.FixedRate);
//}

// need to check
            *BytesWritten = macp->AllowRateArrayCount;
//*BytesWritten = 1;
            *BytesNeeded = 0;
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_802_11_CONFIGURATION:
        case OID_ZDX_802_11_CONFIGURATION:
            ZDPRODUCTDBG("OID_802_11_CONFIGURATION\r\n");
            if (InformationBuffer == NULL ||
                (InformationBufferLength < sizeof(NDIS_802_11_CONFIGURATION)))
            {
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;

                *BytesWritten = 0;
                *BytesNeeded = sizeof(NDIS_802_11_CONFIGURATION);
                break;
            }

            pConfiguration = (PNDIS_802_11_CONFIGURATION)InformationBuffer;
            pConfiguration->Length = cpu_to_le32(sizeof(NDIS_802_11_CONFIGURATION));
            pConfiguration->BeaconPeriod = cpu_to_le32(macp->cardSetting.BeaconInterval);
            pConfiguration->ATIMWindow = cpu_to_le32(macp->cardSetting.ATIMWindow);
            pConfiguration->DSConfig = cpu_to_le32((2412+(macp->cardSetting.Channel-1)*5)*1000);

            if (macp->cardSetting.Channel == 14)
                pConfiguration->DSConfig = cpu_to_le32(2484000);
#ifdef ZDCONF_80211A_SUPPORT
            if(mMacMode == PURE_A_MODE)
                pConfiguration->DSConfig = channel_11A_to_Freq(macp->cardSetting.Channel)*1000;
#endif
            *BytesWritten = sizeof(NDIS_802_11_CONFIGURATION);
            *BytesNeeded = 0;
            return NDIS_STATUS_SUCCESS;

        case OID_802_11_INFRASTRUCTURE_MODE:
        case OID_ZDX_802_11_INFRASTRUCTURE_MODE:
            ZDPRODUCTDBG("OID_802_11_INFRASTRUCTURE_MODE\r\n");

            macmode = macp->cardSetting.BssType;
            switch(macmode)
            {
                case INDEPENDENT_BSS:
                    GenericUlong = cpu_to_le32(Ndis802_11IBSS);
                    break;

                case INFRASTRUCTURE_BSS:
                    GenericUlong = cpu_to_le32(Ndis802_11Infrastructure);
                    break;

                case AP_BSS:
                    GenericUlong = cpu_to_le32(AP_BSS);
                    break;

                case PSEUDO_IBSS:
                    GenericUlong = cpu_to_le32(PSEUDO_IBSS);
                    break;
            }

            memcpy(InformationBuffer, &GenericUlong, sizeof(GenericUlong));
            *BytesWritten = sizeof(GenericUlong);
            *BytesNeeded = 0;
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_ZDX_802_11_SSID:
        case OID_802_11_NETWORK_TYPE_IN_USE:
            ZDPRODUCTDBG("OID_ZDX_802_11_SSID\r\n");

// 11G
            GenericUlong = cpu_to_le32(Ndis802_11OFDM24);

            memcpy(InformationBuffer, &GenericUlong, sizeof(GenericUlong));
            *BytesWritten = sizeof(GenericUlong);
            *BytesNeeded = 0;
            Status = NDIS_STATUS_SUCCESS;
            break;
    case OID_802_3_CURRENT_ADDRESS:
        ZDPRODUCTDBG("OID_802_3_CURRENT_ADDRESS\n");
        GenericUlong = zd_readl(E2P_MACADDR_P1);
        ((u8 *)InformationBuffer)[0]= (u8)GenericUlong ;
        ((u8 *)InformationBuffer)[1]= (u8)(GenericUlong >>  8);
        ((u8 *)InformationBuffer)[2]= (u8)(GenericUlong >> 16);
        ((u8 *)InformationBuffer)[3]= (u8)(GenericUlong >> 24);

        GenericUlong= zd_readl(E2P_MACADDR_P2);
        ((u8 *)InformationBuffer)[4]= (u8)GenericUlong ;
        ((u8 *)InformationBuffer)[5]= (u8)(GenericUlong >>  8);

        *BytesWritten = ETH_ALEN;
        *BytesNeeded = 0;
        Status = NDIS_STATUS_SUCCESS;
        break;

        case OID_ZD_RD:
            ZDPRODUCTDBG("OID_ZD_RD\r\n");

            if (InformationBufferLength < sizeof(ZD_RD_STRUCT)-4)
            {
                ZDPRODUCTDBG("OID_ZD_RD: NDIS_STATUS_BUFFER_TOO_SHORT,%d,%d\r\n",InformationBufferLength,sizeof(ZD_RD_STRUCT));
                *BytesNeeded = sizeof(ZD_RD_STRUCT);
                *BytesWritten = 0;
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                break;
            }

            pZDRD = (PZD_RD_STRUCT)InformationBuffer;

            switch (le32_to_cpu(pZDRD->ZDRdFuncId))
            {
                case ZDAccessPHYRegister1B:
                    ZDPRODUCTDBG("ZDAccessPHYRegister1B\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < sizeof(ZD_RD_STRUCT))
                    {
                        ZDPRODUCTDBG("ZDAccessPHYRegister1B: NDIS_STATUS_BUFFER_TOO_SHORT\r\n");
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

// Write PHY registers
                    LockPhyReg(&dot11Obj);

/* Remap some special PHY registers */
                    switch(le32_to_cpu(pZDRD->Buffer[0]))
                    {
                        case 4:                   //CR4
                            IoAddress = 0x20;
                            break;
                        case 5:                   //CR5
                            IoAddress = 0x10;
                            break;
                        case 6:                   //CR6
                            IoAddress = 0x14;
                            break;
                        case 7:                   //CR7
                            IoAddress = 0x18;
                            break;
                        case 8:                   //CR8
                            IoAddress = 0x1C;
                            break;
                        case 132:                 //CR132
                            IoAddress = 0x0210;

                        default:
                            IoAddress = le32_to_cpu(pZDRD->Buffer[0]) << 2;
                            break;
                    }
//if (!NeedReadProtection(Adapter, IoAddress, &(pZDRD->Buffer[1]))){
                    pZDRD->Buffer[1] = dot11Obj.GetReg(reg, IoAddress);
//}

                    pZDRD->Buffer[1] &= 0xFFFF;
                    ZDPRODUCTDBG("Read Phy Reg (0x%x) = %x\r\n", IoAddress, pZDRD->Buffer[1]);
                    UnLockPhyReg(&dot11Obj);

                    pZDRD->Buffer[1] = cpu_to_le32(pZDRD->Buffer[1]);
                    pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                    *BytesWritten = sizeof(ZD_RD_STRUCT);
                    *BytesNeeded = 0;
                    Status = NDIS_STATUS_SUCCESS;

                    break;

                case ZDAccessMACRegister4B:
                    ZDPRODUCTDBG("ZDAccessMACRegister4B\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT)))
                    {
                        ZDPRODUCTDBG("ZDAccessMACRegister4B: NDIS_STATUS_BUFFER_TOO_SHORT\r\n");
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    if (le32_to_cpu(pZDRD->Buffer[0]) < 0x1000)
                    {
                        pZDRD->Buffer[1] = cpu_to_le32(dot11Obj.GetReg(reg, le32_to_cpu(pZDRD->Buffer[0])));
                    }
//else if (!NeedReadProtection(Adapter, pZDRD->Buffer[0], &(pZDRD->Buffer[1]))){
//pZDRD->Buffer[1] = dot11Obj.GetReg(reg, pZDRD->Buffer[0]);
                    else
                        pZDRD->Buffer[1] = 0;

                    pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                    *BytesWritten = sizeof(ZD_RD_STRUCT);
                    *BytesNeeded = 0;
                    Status = NDIS_STATUS_SUCCESS;
                    break;
                case ZDAccessROMData:

                    ZDPRODUCTDBG("ZDAccessROM read\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT) + 4))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT) + 4);
                        *BytesNeeded = (sizeof(ZD_RD_STRUCT) + 4);
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    {
                        int ReadLength = le32_to_cpu(pZDRD->Buffer[1]);
//int ReadOffset = le32_to_cpu(pZDRD->Buffer[0]);
                        PUCHAR pDataPtr = (PUCHAR)(pZDRD+1);
                        ULONG tmpULONG;
                        PUCHAR pTmpBuffer = 0, pOldTmpBuffer;

/* Update the total EEPROM content */
                        //ORIGION NOT MARK
                        //RamAddr is 2nd argument, it differs from Win
                        HW_EEPROM_ACCESS(&dot11Obj, cFIRMWARE_EEPROM_OFFSET, cEPDATA_OFFSET, (E2P_END-E2P_SUBID)/2, 0);
						//ZD1205_WriteE2P(&dot11Obj);

                        pTmpBuffer = kmalloc(E2P_END - E2P_SUBID+1, GFP_ATOMIC);

                        if (!pTmpBuffer)
                        {
                            Status = NDIS_STATUS_RESOURCES;
                            break;
                        }

                        pOldTmpBuffer = pTmpBuffer;

                        for (i = E2P_SUBID; i < E2P_END; i += 4)
                        {
                            tmpULONG = dot11Obj.GetReg(reg, i);
                            *pTmpBuffer = (UCHAR)(tmpULONG & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 8) & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 16) & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 24) & 0xFF);
                            pTmpBuffer++;
                        }

                        memcpy(pDataPtr, pOldTmpBuffer, (E2P_END - E2P_SUBID + 1) );
                        kfree(pOldTmpBuffer);
                        *BytesWritten = sizeof(ZD_RD_STRUCT) + ReadLength;
                        *BytesNeeded = 0;
                        Status = NDIS_STATUS_SUCCESS;
                    }

                    break;

                case ZDGetNICAdapterTally:
                    ZDPRODUCTDBG("ZDGetNICAdapterTally\r\n");

//ZD_RD_STRUCT contain one ULONG
                    if (le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT) + sizeof(ULONG)*36))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT) + sizeof(ULONG)*36);
                        *BytesNeeded = (sizeof(ZD_RD_STRUCT) + sizeof(ULONG)*37);
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        ZDPRODUCTDBG("ZDGetNICAdapterTally BUFFER_TOO_SHORT\r\n");
                        break;
                    }

                    zd1205_CollectHwTally(macp);

//txUnicastFrames
                    pZDRD->Buffer[0] = cpu_to_le32(macp->txUnicastFrm);
//txMulticastFrames
                    pZDRD->Buffer[1] = cpu_to_le32(macp->txMulticastFrm);
//txUniOctets  byte size
                    pZDRD->Buffer[2] = cpu_to_le32(macp->txUnicastOctets);
//txMultiOctets  byte size
                    pZDRD->Buffer[3] = cpu_to_le32(macp->txMulticastOctets);
                    pZDRD->Buffer[4] = 0;         //txFrmUpperNDIS
//txFrmDrvMgt
                    pZDRD->Buffer[5] = cpu_to_le32(macp->txFrmDrvMgt);
                    pZDRD->Buffer[6] = cpu_to_le32(macp->retryFailCnt);
//Hardware total Tx Frame
                    pZDRD->Buffer[7] = cpu_to_le32(macp->hwTotalTxFrm);
//txMultipleRetriesFrames
                    pZDRD->Buffer[8] = cpu_to_le32(macp->hwRetryCnt);
//
                    pZDRD->Buffer[9] = cpu_to_le32(macp->hwUnderrunCnt);
//
                    pZDRD->Buffer[10] = cpu_to_le32(macp->DriverRxFrmCnt);
//rxUnicastFrames
                    pZDRD->Buffer[11] = cpu_to_le32(macp->rxUnicastFrm);
//rxMulticastFrames
                    pZDRD->Buffer[12] = cpu_to_le32(macp->rxMulticastFrm);
//rxTotalCnt
                    pZDRD->Buffer[13] = cpu_to_le32(macp->rxUnicastFrm+macp->rxMulticastFrm);
//NotifyNDISRxFrmCnt
                    pZDRD->Buffer[14] = cpu_to_le32(macp->rxCnt);
//rxUniOctets  byte size
                    pZDRD->Buffer[15] = cpu_to_le32(macp->rxUnicastOctets);
//rxMultiOctets  byte size
                    pZDRD->Buffer[16] = cpu_to_le32(macp->rxMulticastOctets);
// Discard by ValidateFrame
                    pZDRD->Buffer[17] = cpu_to_le32(macp->invalid_frame_good_crc);
//
                    pZDRD->Buffer[18] = cpu_to_le32(macp->ErrShortFrmCnt);
//
                    pZDRD->Buffer[19] = cpu_to_le32(macp->ErrLongFrmCnt);
                    pZDRD->Buffer[20] = 0;        //DriverDiscardedFrmCauseByMulticastList
                    pZDRD->Buffer[21] = 0;        //DriverDiscardedFrmCauseByFrmCtrl
//rxNeedFrgFrm
                    pZDRD->Buffer[22] = cpu_to_le32(macp->rxNeedFrag);
//DriverRxMgtFrmCnt
                    pZDRD->Buffer[23] = cpu_to_le32(macp->rxMgtFrm);
//Receive broadcast frame count
                    pZDRD->Buffer[24] = cpu_to_le32(macp->rxBroadcastFrm);
//Receive broadcast frame byte size
                    pZDRD->Buffer[25] = cpu_to_le32(macp->rxBroadcastOctets);
//Measured quality 11b data frame count
                    pZDRD->Buffer[26] = cpu_to_le32(macp->rx11bDataFrame);
//Measured quality 11g data frame count
                    pZDRD->Buffer[27] = cpu_to_le32(macp->rxOFDMDataFrame);

//
                    pZDRD->Buffer[28] = cpu_to_le32(macp->hwTotalRxFrm);
//rxPLCPCRCErrCnt
                    pZDRD->Buffer[29] = cpu_to_le32(macp->hwCRC16Cnt);
//rxCRC32ErrCnt
                    pZDRD->Buffer[30] = cpu_to_le32(macp->hwCRC32Cnt);
//
                    pZDRD->Buffer[31] = cpu_to_le32(macp->hwDecrypErr_UNI);
//
                    pZDRD->Buffer[32] = cpu_to_le32(macp->hwDecrypErr_Mul);
//rxDecrypFailCnt
                    pZDRD->Buffer[33] = cpu_to_le32(macp->hwDecrypErr_UNI+macp->hwDecrypErr_Mul);
//
                    pZDRD->Buffer[34] = cpu_to_le32(macp->hwRxFIFOOverrun);
                    pZDRD->Buffer[35] = 0;        //LossAP
                    pZDRD->Buffer[36] = cpu_to_le32(macp->rxDiscardByNotPIBSS);
                    pZDRD->Buffer[37] = cpu_to_le32(macp->rxDiscardByAllocateBuf);;

                    pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT) + sizeof(ULONG)*36);
                    *BytesWritten = sizeof(ZD_RD_STRUCT) + sizeof(ULONG)*36;
                    break;

                case ZDContinuousTx:
                    ZDPRODUCTDBG("ZDContinuousTx\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < sizeof(ZD_RD_STRUCT))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    pZDRD->Buffer[0] = cpu_to_le32(macp->bContinueTxMode);
                    if(macp->bContinueTx == 1)
                        pZDRD->Buffer[1] = 0x00;
                    else
                        pZDRD->Buffer[1] = cpu_to_le32(0x01);

                    pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                    break;

                default:
                    ZDPRODUCTDBG("Unknown FunctionID: 0x%08x\r\n", le32_to_cpu(pZDRD->ZDRdFuncId));
                    break;
            }
            break;

        case OID_ZD_CUSTOM:
            ZDPRODUCTDBG("OID_ZD_CUSTOM\r\n");

            pZDCustom = (ZD_CUSTOM_STRUCT *) InformationBuffer;

            switch (le32_to_cpu(pZDCustom->ZDFuncId))
            {
                case ZDPreambleMode:

                    ZDPRODUCTDBG("ZDPreambleMode\r\n");
                    switch (macp->cardSetting.PreambleType)
                    {
                        case LONG_PREAMBLE:
                            pZDCustom->DataBuffer[0] = cpu_to_le32(ZD_PreambleLong);
                            break;

                        case SHORT_PREAMBLE:
                            pZDCustom->DataBuffer[0] = cpu_to_le32(ZD_PreambleShort);
                            break;

                        default:
                            pZDCustom->DataBuffer[0] = cpu_to_le32(ZD_PreambleAuto);
                            break;
                    }
                    break;
                case ZDAdapterRegion:
                    ZDPRODUCTDBG("ZDAdapterRegion 0x%x\r\n", macp->RegionCode);
                    pZDCustom->DataBuffer[0] = cpu_to_le32(macp->RegionCode);
                    *BytesWritten = sizeof(macp->RegionCode);
                    break;

                case ZDAdapterSupportChannel:
                {
                    int i;
                    int AllowChannelCount = 0;
                    ULONG tmpAllowChannel = dot11Obj.AllowedChannel;
                    char *pChannelNumber = (char *)(pZDCustom->DataBuffer);

                    printk(KERN_ERR "ZDAdapterSupportChannel\n");

                    for (i = 0; i < 14; i++)
                    {
                        if (tmpAllowChannel & (1 << i))
                            AllowChannelCount++;
                    }

                    if (pZDCustom->ZDCustomLength < (AllowChannelCount + sizeof(ZD_RD_STRUCT) - sizeof(ULONG)))
                    {
                        pZDCustom->ZDCustomLength = (AllowChannelCount + sizeof(ZD_RD_STRUCT) - sizeof(ULONG));

                        *BytesNeeded = pZDCustom->ZDCustomLength;
                        *BytesWritten = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    for (i = 0; i < 14; i++)
                    {
                        if (tmpAllowChannel & (1 << i))
                            *(pChannelNumber++) = i+1;
                    }

                    pZDCustom->ZDCustomLength = (AllowChannelCount + sizeof(ZD_RD_STRUCT) - sizeof(ULONG));
                    *BytesWritten = pZDCustom->ZDCustomLength;

                }
                break;

            }
            break;

        case OID_ZD_GETRID:
            ZDPRODUCTDBG("OID_ZD_GETRID\r\n");
            rid_p = (PRID_STRUCT)InformationBuffer;

            switch(le16_to_cpu(rid_p->rid))
            {
                case RID_MONITOR:
                    ZDPRODUCTDBG("RID_MONITOR\r\n");

                    zd1205_CollectHwTally(macp);

                    rid_p->data[0] = (USHORT)macp->hwCRC32Cnt;
                    rid_p->data[1] = (USHORT)macp->hwCRC16Cnt;
                    rid_p->data[2] = (USHORT)macp->hwRetryCnt;
                    rid_p->data[3] = (USHORT)macp->hwTotalRxFrm;
                    rid_p->data[4] = (USHORT)macp->hwTotalTxFrm;
                    rid_p->data[5] = 0;
                    rid_p->data[6] = 0;
                    rid_p->data[7] = 0;
                    rid_p->data[8] = 0;
                    rid_p->data[9] = 0;

                    Reset_Tally(macp);
                    *BytesWritten = sizeof(RID_STRUCT);
                    break;

                case 0xFC7F:
                    ZDPRODUCTDBG("Get UDP Port\r\n");

                    rid_p->data[0] = 0;
                    *BytesWritten = sizeof(RID_STRUCT);
                    break;
                default:
                    ZDPRODUCTDBG("RID: 0x%04x not support\r\n", le16_to_cpu(rid_p->rid));
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
            }
            break;
        case OID_ZD_GET_TALLIES:
            ZDPRODUCTDBG("OID_ZD_GET_TALLIES\r\n");

            pLocalTally = (PLOCAL_TALLY_STRUCT)InformationBuffer;

/* Zero the buffer */
            memset(pLocalTally, 0, sizeof(LOCAL_TALLY_STRUCT));

            zd1205_CollectHwTally(macp);

#if (defined(GCCK) && defined(OFDM))
            pLocalTally->reserved2_L = cpu_to_le32(macp->rx11bDataFrame);
            pLocalTally->reserved3_L = cpu_to_le32(macp->rxOFDMDataFrame);
#else
//pLocalTally->reserved2 = 0;
//pLocalTally->reserved3 = 0;
#endif

            pLocalTally->reserved7_L = cpu_to_le32(macp->rxBroadcastFrm);
            pLocalTally->reserved8_L = cpu_to_le32(macp->rxBroadcastOctets);
            pLocalTally->rxCRC32ErrCnt_L = cpu_to_le32(macp->hwCRC32Cnt);
            pLocalTally->rxDecrypFailCnt_L = cpu_to_le32(macp->hwDecrypErr_UNI+macp->hwDecrypErr_Mul);
// macp->DriverDiscardedFrm
            pLocalTally->rxDiscardedCnt_L = cpu_to_le32(macp->invalid_frame_good_crc);
            pLocalTally->rxMulticastFrames_L = cpu_to_le32(macp->rxMulticastFrm);
            pLocalTally->rxMultiOctets_L = cpu_to_le32(macp->rxMulticastOctets);
            pLocalTally->rxPLCPCRCErrCnt_L = cpu_to_le32(macp->hwCRC16Cnt);
//(macp->rxMulticastFrm+macp->rxUnicastFrm)
            pLocalTally->rxTotalCnt_L = cpu_to_le32(macp->rxMulticastFrm+macp->rxUnicastFrm);
//macp->rxUnicastFrm
            pLocalTally->rxUnicastFrames_L = cpu_to_le32(macp->rxUnicastFrm);
// macp->rxUnicastOctets
            pLocalTally->rxUniOctets_L = cpu_to_le32(macp->rxUnicastOctets);
//pLocalTally->txMulticastFrames = 0; // macp->txMulticastFrm
//pLocalTally->txMultiOctets = 0; // macp->txMulticastOctets
            pLocalTally->txMultipleRetriesFrames_L = cpu_to_le32(macp->hwRetryCnt);
            pLocalTally->txRetryLimitExceeded_L = cpu_to_le32(macp->retryFailCnt);
//pLocalTally->txUnicastFrames = 0; // macp->txUnicastFrm
//pLocalTally->txUniOctets = 0; // macp->txUnicastOctets

            *BytesWritten = sizeof(LOCAL_TALLY_STRUCT);
            break;

        default:
            ZDPRODUCTDBG("%s Unknown OID = 0x%08x\r\n", "ZD1205EM_Custom_QueryInfo", Oid);
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    return Status;
}


NDIS_STATUS ZD1205EM_Custom_SetInformation(
PVOID NDIS_HANDLE,
NDIS_OID Oid,
PVOID InformationBuffer,
ULONG InformationBufferLength,
PULONG BytesRead,
PULONG BytesNeeded
)
{
    NDIS_STATUS Status;
    ULONG GenericUlong;
    ULONG IoAddress;
    ULONG IoValue;
    U8 SSIDLen;

    zd1205_private_t *macp = (zd1205_private_t *) NDIS_HANDLE;
    void *reg = macp->regp;

    NDIS_802_11_CONFIGURATION *pConfiguration;
    NDIS_802_11_SSID *pSSID;
    ZD_CUSTOM_STRUCT *pZDCustom;
    ZD_RD_STRUCT *pZDRD;
    RID_STRUCT *rid_p;
//int flags;

    Status = NDIS_STATUS_SUCCESS;

    ZDPRODUCTDBG("***** ZD1205EM_Custom_SetInformation ******\r\n");
    switch(Oid)
    {
        case OID_802_11_DESIRED_RATES:
            ZDPRODUCTDBG("OID_802_11_DESIRED_RATES\r\n");

/* Fixed the Tx rate */
            GenericUlong = le32_to_cpu(*((ULONG*) InformationBuffer));

            macp->cardSetting.FixedRate = Rate_Convert((GenericUlong & 0x7f));
            macp->bFixedRate = 1;
            ZDPRODUCTDBG("Rate: 0x%08x\r\n", macp->cardSetting.FixedRate);

            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_802_11_CONFIGURATION:
        case OID_ZDX_802_11_CONFIGURATION:
            ZDPRODUCTDBG("OID_802_11_CONFIGURATION\r\n");

            if (InformationBuffer == NULL ||
                (InformationBufferLength < sizeof(NDIS_802_11_CONFIGURATION)))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            pConfiguration = (PNDIS_802_11_CONFIGURATION) InformationBuffer;

/* Update Beacon */
            {
//U16 p = (U16) le32_to_cpu(pConfiguration->BeaconPeriod);
//U8 BcnInterval[2];
//U16 *p = (U16 *)&BcnInterval;

//BcnInterval[0] = (U8)(pConfiguration->BeaconPeriod);
//BcnInterval[1] = (U8)(pConfiguration->BeaconPeriod >> 8);

//HW_UpdateBcnInterval(&dot11Obj, *p);
            }
/* Update ATIMWindow */
            {
//U16 p = (U16) le32_to_cpu(pConfiguration->ATIMWindow);
//U8 ATIMWindow[2];
//U16 *p = (U16 *)&ATIMWindow;

//ATIMWindow[0] = (U8)(pConfiguration->ATIMWindow);
//ATIMWindow[1] = (U8)(pConfiguration->ATIMWindow >> 8);

//HW_UpdateATIMWindow(&dot11Obj, *p);
            }
/* Update Channel */
            {
                ULONG ChannelNo=0;
                UCHAR IntValue = 0xFF;
#ifdef ZDCONF_80211A_SUPPORT
                ChannelNo = Freq_11A_to_channel(pConfiguration->DSConfig/1000);
#endif
                if(!ChannelNo)
                    ChannelNo=(le32_to_cpu(pConfiguration->DSConfig)/1000-2412)/5+1;

/* If the desired channel is 14 */
                if (le32_to_cpu(pConfiguration->DSConfig) == 2484000)
                {
//macp->SaveChannel = 14;
//Set_RF_Channel(Adapter, 14, (U8)dot11Obj.rfMode, 1);
                    ChannelNo = 14;
                    macp->cardSetting.Channel = 14;
                    HW_SwitchChannel(&dot11Obj, 14, 1,1);
                }
                else
                {
//macp->SaveChannel = (U8) ChannelNo;
                    macp->cardSetting.Channel = ChannelNo;
                    if(pConfiguration->DSConfig/1000> 4000)
                    {
                        if(mMacMode != PURE_A_MODE)
                        {
							mMacMode = PURE_A_MODE;
                            ChangeMacMode( PURE_A_MODE, ChannelNo);
                            HW_SwitchChannel(&dot11Obj, ChannelNo, 1,PURE_A_MODE);
                        }
                        else 
                        {
                            //For update iwconfig information only
                            ChangeMacMode(PURE_A_MODE, ChannelNo);     
                            HW_SwitchChannel(&dot11Obj, ChannelNo, 1,PURE_A_MODE);
                        }
                    }
                    else
                    {
                        if(mMacMode == PURE_A_MODE)
                        {
							mMacMode = MIXED_MODE;
                            ChangeMacMode(MIXED_MODE, ChannelNo);
                            HW_SwitchChannel(&dot11Obj, ChannelNo, 1,MIXED_MODE);
                        }
                        else
                        {
                            //For update iwconfig information only
                            ChangeMacMode(MIXED_MODE, ChannelNo);
                            HW_SwitchChannel(&dot11Obj, ChannelNo, 1,MIXED_MODE);
                        }
                    }
                }
/* Reset the Integration value */
				if(pConfiguration->DSConfig/1000 < 4000)
                IntValue = dot11Obj.IntValue[ChannelNo-1];
#ifdef ZDCONF_80211A_SUPPORT
                if(pConfiguration->DSConfig/1000>4000)
                {
                    u8 useless;
                    if(0xff == a_OSC_get_cal_int(ChannelNo, RATE_1M, &IntValue, &useless))
						printk("Read a_OSC_get_cal_int fail\n");;
					IntValue=20; //When Initial value is too larget. The Pwr Meter will be down
                }
#endif
                dot11Obj.TxGainSetting = IntValue;
                dot11Obj.TxGainSetting2 = IntValue;

                LockPhyReg(&dot11Obj);
                dot11Obj.SetReg(reg, IntValue, ZD_CR31);
//reg->CR31 = IntValue[ChannelNo-1];
                UnLockPhyReg(&dot11Obj);
            }

            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_802_11_INFRASTRUCTURE_MODE:
        case OID_ZDX_802_11_INFRASTRUCTURE_MODE:
            ZDPRODUCTDBG("OID_802_11_INFRASTRUCTURE_MODE\r\n");

            if (InformationBuffer == NULL)
            {
                Status = (NDIS_STATUS_INVALID_LENGTH);
                break;
            }

            switch(le32_to_cpu(*(PULONG)InformationBuffer))
            {
                case Ndis802_11IBSS:
                    ZDPRODUCTDBG("Ndis802_11IBSS\r\n");
                    macp->cardSetting.BssType = INDEPENDENT_BSS;
                    zd_writel(STA_RX_FILTER, Rx_Filter);
                    break;
                case Ndis802_11Infrastructure:
                case Ndis802_11AutoUnknown:
                    ZDPRODUCTDBG("Ndis802_11Infrastructure\r\n");
                    macp->cardSetting.BssType = INFRASTRUCTURE_BSS;
                    macp->cardSetting.AuthMode = 0;
                    zd_writel(STA_RX_FILTER, Rx_Filter);
                    break;
                case AP_BSS:
                    ZDPRODUCTDBG("AP_BSS\r\n");
                    macp->cardSetting.BssType = AP_BSS;
/* Set bssid = MacAddress */
                    macp->BSSID[0] = macp->macAdr[0];
                    macp->BSSID[1] = macp->macAdr[1];
                    macp->BSSID[2] = macp->macAdr[2];
                    macp->BSSID[3] = macp->macAdr[3];
                    macp->BSSID[4] = macp->macAdr[4];
                    macp->BSSID[5] = macp->macAdr[5];

                    zd_writel(cpu_to_le32(*(u32 *)&macp->macAdr[0]), BSSID_P1);
                    zd_writel(cpu_to_le32(*(u32 *)&macp->macAdr[4]), BSSID_P2);
                    macp->cardSetting.AuthMode = 2;//auto auth
                    break;
                default:
                    ZDPRODUCTDBG("PSEUDO_IBSS\r\n");
                    macp->cardSetting.BssType = PSEUDO_IBSS;
                    macp->bPseudoIBSSMode = 1;
/* Disable beacon */
//GenericUlong = reg->BCNInterval;
                    GenericUlong = dot11Obj.GetReg(reg, BCNInterval);
                    GenericUlong &= ~BIT_24;
                    GenericUlong &= ~BIT_25;
                    dot11Obj.SetReg(reg, GenericUlong, BCNInterval);
                    dot11Obj.SetReg(reg, 0, Rx_Filter);
//reg->BCNInterval = GenericUlong;
//zd_writel(STA_RX_FILTER, Rx_Filter);
//reg->Rx_Filter = 0;
                    break;
            }

            zd_UpdateCardSetting(&macp->cardSetting);
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_ZDX_802_11_SSID:
            ZDPRODUCTDBG("OID_ZDX_802_11_SSID\r\n");

            if (InformationBufferLength < sizeof(NDIS_802_11_SSID))
            {
                Status = (NDIS_STATUS_INVALID_LENGTH);
                break;
            }

            pSSID = (PNDIS_802_11_SSID)InformationBuffer;
            SSIDLen = le32_to_cpu(pSSID->SsidLength);

            if (SSIDLen > 32)
            {
                SSIDLen = 32;
            }

            zd1205_lock(macp);
            memcpy(&macp->cardSetting.Info_SSID[2], pSSID->Ssid, SSIDLen);
            macp->cardSetting.Info_SSID[1] = SSIDLen;
            zd1205_unlock(macp);

            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_802_11_NETWORK_TYPE_IN_USE:
            printk(KERN_ERR "OID_802_11_NETWORK_TYPE_IN_USE\n");

            //Status = NDIS_STATUS_NOT_SUPPORTED;
            //Linux doesn't have NETWORK_TYPE_IN_USE variable
            //But we need to reture a success result to GoGo
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_ZD_RD:
            ZDPRODUCTDBG("OID_ZD_RD\r\n");

            if (le32_to_cpu(InformationBufferLength) < sizeof(ZD_RD_STRUCT)-4)
            {
                *BytesNeeded = sizeof(ZD_RD_STRUCT);
                *BytesRead = 0;
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                break;
            }

            pZDRD = (PZD_RD_STRUCT)InformationBuffer;

            switch (le32_to_cpu(pZDRD->ZDRdFuncId))
            {
                case ZDAccessPHYRegister1B:
                    ZDPRODUCTDBG("ZDAccessPHYRegister1B\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < sizeof(ZD_RD_STRUCT))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = (sizeof(ZD_RD_STRUCT) + sizeof(ULONG));
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

// Write PHY registers
                    LockPhyReg(&dot11Obj);

                    switch(pZDRD->Buffer[0])
                    {
                        case 4:                   //CR4
                            IoAddress = 0x20;
                            break;
                        case 5:                   //CR5
                            IoAddress = 0x10;
                            break;
                        case 6:                   //CR6
                            IoAddress = 0x14;
                            break;
                        case 7:                   //CR7
                            IoAddress = 0x18;
                            break;
                        case 8:                   //CR8
                            IoAddress = 0x1C;
                            break;
                        case 132:                 //CR132
                            IoAddress = 0x0210;

                        default:
                            IoAddress = le32_to_cpu(pZDRD->Buffer[0]) << 2;
                            break;
                    }
//if (!NeedWriteProtection(Adapter, IoAddress, pZDRD->Buffer[1])){
                    dot11Obj.SetReg(reg, IoAddress, le32_to_cpu(pZDRD->Buffer[1]));
//}

                    ZDPRODUCTDBG("Write Phy Reg (%x) = %x\n", le32_to_cpu(IoAddress), le32_to_cpu(pZDRD->Buffer[1]));
                    UnLockPhyReg(&dot11Obj);

                    *BytesRead = sizeof(ZD_RD_STRUCT);

                    break;

                case ZDAccessMACRegister4B:
                    ZDPRODUCTDBG("ZDAccessMACRegister4B\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < sizeof(ZD_RD_STRUCT))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }
                    if (le32_to_cpu(pZDRD->Buffer[0]) < 0x1000)
                    {
                        dot11Obj.SetReg(reg, le32_to_cpu(pZDRD->Buffer[0]), le32_to_cpu(pZDRD->Buffer[1]));
                    }
//else if (!NeedWriteProtection(Adapter, pZDRD->Buffer[0], pZDRD->Buffer[1])){
//	dot11Obj.SetReg(reg, pZDRD->Buffer[0], (pZDRD->Buffer[1]));
//}

                    *BytesRead = sizeof(ZD_RD_STRUCT);
                    break;
                case ZDAccessROMData:
                    ZDPRODUCTDBG("ZDAccessROM Write\r\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT) + 4))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT) + 4);
                        *BytesNeeded = (sizeof(ZD_RD_STRUCT) + 4);
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }
                    {
                        int i;
                        int WriteLength = le32_to_cpu(pZDRD->Buffer[1]);
                        int WriteOffset = le32_to_cpu(pZDRD->Buffer[0]);
                        PUCHAR  pDataPtr = (PUCHAR) (pZDRD+1);
                        ULONG   tmpULONG;
                        PUCHAR  pTmpBuffer = 0, pOldTmpBuffer;

                        pTmpBuffer = kmalloc(E2P_END - E2P_SUBID+1, GFP_ATOMIC);

                        if (!pTmpBuffer)
                        {
                            Status = NDIS_STATUS_RESOURCES;
                            break;
                        }

                        pOldTmpBuffer = pTmpBuffer;

                        for (i = E2P_SUBID; i < E2P_END; i += 4)
                        {
                            tmpULONG = dot11Obj.GetReg(reg, i);
                            *pTmpBuffer = (UCHAR)(tmpULONG & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 8) & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 16) & 0xFF);
                            pTmpBuffer++;
                            *pTmpBuffer = (UCHAR)((tmpULONG >> 24) & 0xFF);
                            pTmpBuffer++;
                        }

                        pTmpBuffer = pOldTmpBuffer;

                        if ((WriteOffset+WriteLength) <= (E2P_END - E2P_SUBID+1))
                            memcpy(pTmpBuffer+WriteOffset, pDataPtr, WriteLength);
                        else
                            Status = NDIS_STATUS_INVALID_LENGTH;

                        for (i = E2P_SUBID; i <= E2P_END; i += 4)
                        {
                            tmpULONG = *pTmpBuffer;
                            pTmpBuffer++;
                            tmpULONG |= ((ULONG)(*pTmpBuffer)) << 8;
                            pTmpBuffer++;
                            tmpULONG |= ((ULONG)(*pTmpBuffer)) << 16;
                            pTmpBuffer++;
                            tmpULONG |= ((ULONG)(*pTmpBuffer)) << 24;
                            pTmpBuffer++;

                            dot11Obj.SetReg(reg, i, tmpULONG);
                            dot11Obj.DelayUs(10);
                            ZDPRODUCTDBG("Write 0x%x with 0x%x\n", i, tmpULONG);
                        }

/* Update the total EEPROM content */
                        //RamAddr is 2nd argument, it differs from Win
                        HW_EEPROM_ACCESS(&dot11Obj, cFIRMWARE_EEPROM_OFFSET, cEPDATA_OFFSET, (E2P_END-E2P_SUBID)/2, 1);
						//ZD1205_WriteE2P(&dot11Obj);
                        kfree(pOldTmpBuffer);
                    }
                    break;
                case ZDROMUpdate:

                    ZDPRODUCTDBG("ZDROMUpdate Write\r\n");
                    {
                        int i;
                        PUCHAR pImage = (PUCHAR)pZDRD->Buffer;
                        USHORT ImageLen = (USHORT)le32_to_cpu(pZDRD->ZDRdLength)-sizeof(ZD_RD_STRUCT)+8;
                        PUCHAR pBuffer;
                        ULONG tmpULONG;

                        pBuffer = pImage;

                        if (ImageLen > EEPROM_SIZE)
                        {
                            Status = NDIS_STATUS_INVALID_LENGTH;
                            ZDPRODUCTDBG("ImageLen>EEPROM_SIZE in ZDROMUpdate Write\n");
                            ZDPRODUCTDBG("ImageLen:%d,EEPROM_SIZE:%d\n",ImageLen,EEPROM_SIZE);
                            break;
                        }

                        for (i = E2P_SUBID; i <= E2P_END; i += 4)
                        {
                            tmpULONG = *pBuffer;
                            pBuffer++;
                            tmpULONG |= ((ULONG)(*pBuffer)) << 8;
                            pBuffer++;
                            tmpULONG |= ((ULONG)(*pBuffer)) << 16;
                            pBuffer++;
                            tmpULONG |= ((ULONG)(*pBuffer)) << 24;
                            pBuffer++;

                            dot11Obj.SetReg(reg, i, tmpULONG);
                            dot11Obj.DelayUs(10);
                        }

/* Update the total EEPROM content */
                        //RamAddr is 2nd argument, it differs from Win
                        HW_EEPROM_ACCESS(&dot11Obj, cFIRMWARE_EEPROM_OFFSET, cEPDATA_OFFSET, (E2P_END-E2P_SUBID)/2, 1);
						//ZD1205_WriteE2P(&dot11Obj);
                        ZDPRODUCTDBG("ZDROMUpdate Write Complete\n");
                    }
                    break;
                case ZDSetMACAddress:
                    ZDPRODUCTDBG("ZDSetMACAddress\r\n");
                    {
                        ULONG j=0;
                        u8 *p = (u8 *)pZDRD->Buffer;
                        j+=p[3]; j<<=8;
                        j+=p[2]; j<<=8;
                        j+=p[1]; j<<=8;
                        j+=p[0];
                        dot11Obj.SetReg(reg,E2P_MACADDR_P1,j);
                        dot11Obj.DelayUs(10);
                        j=0;
                        j+=p[5]; j<<=8;
                        j+=p[4];

                        dot11Obj.SetReg(reg,E2P_MACADDR_P2,j);
                        dot11Obj.DelayUs(10);

                        //HW_EEPROM_ACCESS(&dot11Obj, 0, 0, EEPROM_SIZE, 1);
                        //RamAddr is 2nd argument, it differs from Win
                        HW_EEPROM_ACCESS(&dot11Obj, cFIRMWARE_EEPROM_OFFSET, cEPDATA_OFFSET, (E2P_END-E2P_SUBID)/2, 1);
						//ZD1205_WriteE2P(&dot11Obj);
                    }
                    break;
                case ZDEEPROMDataWrite:
                    ZDPRODUCTDBG("ZDEEPROMDataWrite\r\n");

                    if(le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT) - 4))
                    {
                        pZDRD->ZDRdLength = sizeof(ZD_RD_STRUCT);
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    //RamAddr is 2nd argument, it differs from Win
/* Update the total EEPROM content */
                    HW_EEPROM_ACCESS(&dot11Obj, cFIRMWARE_EEPROM_OFFSET, cEPDATA_OFFSET, (E2P_END-E2P_SUBID)/2, 1);
					//ZD1205_WriteE2P(&dot11Obj);
                    break;

                case ZDTxPowerGainControl:
                    ZDPRODUCTDBG("ZDTxPowerGainControl\r\n");

                    if(le32_to_cpu(pZDRD->ZDRdLength) < (sizeof(ZD_RD_STRUCT) - 4))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    if(le32_to_cpu(pZDRD->Buffer[0]) == 0)
                    {
                        zd1205_DecreaseTxPower(macp, cTX_CCK);
#if fTX_GAIN_OFDM
                        zd1205_DecreaseTxPower(macp, cTX_OFDM);
#endif
                    }
                    else
                    {
                        zd1205_IncreaseTxPower(macp, cTX_CCK);
#if fTX_GAIN_OFDM
                        zd1205_IncreaseTxPower(macp, cTX_OFDM);
#endif
                    }
                    break;

                case ZDGetNICAdapterTally:
                    printk("Clear Tally Information !!!!!!!!!!!!!!!!11\n");
                    ZDPRODUCTDBG("ZDGetNICAdapterTally\r\n");

                    Reset_Tally(macp);
                    break;

                case ZDContinuousTx:
                    printk(KERN_ERR "ZDContinuousTx\n");

                    if (le32_to_cpu(pZDRD->ZDRdLength) < sizeof(ZD_RD_STRUCT))
                    {
                        pZDRD->ZDRdLength = cpu_to_le32(sizeof(ZD_RD_STRUCT));
                        *BytesNeeded = sizeof(ZD_RD_STRUCT);
                        *BytesRead = 0;
                        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                        break;
                    }

                    macp->bContinueTxMode = le32_to_cpu(pZDRD->Buffer[0]);
                    macp->bContinueTx = le32_to_cpu(pZDRD->Buffer[1]);

/* Use the Fixed Rate instead of LastSentTxRate */
//macp->LastZDContinuousTxRate = macp->cardSetting.LastSentTxRate;
                    macp->LastZDContinuousTxRate = macp->cardSetting.FixedRate;

					if ((macp->RF_Mode == AL7230B_RF) && (mMacMode != PURE_A_MODE)) {
						if (macp->LastZDContinuousTxRate > 3) {
							HW_Set_IF_Synthesizer(&dot11Obj, 0x00093c); 
						}
						else if (macp->LastZDContinuousTxRate <= 3) {
							HW_Set_IF_Synthesizer(&dot11Obj, 0x000abc); 
						}
					}
#ifdef ZDCONF_BANDEDGE_ADJUST
//					if((macp->RF_Mode == AL7230B_RF) && 
//						(mMacMode == PURE_A_MODE) && 
//						(macp->LastZDContinuousTxRate  == 4) && 
//						(macp->PHY_36M_Setpoint_Flag == 0)
//					)
//						macp->PHY_36M_Setpoint_Flag = 1;
//				    else if((macp->RF_Mode == AL7230B_RF) && 
//						(mMacMode == PURE_A_MODE) && 
//						(macp->LastZDContinuousTxRate != 4) && 
//						(macp->PHY_36M_Setpoint_Flag == 2)
//					)
//						macp->PHY_36M_Setpoint_Flag = 3;
    
	                if(dot11Obj.HWFeature & BIT_21) { //band edge adjust for calibration, the production test tool must do integration value /set point compensation for other channels which are not calibrated
						if( (macp->RF_Mode == AL7230B_RF) && 
							((4 <= macp->LastZDContinuousTxRate) && (macp->LastZDContinuousTxRate <= 9)) && 
							(mMacMode != PURE_A_MODE) && 
							(macp->cardSetting.Channel == 1 || macp->cardSetting.Channel == 11)
						)
						{ 
							if(macp->cardSetting.Channel == 1)   
								HW_Set_IF_Synthesizer(&dot11Obj, 0x000abc);
							LockPhyReg(&dot11Obj);
							dot11Obj.SetReg(dot11Obj.reg, ZD_CR128, 0x10);
							dot11Obj.SetReg(dot11Obj.reg, ZD_CR129, 0x10);
							UnLockPhyReg(&dot11Obj);   
						}
						else if((macp->RF_Mode == AL7230B_RF) && 
								(macp->LastZDContinuousTxRate > 3) && 
								(mMacMode != PURE_A_MODE))
						{
							if((macp->LastZDContinuousTxRate > 9) || 
								(macp->cardSetting.Channel != 1 && 
								macp->cardSetting.Channel != 11))
							{
								if(macp->cardSetting.Channel == 1)
									HW_Set_IF_Synthesizer(&dot11Obj, 0x00093c);
								LockPhyReg(&dot11Obj);
								dot11Obj.SetReg(&dot11Obj.reg, ZD_CR128, 0x14);
								dot11Obj.SetReg(&dot11Obj, ZD_CR129, 0x12);
								UnLockPhyReg(&dot11Obj);
							}
						}    
						else if((macp->RF_Mode == AL7230B_RF) && 
								(macp->LastZDContinuousTxRate <=3) && 
								(mMacMode != PURE_A_MODE))
						{
                                LockPhyReg(&dot11Obj);
                                dot11Obj.SetReg(&dot11Obj.reg, ZD_CR128, 0x14);
                                dot11Obj.SetReg(&dot11Obj, ZD_CR129, 0x12);
                                UnLockPhyReg(&dot11Obj);
						}
					}
 
#elif !defined(ZDCONF_BANDEDGE_ADJUST)
	#error "Undefined ZDCONF_BANDEDGE_ADJUST"
#endif
// Roger 2004-11-10 , Set for Dr.Wang request , set 0x0001c4 when CCK mode with AL2230
#ifdef ZDCONF_RF_AL2230_SUPPORT
                    if (dot11Obj.rfMode == AL2230_RF || dot11Obj.rfMode == AL2230S_RF)
                    {
//if (macp->cardSetting.LastSentTxRate > 3) {
                        if (macp->LastZDContinuousTxRate > 3)
                        {
                            HW_Set_IF_Synthesizer(&dot11Obj, 0x0005a4);
                        }
//else if (macp->cardSetting.LastSentTxRate <= 3) {
                        else if (macp->LastZDContinuousTxRate <= 3)
                        {
                            HW_Set_IF_Synthesizer(&dot11Obj, 0x0001c4);
                        }
                    }
#endif

                    ZDPRODUCTDBG("ZDContinuousTx TxMode=%d TxStart=%d TxRate=0x%x\r\n",
                        macp->bContinueTxMode,
                        macp->bContinueTx,
                        macp->LastZDContinuousTxRate);

// Start
                    if(le32_to_cpu(pZDRD->Buffer[1]) == ContTx_Start)
                    {
                        UCHAR   tmpChr = 0;
                        UINT    RateTmp= 0;
                        ULONG tmpvalue;
                        ULONG   nLoop;
                        LockPhyReg(&dot11Obj);
                        dot11Obj.SetReg(reg, ZD1205_CR2, 0x3F);
                        dot11Obj.SetReg(reg, ZD1205_CR138, 0x28);
                        dot11Obj.SetReg(reg, ZD1205_CR33, 0x20);
// Query CR60 until change to 0x04
                        nLoop = 20;
                        while(nLoop--)
                        {
                            dot11Obj.DelayUs(10*1000);// sleep 10ms
                            tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR60);

                            if(tmpvalue == 0x04)
                                break;
                        }
                        if(nLoop == 0) printk("nLoop count down to zero. But it still fails\n");

                        UnLockPhyReg(&dot11Obj);
                        switch(le32_to_cpu(pZDRD->Buffer[0]))
                        {
                            case ContTx_Normal:   // Normal continous transmit

                                ZDPRODUCTDBG("Start ContTx_Normal\n");

                                macp->bContinueTx = 1;
                                LockPhyReg(&dot11Obj);
                                macp->PHYTestTimer = 0;
//ZD1205_WRITE_REGISTER(Adapter,CR122, 0xFF);  2004/10/22 mark
                                UnLockPhyReg(&dot11Obj);

                                LockPhyReg(&dot11Obj);
/* In order to avoid the uninitial length problem,
   force to set length to 0x20.
 */
                                dot11Obj.SetReg(reg, ZD1205_CR134, 0x20);

                                switch (macp->LastZDContinuousTxRate)
                                {
                                    case 4:       //6M
                                        RateTmp = 0xB;
                                        break;
                                    case 5:       //9M
                                        RateTmp = 0xF;
                                        break;
                                    case 6:       //12M
                                        RateTmp = 0xA;
                                        break;
                                    case 7:       //18M
                                        RateTmp = 0xE;
                                        break;
                                    case 8:       //24M
                                        RateTmp = 0x9;
                                        break;
                                    case 9:       //36M
                                        RateTmp = 0xD;
                                        break;

                                    case 0xA:     //48M
                                        RateTmp = 0x8;
                                        break;

                                    case 0xB:     //54M
                                        RateTmp = 0xC;
                                        break;

                                    default:
                                        RateTmp = 0;
                                        break;
                                }

                                ZDPRODUCTDBG("RateTmp=0x%08x\n", RateTmp);

                                if (RateTmp)
                                {
                                    dot11Obj.SetReg(reg, ZD1205_CR132, RateTmp);

//AcquireCtrOfPhyReg(Adapter);
                                    tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR159);
                                    tmpvalue &= ~(BIT_0 + BIT_1 );
                                    tmpvalue |= BIT_2;
                                    dot11Obj.SetReg(reg, ZD1205_CR159, tmpvalue);

                                    UnLockPhyReg(&dot11Obj);

                                    dot11Obj.SetReg(reg, 0x644, 7);

                                    tmpvalue = dot11Obj.GetReg(reg, 0x648);
                                    tmpvalue &= ~BIT_0;
                                    dot11Obj.SetReg(reg, 0x648, tmpvalue);

                                    break;
                                }

                                tmpChr = macp->LastZDContinuousTxRate;
                                ZDPRODUCTDBG("tmpChr=0x%x\n", tmpChr);

#if 0
                                if (macp->preambleMode == 1)
                                    macp->cardSetting.PreambleType = 0x00;
                                else if (macp->preambleMode == 2)
                                    macp->cardSetting.PreambleType = 0x20;
#endif

                                if (macp->cardSetting.PreambleType == SHORT_PREAMBLE)
                                {
// short premable
                                    tmpChr |= BIT_5;
                                }
                                else
                                {
// long premable
                                    tmpChr &= ~BIT_5;
                                }

                                if (macp->RegionCode == 0x10)
                                    tmpChr &= ~BIT_6;//USA
                                if (macp->RegionCode == 0x40)
                                    tmpChr |= BIT_6;//japan

                                dot11Obj.SetReg(reg, ZD1205_CR5, tmpChr);
                                UnLockPhyReg(&dot11Obj);

                                dot11Obj.SetReg(reg, 0x644, 3);
                                break;

                            case ContTx_CW:       // CW transmit
                                ZDPRODUCTDBG("Start CW transmit\n");
                                macp->bContinueTx = 1;

                                LockPhyReg(&dot11Obj);
                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR9);
                                tmpvalue |= 0x18;
                                dot11Obj.SetReg(reg, ZD1205_CR9, tmpvalue);
                                UnLockPhyReg(&dot11Obj);

                                dot11Obj.SetReg(reg, 0x644, 0x3);
                                break;

// Carrier suppression
                            case ContTx_CarrierSuppression:
                                ZDPRODUCTDBG("Start Carrier suppression transmit\n");
                                macp->bContinueTx = 1;

                                LockPhyReg(&dot11Obj);
                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR9);
                                tmpvalue &= ~0x10;
                                tmpvalue |= 0x8;
                                dot11Obj.SetReg(reg, ZD1205_CR9, tmpvalue);

                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR32);
                                tmpvalue |= 0x4;
                                dot11Obj.SetReg(reg, ZD1205_CR32, tmpvalue);
                                UnLockPhyReg(&dot11Obj);

                                dot11Obj.SetReg(reg, 0x644, 0x3);
                                break;

                            default:
                                ZDPRODUCTDBG("Continuous Tx mode: %d not support\n", pZDRD->Buffer[0]);
                                break;
                        }
                    }
                    else
                    {
                        ULONG tmpvalue;

// Roger 2004-11-10 , Set for Dr.Wang request , set 0x0001c4 when CCK mode with AL2230
#ifdef ZDCONF_RF_AL2230_SUPPORT
                        if (dot11Obj.rfMode == AL2230_RF || dot11Obj.rfMode == AL2230S_RF)
                        {
                            HW_Set_IF_Synthesizer(&dot11Obj, 0x0005a4);
                        }
#endif

                        LockPhyReg(&dot11Obj);
                        dot11Obj.SetReg(reg, ZD1205_CR2, 0x26);
                        dot11Obj.SetReg(reg, ZD1205_CR138, 0xA8);
                        dot11Obj.SetReg(reg, ZD1205_CR33, 0x08);
                        UnLockPhyReg(&dot11Obj);
                        switch(pZDRD->Buffer[0])
                        {
                            case ContTx_Normal:   // Normal continous transmit
                                ZDPRODUCTDBG("Stop Normal Continuous Transmit\n");

                                macp->bContinueTx = 0;
                                LockPhyReg(&dot11Obj);
                                macp->PHYTestTimer = 30;
//						ZD1205_WRITE_REGISTER(Adapter,CR122, 0x0);
                                UnLockPhyReg(&dot11Obj);

                                if (macp->LastZDContinuousTxRate >= 4)
                                {
                                    LockPhyReg(&dot11Obj);
                                    tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR159);
                                    tmpvalue &= ~(BIT_0 + BIT_1 + BIT_2 );
                                    dot11Obj.SetReg(reg, ZD1205_CR159, tmpvalue);
                                    UnLockPhyReg(&dot11Obj);

                                    dot11Obj.SetReg(reg, 0x644, 0);

                                    tmpvalue = dot11Obj.GetReg(reg, 0x648);
                                    tmpvalue |= BIT_0;
                                    dot11Obj.SetReg(reg, 0x648, tmpvalue);

                                }
                                else
                                {

                                    dot11Obj.SetReg(reg, 0x644, 0);

                                    tmpvalue = dot11Obj.GetReg(reg, 0x648);
                                    tmpvalue |= BIT_0;
                                    dot11Obj.SetReg(reg, 0x648, tmpvalue);
                                }

                                break;
                            case ContTx_CW:       // CW transmit
                                ZDPRODUCTDBG("Stop CW transmit\n");
                                macp->bContinueTx = 0;
                                LockPhyReg(&dot11Obj);
                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR9);
                                tmpvalue &= ~0x18;
                                dot11Obj.SetReg(reg, ZD1205_CR9, tmpvalue);
                                UnLockPhyReg(&dot11Obj);
                                dot11Obj.SetReg(reg, 0x644, 0x0);
                                break;

// Carrier suppression
                            case ContTx_CarrierSuppression:
                                ZDPRODUCTDBG("Stop Carrier suppression transmit\n");
                                macp->bContinueTx = 0;
                                LockPhyReg(&dot11Obj);
                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR9);
                                tmpvalue &= ~0x18;
                                dot11Obj.SetReg(reg, ZD1205_CR9, tmpvalue);

                                tmpvalue = dot11Obj.GetReg(reg, ZD1205_CR32);
                                tmpvalue &= ~0x4;
                                dot11Obj.SetReg(reg, ZD1205_CR32, tmpvalue);
                                UnLockPhyReg(&dot11Obj);

                                dot11Obj.SetReg(reg, 0x644, 0x0);
                                break;
                            default:
                                ZDPRODUCTDBG("Continuous Tx mode: %d not support\r\n", le32_to_cpu(pZDRD->Buffer[0]));
                                break;
                        }

//dot11Obj.SetReg(reg, ZD_PS_Ctrl, 0x1);
//Roger 2004-11-16 SoftwareReset here to solve RX fail after TxContinue problem
                        zd1205_device_reset(macp);
                    }
            }
            break;

        case OID_ZD_CUSTOM:
            ZDPRODUCTDBG("OID_ZD_CUSTOM\r\n");

            pZDCustom = (ZD_CUSTOM_STRUCT *) InformationBuffer;

            switch (le32_to_cpu(pZDCustom->ZDFuncId))
            {
                case ZDPreambleMode:
                    ZDPRODUCTDBG("ZDPreambleMode\r\n");
                    switch (le32_to_cpu(pZDCustom->DataBuffer[0]))
                    {
                        case ZD_PreambleLong:
                            macp->cardSetting.PreambleType = LONG_PREAMBLE;
                            break;

                        case ZD_PreambleShort:
                            macp->cardSetting.PreambleType = SHORT_PREAMBLE;
                            break;

                        case ZD_PreambleAuto:
                            macp->cardSetting.PreambleType = 2;
                            break;
                    }
                    break;
                case ZDAdapterRegion:
                    ZDPRODUCTDBG("ZDAdapterRegion\r\n");

                    macp->RegionCode = (USHORT)pZDCustom->DataBuffer[0];

                    if (macp->RegionCode == 0)
                    {
                        ULONG tmpvalue = dot11Obj.GetReg(reg, E2P_SUBID);
                        macp->RegionCode = (USHORT)(tmpvalue >> 16);
                        ZDPRODUCTDBG("Restore OperationRegionCode=0x%x\n", macp->RegionCode);
                    }

//HW_SetRfChannel(&dot11Obj, macp->cardSetting.Channel, 1);
                    break;

                case ZDAdapterSupportChannel:
                {
                    int i;
                    char *pChannelNumber = (char *)(pZDCustom->DataBuffer);
                    UINT TotalChannel = le32_to_cpu(pZDCustom->ZDCustomLength) - sizeof(ZD_RD_STRUCT) + 8;

                    printk(KERN_ERR "ZDAdapterSupportChannel\n");

                    if (TotalChannel > 14)
                    {
                        *BytesRead = 0;
                        //Status = NDIS_STATUS_INVALID_LENGTH;
                        Status = NDIS_STATUS_SUCCESS;
                        break;
                    }

                    if (TotalChannel == 0)
                    {
                        dot11Obj.AllowedChannel = dot11Obj.GetReg(reg, E2P_ALLOWED_CHANNEL);
                        ZDPRODUCTDBG("Restore RID_CHANNEL_LIST, dot11Obj.AllowedChannel = %x\n", dot11Obj.AllowedChannel);
                        break;
                    }

                    dot11Obj.AllowedChannel &= ~0xffff;

                    for (i=0; i < TotalChannel; i++)
                    {
                        dot11Obj.AllowedChannel |= (1 << (*(pChannelNumber)-1));
                        ZDPRODUCTDBG("AllowedChannel = %x\n", *(pChannelNumber));
                        pChannelNumber++;
                    }

                    ZDPRODUCTDBG("Set RID_CHANNEL_LIST, dot11Obj.AllowedChannel = %x\n", dot11Obj.AllowedChannel);
                }
                break;

                default:
                    ZDPRODUCTDBG("Unsupport function code: 0x%08x\r\n", (U32) pZDCustom->ZDFuncId);
                    break;
            }

            break;

        case OID_ZD_IO32:
            IoAddress = le32_to_cpu((ULONG)(*((PULONG)InformationBuffer))) & 0xffffffff;
            IoValue = le32_to_cpu((ULONG)(*((PULONG)InformationBuffer+ 1)));

            ZDPRODUCTDBG("OID_ZD_IO32\r\n");

/* Write command */
            if (IoAddress & 0x8000)
            {
                IoAddress &= 0x7fff;

                if (IoAddress == 0x712)
                {
/* Disable all register access */
                    macp->PHYTestTimer = (UCHAR) IoValue;
                }
                else if (IoAddress == 0x730)
                {
                    macp->PHYLowPower = (UCHAR) IoValue;
                    if (!(macp->PHYLowPower & BIT_1))
                    {
                        dot11Obj.CR31Flag = 0;
                        macp->bTraceSetPoint = 1;
                        HW_UpdateIntegrationValue(&dot11Obj, dot11Obj.Channel,1);
                    }
                }
                else if (IoAddress == 0x903)
                {
                    if (IoValue & 0x80)
                    {
                        LockPhyReg(&dot11Obj);
                        dot11Obj.SetReg(reg, ZD1205_CR30, 0x49);
                        UnLockPhyReg(&dot11Obj);
                    }
                    else
                    {
                        LockPhyReg(&dot11Obj);
                        dot11Obj.SetReg(reg, ZD1205_CR30, 0x4b);
                        UnLockPhyReg(&dot11Obj);
                    }

                    IoValue &= ~0x80;
                    macp->cardSetting.FixedRate = (UCHAR)IoValue;
                    macp->bFixedRate = 1;
                }
				else if (IoAddress == 0xa00) {
					HW_Set_IF_Synthesizer(&dot11Obj, IoValue);
					LockPhyReg(&dot11Obj);
                    if(dot11Obj.rfMode != UW2453_RF)
                    {
                        dot11Obj.SetReg(reg, ZD1205_CR203, 0x06);
                    }
                    else
                    {
                        dot11Obj.SetReg(reg, ZD1205_CR101, 0x09);
                        dot11Obj.SetReg(reg, ZD1205_CR109, 0x13);
                        dot11Obj.SetReg(reg, ZD1205_CR110, 0x09);
                        dot11Obj.SetReg(reg, ZD1205_CR111, 0x13);
                        dot11Obj.SetReg(reg, ZD1205_CR113, 0x27);
                        dot11Obj.SetReg(reg, ZD1205_CR117, 0xFA);
                        dot11Obj.SetReg(reg, ZD1205_CR46, 0x92);
                        dot11Obj.SetReg(reg, ZD1205_CR127, 0x03);

                    }
                    UnLockPhyReg(&dot11Obj);
                    dot11Obj.CR203Flag = 2;
                    
                }
                else if (IoAddress == 0xa1a)
                {
                    macp->EnableTxPwrCtrl = (U8) IoValue;
                }
                else
                {
                    ZDPRODUCTDBG("Unknown IOAddress: 0x%08x\r\n", IoAddress);
                    break;
                }
            }
            else
            {
                    ZDPRODUCTDBG("Unknow command: 0x%08x\r\n", IoAddress);
            }
            break;

        case OID_ZD_SETRID:
            ZDPRODUCTDBG("OID_ZD_SETRID\r\n");
            rid_p = (RID_STRUCT *) InformationBuffer;

            switch(le16_to_cpu(rid_p->rid))
            {
                    default:
                            ZDPRODUCTDBG("RID: 0x%04x not support\r\n", le16_to_cpu(rid_p->rid));
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
            }

            break;

        case OID_ZD_SET_TALLIES:
            ZDPRODUCTDBG("OID_ZD_SET_TALLIES\r\n");
            Status = NDIS_STATUS_SUCCESS;
            break;

        default:
            ZDPRODUCTDBG("%s Unknown OID = 0x%08x\r\n", "ZD1205EM_Custom_SetInformation", Oid);
            break;
    }
    return Status;
}
#endif

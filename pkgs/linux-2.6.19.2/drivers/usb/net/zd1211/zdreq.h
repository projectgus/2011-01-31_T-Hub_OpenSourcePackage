#ifndef _ZDREQ_H
#define _ZDREQ_H

#include "zd1205.h"
#include "zdtypes.h"
#include "zdapi.h"
#include "common.h"
#include "zdsorts.h"

/* Type definition */
typedef U32 NDIS_STATUS;
typedef U32 NDIS_OID;
typedef void* PVOID;
typedef U8 UCHAR;
typedef U8* PUCHAR;
typedef U16 USHORT;
typedef U16* PUSHORT;
typedef U32 UINT;
typedef U32 ULONG;
typedef U32* PULONG;
typedef unsigned long long ULONGLONG;

#define NDIS_MAJOR_VERSION          0x5
#define NDIS_MINOR_VERSION          0x1
#define ZD1205_DRIVER_VERSION           ((NDIS_MAJOR_VERSION * 0x100) + (NDIS_MINOR_VERSION))
/* OID definition */
#define OID_GEN_MEDIA_CONNECT_STATUS        0x00010114
#define OID_GEN_DRIVER_VERSION          0x00010116
#define OID_802_3_CURRENT_ADDRESS       0x01010102
#define OID_802_11_DESIRED_RATES        0x0D010210
#define OID_802_11_CONFIGURATION        0x0D010211
#define OID_ZDX_802_11_CONFIGURATION        0xFF12FF11
#define OID_802_11_INFRASTRUCTURE_MODE      0x0D010108
#define OID_ZDX_802_11_INFRASTRUCTURE_MODE  0xFF12FF08
#define OID_ZDX_802_11_SSID         0xFF12FF02
#define OID_802_11_NETWORK_TYPE_IN_USE      0x0D010204
#define OID_ZD_RD               0xFF129902
#define OID_ZD_CUSTOM               0xFF129901
#define OID_ZD_IO32             0xFF0201AC
#define OID_ZD_GET_TALLIES          0xFF02018D
#define OID_ZD_SET_TALLIES          0xFF02018D
#define OID_ZD_GETRID               0xFF010181
#define OID_ZD_SETRID               0xFF010181

//
// NDIS_STATUS values
//

#define STATUS_SUCCESS                          (0x00000000L)
#define STATUS_PENDING                          (0x00000103L)
#define STATUS_INSUFFICIENT_RESOURCES       (0xC000009AL)
#define STATUS_NOT_SUPPORTED            (0xC00000BBL)

#define NDIS_STATUS_SUCCESS                     ((NDIS_STATUS)STATUS_SUCCESS)
#define NDIS_STATUS_PENDING                     ((NDIS_STATUS)STATUS_PENDING)
#define NDIS_STATUS_NOT_RECOGNIZED              ((NDIS_STATUS)0x00010001L)
#define NDIS_STATUS_NOT_COPIED                  ((NDIS_STATUS)0x00010002L)
#define NDIS_STATUS_NOT_ACCEPTED                ((NDIS_STATUS)0x00010003L)
#define NDIS_STATUS_CALL_ACTIVE                 ((NDIS_STATUS)0x00010007L)

#define NDIS_STATUS_ONLINE                      ((NDIS_STATUS)0x40010003L)
#define NDIS_STATUS_RESET_START                 ((NDIS_STATUS)0x40010004L)
#define NDIS_STATUS_RESET_END                   ((NDIS_STATUS)0x40010005L)
#define NDIS_STATUS_RING_STATUS                 ((NDIS_STATUS)0x40010006L)
#define NDIS_STATUS_CLOSED                      ((NDIS_STATUS)0x40010007L)
#define NDIS_STATUS_WAN_LINE_UP                 ((NDIS_STATUS)0x40010008L)
#define NDIS_STATUS_WAN_LINE_DOWN               ((NDIS_STATUS)0x40010009L)
#define NDIS_STATUS_WAN_FRAGMENT                ((NDIS_STATUS)0x4001000AL)
#define NDIS_STATUS_MEDIA_CONNECT               ((NDIS_STATUS)0x4001000BL)
#define NDIS_STATUS_MEDIA_DISCONNECT            ((NDIS_STATUS)0x4001000CL)
#define NDIS_STATUS_HARDWARE_LINE_UP            ((NDIS_STATUS)0x4001000DL)
#define NDIS_STATUS_HARDWARE_LINE_DOWN          ((NDIS_STATUS)0x4001000EL)
#define NDIS_STATUS_INTERFACE_UP                ((NDIS_STATUS)0x4001000FL)
#define NDIS_STATUS_INTERFACE_DOWN              ((NDIS_STATUS)0x40010010L)
#define NDIS_STATUS_MEDIA_BUSY                  ((NDIS_STATUS)0x40010011L)
#define NDIS_STATUS_MEDIA_SPECIFIC_INDICATION   ((NDIS_STATUS)0x40010012L)
#define NDIS_STATUS_WW_INDICATION               NDIS_STATUS_MEDIA_SPECIFIC_INDICATION
#define NDIS_STATUS_LINK_SPEED_CHANGE           ((NDIS_STATUS)0x40010013L)
#define NDIS_STATUS_WAN_GET_STATS               ((NDIS_STATUS)0x40010014L)
#define NDIS_STATUS_WAN_CO_FRAGMENT             ((NDIS_STATUS)0x40010015L)
#define NDIS_STATUS_WAN_CO_LINKPARAMS           ((NDIS_STATUS)0x40010016L)

#define NDIS_STATUS_NOT_RESETTABLE              ((NDIS_STATUS)0x80010001L)
#define NDIS_STATUS_SOFT_ERRORS                 ((NDIS_STATUS)0x80010003L)
#define NDIS_STATUS_HARD_ERRORS                 ((NDIS_STATUS)0x80010004L)
#define NDIS_STATUS_BUFFER_OVERFLOW             ((NDIS_STATUS)STATUS_BUFFER_OVERFLOW)

#define NDIS_STATUS_FAILURE                     ((NDIS_STATUS) STATUS_UNSUCCESSFUL)
#define NDIS_STATUS_RESOURCES                   ((NDIS_STATUS)STATUS_INSUFFICIENT_RESOURCES)
#define NDIS_STATUS_CLOSING                     ((NDIS_STATUS)0xC0010002L)
#define NDIS_STATUS_BAD_VERSION                 ((NDIS_STATUS)0xC0010004L)
#define NDIS_STATUS_BAD_CHARACTERISTICS         ((NDIS_STATUS)0xC0010005L)
#define NDIS_STATUS_ADAPTER_NOT_FOUND           ((NDIS_STATUS)0xC0010006L)
#define NDIS_STATUS_OPEN_FAILED                 ((NDIS_STATUS)0xC0010007L)
#define NDIS_STATUS_DEVICE_FAILED               ((NDIS_STATUS)0xC0010008L)
#define NDIS_STATUS_MULTICAST_FULL              ((NDIS_STATUS)0xC0010009L)
#define NDIS_STATUS_MULTICAST_EXISTS            ((NDIS_STATUS)0xC001000AL)
#define NDIS_STATUS_MULTICAST_NOT_FOUND         ((NDIS_STATUS)0xC001000BL)
#define NDIS_STATUS_REQUEST_ABORTED             ((NDIS_STATUS)0xC001000CL)
#define NDIS_STATUS_RESET_IN_PROGRESS           ((NDIS_STATUS)0xC001000DL)
#define NDIS_STATUS_CLOSING_INDICATING          ((NDIS_STATUS)0xC001000EL)
#define NDIS_STATUS_NOT_SUPPORTED               ((NDIS_STATUS)STATUS_NOT_SUPPORTED)
#define NDIS_STATUS_INVALID_PACKET              ((NDIS_STATUS)0xC001000FL)
#define NDIS_STATUS_OPEN_LIST_FULL              ((NDIS_STATUS)0xC0010010L)
#define NDIS_STATUS_ADAPTER_NOT_READY           ((NDIS_STATUS)0xC0010011L)
#define NDIS_STATUS_ADAPTER_NOT_OPEN            ((NDIS_STATUS)0xC0010012L)
#define NDIS_STATUS_NOT_INDICATING              ((NDIS_STATUS)0xC0010013L)
#define NDIS_STATUS_INVALID_LENGTH              ((NDIS_STATUS)0xC0010014L)
#define NDIS_STATUS_INVALID_DATA                ((NDIS_STATUS)0xC0010015L)
#define NDIS_STATUS_BUFFER_TOO_SHORT            ((NDIS_STATUS)0xC0010016L)
#define NDIS_STATUS_INVALID_OID                 ((NDIS_STATUS)0xC0010017L)
#define NDIS_STATUS_ADAPTER_REMOVED             ((NDIS_STATUS)0xC0010018L)
#define NDIS_STATUS_UNSUPPORTED_MEDIA           ((NDIS_STATUS)0xC0010019L)
#define NDIS_STATUS_GROUP_ADDRESS_IN_USE        ((NDIS_STATUS)0xC001001AL)
#define NDIS_STATUS_FILE_NOT_FOUND              ((NDIS_STATUS)0xC001001BL)
#define NDIS_STATUS_ERROR_READING_FILE          ((NDIS_STATUS)0xC001001CL)
#define NDIS_STATUS_ALREADY_MAPPED              ((NDIS_STATUS)0xC001001DL)
#define NDIS_STATUS_RESOURCE_CONFLICT           ((NDIS_STATUS)0xC001001EL)
#define NDIS_STATUS_NO_CABLE                    ((NDIS_STATUS)0xC001001FL)

#define NDIS_STATUS_INVALID_SAP                 ((NDIS_STATUS)0xC0010020L)
#define NDIS_STATUS_SAP_IN_USE                  ((NDIS_STATUS)0xC0010021L)
#define NDIS_STATUS_INVALID_ADDRESS             ((NDIS_STATUS)0xC0010022L)
#define NDIS_STATUS_VC_NOT_ACTIVATED            ((NDIS_STATUS)0xC0010023L)
// cause 27
#define NDIS_STATUS_DEST_OUT_OF_ORDER           ((NDIS_STATUS)0xC0010024L)
// cause 35,45
#define NDIS_STATUS_VC_NOT_AVAILABLE            ((NDIS_STATUS)0xC0010025L)
// cause 37
#define NDIS_STATUS_CELLRATE_NOT_AVAILABLE      ((NDIS_STATUS)0xC0010026L)
// cause 49
#define NDIS_STATUS_INCOMPATABLE_QOS            ((NDIS_STATUS)0xC0010027L)
// cause 93
#define NDIS_STATUS_AAL_PARAMS_UNSUPPORTED      ((NDIS_STATUS)0xC0010028L)
// cause 3
#define NDIS_STATUS_NO_ROUTE_TO_DESTINATION     ((NDIS_STATUS)0xC0010029L)

#define NDIS_STATUS_TOKEN_RING_OPEN_ERROR       ((NDIS_STATUS)0xC0011000L)
#define NDIS_STATUS_INVALID_DEVICE_REQUEST      ((NDIS_STATUS)STATUS_INVALID_DEVICE_REQUEST)
#define NDIS_STATUS_NETWORK_UNREACHABLE         ((NDIS_STATUS)STATUS_NETWORK_UNREACHABLE)

#define NDIS_802_11_LENGTH_SSID         32

//
//#define ZD_GENERIC_OID_HDR_LEN			16

// NDIS 802_11 structure

typedef struct _NDIS_802_11_CONFIGURATION_FH
{
    ULONG           Length;                       // Length of structure
    ULONG           HopPattern;                   // As defined by 802.11, MSB set
    ULONG           HopSet;                       // to one if non-802.11
    ULONG           DwellTime;                    // units are Kusec
} NDIS_802_11_CONFIGURATION_FH, *PNDIS_802_11_CONFIGURATION_FH;

typedef struct _NDIS_802_11_CONFIGURATION
{
    ULONG           Length;                       // Length of structure
    ULONG           BeaconPeriod;                 // units are Kusec
    ULONG           ATIMWindow;                   // units are Kusec
    ULONG           DSConfig;                     // Frequency, units are kHz
    NDIS_802_11_CONFIGURATION_FH    FHConfig;
} NDIS_802_11_CONFIGURATION, *PNDIS_802_11_CONFIGURATION;

typedef struct _NDIS_802_11_SSID
{
    ULONG           SsidLength;                   // length of SSID field below, in bytes;
// this can be zero.

    UCHAR           Ssid[NDIS_802_11_LENGTH_SSID];
} NDIS_802_11_SSID, *PNDIS_802_11_SSID;

typedef struct   _ZD_RD_STRUCT
{
    ULONG           ZDRdLength;
    ULONG           ZDRdFuncId;
    ULONG           Buffer[2];
} ZD_RD_STRUCT, *PZD_RD_STRUCT;

typedef struct   _ZD_CUSTOM_STRUCT
{
    ULONG           ZDCustomLength;
    ULONG           ZDFuncId;
    ULONG           DataBuffer[2];
} ZD_CUSTOM_STRUCT, *PZD_CUSTOM_STRUCT;

typedef struct _RID_STRUCT
{
    USHORT          length;
    USHORT          rid;
    USHORT          data[100];
} RID_STRUCT;

typedef RID_STRUCT* PRID_STRUCT;

typedef struct _LOCAL_TALLY_STRUCT
{
    ULONG  txUnicastFrames_L;
    ULONG  txUnicastFrames_H;
    ULONG  txMulticastFrames_L;
    ULONG  txMulticastFrames_H;
    ULONG  reserved1_L;
    ULONG  reserved1_H;
    ULONG  txUniOctets_L;
    ULONG  txUniOctets_H;
    ULONG  txMultiOctets_L;
    ULONG  txMultiOctets_H;
    ULONG  reserved2_L;
    ULONG  reserved2_H;
    ULONG  reserved3_L;
    ULONG  reserved3_H;
    ULONG  txMultipleRetriesFrames_L;
    ULONG  txMultipleRetriesFrames_H;
    ULONG  txRetryLimitExceeded_L;
    ULONG  txRetryLimitExceeded_H;
    ULONG  reserved4_L;
    ULONG  reserved4_H;

    ULONG  rxUnicastFrames_L;
    ULONG  rxUnicastFrames_H;
    ULONG  rxMulticastFrames_L;
    ULONG  rxMulticastFrames_H;
    ULONG  rxPLCPCRCErrCnt_L;
    ULONG  rxPLCPCRCErrCnt_H;
    ULONG  rxUniOctets_L;
    ULONG  rxUniOctets_H;
    ULONG  rxMultiOctets_L;
    ULONG  rxMultiOctets_H;
    ULONG  rxCRC32ErrCnt_L;
    ULONG  rxCRC32ErrCnt_H;
    ULONG  rxDiscardedCnt_L;
    ULONG  rxDiscardedCnt_H;
    ULONG  rxTotalCnt_L;
    ULONG  rxTotalCnt_H;
    ULONG  rxDecrypFailCnt_L;
    ULONG  rxDecrypFailCnt_H;
    ULONG  reserved7_L;
    ULONG  reserved7_H;
    ULONG  reserved8_L;
    ULONG  reserved8_H;
} LOCAL_TALLY_STRUCT;
typedef LOCAL_TALLY_STRUCT* PLOCAL_TALLY_STRUCT;
// Defines the state of the LAN media
//
typedef enum _NDIS_MEDIA_STATE
{
    NdisMediaStateConnected,
    NdisMediaStateDisconnected
} NDIS_MEDIA_STATE, *PNDIS_MEDIA_STATE;

typedef enum _NDIS_802_11_NETWORK_INFRASTRUCTURE
{
    Ndis802_11IBSS,
    Ndis802_11Infrastructure,
    Ndis802_11AutoUnknown,
    Ndis802_11InfrastructureMax                   // Not a real value, defined as upper bound
} NDIS_802_11_NETWORK_INFRASTRUCTURE, *PNDIS_802_11_NETWORK_INFRASTRUCTURE;

typedef enum _NDIS_802_11_NETWORK_TYPE
{
    Ndis802_11FH,
    Ndis802_11DS,
    Ndis802_11OFDM5,
    Ndis802_11OFDM24,
    Ndis802_11Automode,
    Ndis802_11NetworkTypeMax
} NDIS_802_11_NETWORK_TYPE, *PNDIS_802_11_NETWORK_TYPE;
typedef enum _ZD_802_11_PREAMBLE_MODE
{
    ZD_PreambleLong,
    ZD_PreambleShort,
    ZD_PreambleAuto
} ZD_802_11_PREAMBLE_MODE, *PZD_802_11_PREAMBLE_MODE;

typedef enum _ZD_CUSTOM_FUNC_ID
{
    ZDAdapterOperationMode      = 0x00,
    ZDLinkStatus            = 0x10,
    ZDCommQuality           = 0x11,
    ZDPreambleMode          = 0x30,
    ZDDesiredSSID           = 0x31,
    ZDAdapterRegion         = 0x32,
    ZDAdapterSupportChannel     = 0x33,
    ZDWepInfo           = 0x34,
    ZDMultiDomainCapability     = 0x35,
    ZDGetAccessPointStationList = 0x50,
    ZDAccessPointFilterMode     = 0x51,
    ZDAccessPointFilterList     = 0x52,
    ZDAccessPointHideSSID       = 0x53,
    ZDAccessPointTxPower        = 0x54,
    ZDAccessPointBasicRate      = 0x55,
    ZDDelOneStationFromAPStationList = 0x60,
    ZDFirmwareVersion       = 0x70,
    ZDUseZDXOid         = 0x71,
    ZDRadioState            = 0x72,
    ZDUSBType           = 0x73,
    ZDTxPowerLevel          = 0x74,
    ZDAdhocMode         = 0x75,
    ZDUseGinUsb1_1          = 0x76,
    ZDApWirelessMode        = 0x77,
    ZDApProtectionMode      = 0x78,
} ZD_CUSTOM_FUNC_ID, *PZD_CUSTOM_FUNC_ID;

// Function ID define for OID_ZD_RD
#define ZDAccessPHYRegister4B       0
#define ZDAccessPHYRegister2B       1
#define ZDAccessPHYRegister1B       2
#define ZDAccessMACRegister4B       3
#define ZDAccessMACRegister2B       4
#define ZDAccessMACRegister1B       5
#define ZDAccessROMData         6
#define ZDROMUpdate         7
#define ZDContinuousTx          8
#define ZDGetNICAdapterTally        9
#define ZDSetMACAddress         10
#define ZDBootCodeUpdate        11
#define ZDFlashErase            12
#define ZDFlashProgram          13
#define ZDFlashRead         14
#define ZDEEPROMDataWrite       15
#define ZDTxPowerGainControl        16
#define ZDFlashIDQuery          17
#define ZDFlashGetChkSum        18
#define ZDFlashGetSubChkSum     19

// Function ID define for OID_ZD_GETRID
#define RID_MONITOR         0xFFFE
/* Definition of commands */
#define CMD_QUERY_DEVICE_STATUS         0x10
#define CMD_DEVICE_STATUS_RESPONSE      0x11
#define CMD_QUERY_INFORMATION           0x20
#define CMD_QUERY_INFORMATION_RESPONSE  0x21
#define CMD_SET_INFORMATION             0x30
#define CMD_SET_INFORMATINO_RESPONSE    0x31

/* Definition for some special usage */
#define EEPROM_SIZE         0x1000

/* Definition for Continuous Tx request */
#define ContTx_Start            0
#define ContTx_Stop         1

/* Definition flags for Continuous Tx mode */
#define ContTx_Normal           0
#define ContTx_CW           1
#define ContTx_CarrierSuppression   2

#define LONG_PREAMBLE           0
#define SHORT_PREAMBLE          1

//BssType
#define INDEPENDENT_BSS         0x0
#define INFRASTRUCTURE_BSS      0x1
#define PSEUDO_IBSS         0x3
#define AP_BSS              0x4

//RxFilter
#define AP_RX_FILTER            0x0400feff
#define STA_RX_FILTER           0x0000ffff

//#define ZDDEBUG
/* Macro definition */
#ifdef ZDDEBUG
#define ZDPRODUCTDBG(args...)           do { printk(KERN_DEBUG args); } while (0)
#else
#define ZDPRODUCTDBG(args...)           do { } while (0)
#endif

// EEPROM Memmory Map Region
#define     E2P_SUBID           0x0900
#define     E2P_POD             0x0904
#define     E2P_MACADDR_P1          0x0908
#define     E2P_MACADDR_P2          0x090C
#define     E2P_PWR_CAL_VALUE       0x0910
#define     E2P_PWR_INT_VALUE       0x0920
#define     E2P_ALLOWED_CHANNEL     0x0930
//#define     E2P_PHY_REG         0x0934
#define     E2P_FEATURE_BITMAP      0x0964
//#define     E2P_END             0x09FF

int zdproduction_ioctl(struct zd1205_private *macp, struct zd_point *p);

NDIS_STATUS ZD1205EM_Custom_QueryInformation(
PVOID NDIS_HANDLE,                                // IN
NDIS_OID Oid,                                     // IN
PVOID InformationBuffer,                          // IN
ULONG InformationBufferLength,                    // IN
PULONG BytesWritten,                              // OUT
PULONG BytesNeeded                                // OUT
);

NDIS_STATUS ZD1205EM_Custom_SetInformation(
PVOID NDIS_HANDLE,                                // IN
NDIS_OID Oid,                                     // IN
PVOID InformationBuffer,                          // IN
ULONG InformationBufferLength,                    // IN
PULONG BytesRead,                                 // OUT
PULONG BytesNeeded                                // OUT
);
#endif

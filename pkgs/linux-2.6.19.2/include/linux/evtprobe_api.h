/*
 *  This driver provides event API for the user space, based on netlink.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: evtprobe_api.h
 *  Creation date: 23/10/2008
 *  Author: Guillaume Chauvel, Sagemcom
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#ifndef __EVTPROBE_API_H__
#define __EVTPROBE_API_H__

/**
 * Events List by category
 */

typedef enum { 
   SYSM_N0=0, /*!< no event */

   /* Ethernet */
   SYSM_E1=1,
   SYSM_E2=2,
   SYSM_E3=3,

   /* Wi-Fi */
   SYSM_W1 = 4,
   SYSM_W2 = 5,
   SYSM_W3 = 6,
   SYSM_W4 = 7,

   /* Power */
   SYSM_P1=8, /*!< power button pressed ? */
   SYSM_P2=9,
   SYSM_P3=10,
   SYSM_P4=11,
   SYSM_P5=12,

   /* USB */
   SYSM_U1=13,
   SYSM_U2=14,

   /* Boot */
   SYSM_H1=15,

   /* Time */
   SYSM_T1=16,

   /* Alarm */
   SYSM_A1=17,

   /* Activity notification */
   SYSM_M1=18,

   /* Ethernet plugged */
   SYSM_G1=19,
   SYSM_G2=20,

   /* MMC */
   SYSM_C1=21,
   SYSM_C2=22,

   /* Sound: Headset plugged */
   SYSM_S1=23,

   /* Wireless Wide Area Network device (3G) plugged */
   SYSM_O1 = 24,

   /* Wireless Wide Area Network (3G) connected */
   SYSM_O4 = 25,

   /* Wireless Wide Area Network (3G) network type */
   SYSM_O3 = 26,

   /* Wireless Wide Area Network (3G) signal level */
   SYSM_O2 = 27,

   /* Kernel module trouble ; reboot event */
   SYSM_R1 = 28,

   /* ------------------------------------ */
   /*--- NB_EVENTS MUST BE the last symbol */
   NB_EVENTS
} sysm_eventid_t;

/**
 * MODULE ID
 * Module should send reboot event value as follow :
 * MODULE_ID + module's internel defect value
 * For instance the four Most significant bits would be used to define 15 module IDs
 */
#define K_MBX_EVENT_PROBE_ID	0x10000000
#define K_MMC_EVENT_PROBE_ID    0x20000000


typedef struct {
   unsigned long eventid; /*!< don't use 'sysm_eventid_t' enum here */
   long value;            /*!< associated value of the event or state */
} sysm_event_t;

typedef enum {
   E_EVT_FLG_STATEFUL, /*!< the designated item is a 'state' */
} event_flag_t;

typedef struct {
   sysm_eventid_t eventid;
   const char *  shortname;
   const char *  description;
   unsigned long flags; /*!< or-ed bits */
} event_description_t;

#define FLAG_BIT(b)  (1<<(b))


#if defined(EVTPROBE_WANT_DEBUG_STRINGS) && (EVTPROBE_WANT_DEBUG_STRINGS)
static const event_description_t event_description[NB_EVENTS] = {
	{SYSM_N0,	"N0",	"No or unknown event",			0},
	{SYSM_E1,	"E1",	"Ethernet interface up/down",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_E2,	"E2",	"IP address assigned",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_E3,	"E3",	"Internet connectivity",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_W1,	"W1",	"AP reachable",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_W2,	"W2",	"IP address assigned",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_W3,	"W3",	"Internet connectivity",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_W4,	"W4",	"Wi-Fi level",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_P1,	"P1",	"Power button",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_P2,	"P2",	"Battery level",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_P3,	"P3",	"Temp high limit reached",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_P4,	"P4",	"Craddle detection",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_P5,	"P5",	"Battery plugged",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_U1,	"U1",	"USB plugged",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_U2,	"U2",	"USB mounted",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_H1,	"H1",	"Maintenance requested at boot",	0},
	{SYSM_T1,	"T1",	"Time resync",				0},
	{SYSM_A1,	"A1",	"Alarm",				0},
	{SYSM_M1,	"M1",	"Activity notification",		0},
	{SYSM_G1,	"G1",	"Ethernet PHY1: cable plugged",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_G2,	"G2",	"Ethernet PHY2: cable plugged",		FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_C1,	"C1",	"MMC plugged",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_C2,	"C2",	"MMC mounted",				FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_S1,	"S1",	"Headset plugged",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_O1,	"O1",	"WWAN device plugged",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_O2,	"O2",	"WWAN signal level",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_O3,	"O3",	"WWAN network type",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_O4,	"O4",	"WWAN connected",			FLAG_BIT(E_EVT_FLG_STATEFUL)},
	{SYSM_R1,	"R1",	"Reboot Event",			FLAG_BIT(E_EVT_FLG_STATEFUL)}
};

/**
 * Gives the index of event.
 * @param event event number
 * @return index of event.
 * @retval 0 is an error.
 *
 */
static int sysm_get_event_index(sysm_eventid_t eventid)
{
   unsigned int i;
   int found=0;
   for (i=0 ; i<(sizeof(event_description)/sizeof(event_description[0])) ; i++) {
      if ( event_description[i].eventid == eventid) {
         found=i;
         break;
      }
   }
   return found ;
}

#endif /* EVTPROBE_WANT_DEBUG_STRINGS */

/*
 *	Events description
 */
#ifdef __KERNEL__
/**
 * Send a message using netlink facility
 *
 * @param eventid event or state number @see sysm_eventid_t
 * @param value value of event or state value
 * @param location is either GFP_KERNEL or GFP_ATOMIC if from interrupt context
 * @returb Operation status
 * @retval 0 success
 * @retval other are standard values.
 */
extern int send_netlink_msg(sysm_eventid_t eventid, long value, int location);
#endif /* __KERNEL__ */



#endif /* __EVTPROBE_API_H__ */

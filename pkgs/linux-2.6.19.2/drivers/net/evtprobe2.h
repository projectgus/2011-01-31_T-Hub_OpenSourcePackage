/***************************************************************************
 *            evtprobe.h
 *
 *  Mon Apr 30 16:24:11 2007
 *  Modified on Wed Oct 17 15:30:00 2007 by Julien Aubé
 *  Copyright  2007  Tristan LELONG
 *  Copyright  2007  Julien Aubé
 *  Email tristan.lelong@os4i.com
 *  Email julien.aube@os4i.com
 * 
 * This version adds:
 *  - Genericity against hardware version (Mediaphone/Homescreen/...)
 *  - Genericity against reporting mecanism ( Notify() or Netlink socket
 *  - Relocate the GPIO initialisations in the correct <plateform>_gpio.c .
 * 
 ****************************************************************************/

/* Choose 1 : either signal events using NETLINK or NOTIFY (d-bus/hal)  */
#define EVTPROBE_REPORT_NETLINK 1
//#define EVTPROBE_REPORT_NOTIFY  1

#define EVTPROBE_USE_KTHREAD 1
#define EVTPROBE_USE_IRQ     1
#define EVTPROBE_USE_IRQ_FOR_POWER_KEY 1

#ifndef NETLINK_EVTPROBE
#define NETLINK_EVTPROBE	29
#endif



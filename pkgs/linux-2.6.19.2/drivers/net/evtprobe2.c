/*
 *  Copyright 2005-2007 Sagem Communications. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * Includes
 */
#include <linux/kernel.h>  /* printk() */
#include <linux/module.h>  /* modules */
#include <linux/init.h>    /* module_{init,exit}() */
#include <linux/netlink.h> /* netlink socket */
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm-arm/arch-mxc/mxc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm-arm/arch-mxc/gpio.h>

#include "evtprobe2.h"

#define EVTPROBE_WANT_DEBUG_STRINGS (1) 
#include <linux/evtprobe_api.h>

#define MSGSIZE 1024
#define NETLINK_MESSAGE_SIZE 32

#if defined(EVTPROBE_REPORT_NETLINK)
/* netlink socket */
static struct sock * nl_sock;
#endif

#if defined(CONFIG_MACH_MX31HSV1)
static struct task_struct *th2   = NULL;
static spinlock_t detection_lock = SPIN_LOCK_UNLOCKED;
#endif

static int send_netlink_msg_internal(int index, long value, int location);

int kbd_pressed = 0;
long f_EVTPROBE_EventValue[NB_EVENTS];

#if defined(EVTPROBE_REPORT_NETLINK)
int send_netlink_msg(sysm_eventid_t eventid, long value, int location) {
   int z_iEvent;
   int ret = 0;
   int best_location = in_softirq() ? GFP_ATOMIC : GFP_KERNEL ;

   if (location != best_location) {
      printk( KERN_DEBUG "evtprobe: wrong location asked.\n");
   }

   z_iEvent = sysm_get_event_index(eventid) ;
   
   if ( z_iEvent <= 0  ) {
      return -EINVAL;
   }

   if (!nl_sock) {
      return -EUNATCH ;
   }

   /* unconditionnaly send message */
//   if ( f_EVTPROBE_EventValue[z_iEvent] != value ) {
      ret = send_netlink_msg_internal(z_iEvent, value, best_location);
//   } 

   return ret;
}

static int send_netlink_msg_internal(int index, long value, int location)
{
   struct sk_buff *z_skb;
   struct nlmsghdr *nlh = NULL;
   sysm_event_t z_msg;
   int z_iEvent;
   unsigned int size;
   static unsigned int seq = 0;

   z_iEvent = index ;

   if ( z_iEvent <= 0  ) {
      return -EINVAL;
   }

   /* store event value */
   f_EVTPROBE_EventValue[z_iEvent] = value;

   /* fill event structure */
   z_msg.eventid = event_description[z_iEvent].eventid ;
   z_msg.value = f_EVTPROBE_EventValue[z_iEvent];

   if ( (z_msg.eventid == SYSM_H1) && (kbd_pressed == 0) ) {
      kbd_pressed = 1;
   }

   size = NLMSG_SPACE(sizeof(z_msg));

   /* alloc skb */
   z_skb = alloc_skb(size, location);

   if ( ! z_skb ) {
      printk(KERN_ERR "evtprobe: skb_alloc() failed.\n");
      return -ENOMEM;
   } 
   else {
      /* NLMSG_PUT contains a hidden goto nlmsg_failure !!! */
      nlh = NLMSG_PUT(z_skb, 0 /*pid*/, seq++, NETLINK_EVTPROBE/*fake*/, sizeof(z_msg));
      
      memcpy(NLMSG_DATA(nlh), &z_msg, sizeof(z_msg));
      
      /* to mcast group 1<<0 */
      NETLINK_CB(z_skb).dst_group = 1;
      
      printk(KERN_DEBUG "evtprobe: event %s (%ld) = %ld sent\n",
            event_description[z_iEvent].shortname,
            z_msg.eventid,
            z_msg.value
            );

      /*multicast the message to all listening processes*/
      netlink_broadcast(nl_sock, z_skb, 0, 1, location);
   }
   return 0;

nlmsg_failure:
   printk(KERN_ERR "evtprobe: NLMSG operation failed.\n");
   kfree_skb(z_skb);
   return -ENOMEM;
}

void f_EVTPROBE_Callback (struct sock *sk, int len)
{
   int location = GFP_KERNEL; /* vs GFP_ATOMIC if from interrupt context */
   struct sk_buff *z_skb;
   struct nlmsghdr *nlh = NULL;
   int z_iEvent;
   u8 *message = NULL;
   int message_size;
   
   while ((z_skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
      /* process netlink message pointed by skb->data */
      nlh = (struct nlmsghdr *)z_skb->data;
      if(nlh) {
         message = NLMSG_DATA(nlh);
         message_size = nlh->nlmsg_len - NLMSG_LENGTH(0); /*!< message_size is ceiled because of alignment */
         if ( message_size ) {
            printk( KERN_DEBUG "evtprobe: '%c%c%c' received (len=%d)\n",
                  message[0], message[1], message[2], message_size);
         }
      }
      kfree_skb(z_skb);
   }
   // initialise netlink buffer
   for(z_iEvent=0 + 1; z_iEvent<ARRAY_SIZE(event_description); z_iEvent++) {
      if (  FLAG_BIT(E_EVT_FLG_STATEFUL) & event_description[z_iEvent].flags ) {
         send_netlink_msg_internal(
               z_iEvent, 
               f_EVTPROBE_EventValue[z_iEvent],
               location
               );
      }
   }
}

#endif //EVTPROBE_REPORT_NETLINK

/*
 * Init and Exit
 */
static int __init k_init2(void)
{
   printk(KERN_INFO "evtprobe: loading\n");

#if defined(EVTPROBE_REPORT_NETLINK)
   // initialise netlink socket with callback function
   nl_sock = netlink_kernel_create(NETLINK_EVTPROBE, 1, f_EVTPROBE_Callback, THIS_MODULE);
   if(!nl_sock) {
      printk(KERN_WARNING "evtprobe: cannot create NL socket\n");
   }
#endif

   printk(KERN_INFO "evtprobe: loaded with success\n");
   return 0;
}

static void __exit k_exit2(void)
{
#if defined(EVTPROBE_REPORT_NETLINK)
   sock_release(nl_sock->sk_socket);
#endif


   printk(KERN_INFO "evtprobe: successfully unloaded\n");
}

/* Module entry points */
module_init(k_init2);
module_exit(k_exit2);

#if defined(EVTPROBE_REPORT_NETLINK)
EXPORT_SYMBOL(nl_sock);
EXPORT_SYMBOL(send_netlink_msg);
#endif

MODULE_DESCRIPTION("evtprobe2: Send plateform kernel event to SyMon (v2)");
MODULE_AUTHOR("Sagem Communications");
MODULE_LICENSE("GPL");


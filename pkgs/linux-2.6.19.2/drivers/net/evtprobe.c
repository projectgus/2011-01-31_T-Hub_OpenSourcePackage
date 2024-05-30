/***************************************************************************
 *            evtprobe.c
 *
 *  Mon Apr 30 16:24:11 2007
 *  Copyright  2007  Tristan LELONG
 *  Email tristan.lelong@os4i.com
 ****************************************************************************/

#include "evtprobe.h"

MODULE_DESCRIPTION("evtprobe: send mediaphone kernel event to SyMon");
MODULE_AUTHOR("tristan LELONG");
MODULE_LICENSE("GPL");

// Module entry points
module_init(k_init);
module_exit(k_exit);

int irq_PE5;
int kbd_pressed = 0;
long f_EVTPROBE_EventValue[18];

int send_netlink_msg(int event, long value, int priority)	{
	if(event >= 0 && event < 18) { 
		f_EVTPROBE_EventValue[event] = value;
	}
	// initialise netlink buffer
	nl_buf = alloc_skb( MSGSIZE+32, priority);
	if(!nl_buf)	{
		printk(KERN_WARNING "evtprobe: error in skb_buf alloc \n");	
	}else	{
		// set message
		char msg[32];
		memcpy(msg,&event,sizeof(int));
		memcpy(msg+sizeof(int),&value,sizeof(long));
		
		memcpy(skb_put(nl_buf,sizeof(int)+sizeof(long)), msg, sizeof(int)+sizeof(long));
		
		// send current state
		netlink_broadcast(nl_sock, nl_buf, 0, 1, priority);
		switch(event)	{
			case 1:
				printk(KERN_INFO "evtprobe: event E1 (%d) = %ld sent\n",event,value);				
			break;
			case 2:
				printk(KERN_INFO "evtprobe: event E2 (%d) = %ld sent\n",event,value);				
			break;
			case 4:
				printk(KERN_INFO "evtprobe: event W1 (%d) = %ld sent\n",event,value);				
			break;
			case 5:
				printk(KERN_INFO "evtprobe: event W2 (%d) = %ld sent\n",event,value);				
			break;
			case 7:
				printk(KERN_INFO "evtprobe: event W4 (%d) = %ld sent\n",event,value);				
			break;
			case 8:
				printk(KERN_INFO "evtprobe: event P1 (%d) = %ld sent\n",event,value);				
			break;
			case 10:
				printk(KERN_INFO "evtprobe: event P3 (%d) = %ld sent\n",event,value);				
			break;
			case 11:
				printk(KERN_INFO "evtprobe: event P4 (%d) = %ld sent\n",event,value);				
			break;
			case 15:
				if(kbd_pressed == 0)	{
					printk(KERN_INFO "evtprobe: event H1 (%d) = %ld sent\n",event,value);				
					kbd_pressed = 1;
				}
			break;
			case 16:
				printk(KERN_INFO "evtprobe: event T1 (%d) = %ld sent\n",event,value);				
			break;
		}
	}
	return 0;
}

static int nl_thread(void * data) {
	int cpt = -1;
 	printk(KERN_INFO "evtprobe: starting thread NL\n");


	// thread loop
 	do {
		// deactivate keyboard debug msg for 1 sec
		if(kbd_pressed == 1)
			cpt++;
		if(cpt > 0)	{
			kbd_pressed = 0;
			cpt = -1;
		}

		//
		// DO WHATEVER YOU WANT
		//
		
		// wait X ms before checking again
		set_current_state(TASK_INTERRUPTIBLE);	
		schedule_timeout(1*HZ);
	} while (!kthread_should_stop());	

 	printk(KERN_INFO "evtprobe: end of thread NL\n");
 
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
static irqreturn_t k_irq_handler(int irq, void *dev_id)
#else
static irqreturn_t k_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
	int valPE5, valPC14;
	int event;
	long value;

	// read gpio values
	valPE5  = mxc_get_gpio_datain(PE5);
	valPC14 = mxc_get_gpio_datain(PC14);

	// reconfigure interruption to detect good trasition
	if(valPE5 == 0)
		gpio_set_irq_type(irq_PE5, IRQT_RISING);
      //gpio_config(4,5,false,GPIO_INT_RISE_EDGE);
	else
		gpio_set_irq_type(irq_PE5, IRQT_FALLING);
		//gpio_config(4,5,false,GPIO_INT_FALL_EDGE);

	// calculate corresponding event and value
	value = 0;		// default alim unplugged
	if(valPE5 == 1)	{	// if valPE5 => alim plugged
		value = 1;
	}

	event = 11;		// default craddle alim
	if(valPC14 == 1){	// else side alim
		event = 8;
	}

	// send netlink message
	send_netlink_msg(event,value, GFP_ATOMIC);

	return IRQ_HANDLED;
}

/*
 * Init and Exit
 */
 
void f_EVTPROBE_Callback (struct sock *sk, int len)
{
	int nl_group = 29;
	int nl_pid = 0;
	int priority = GFP_KERNEL;
	struct sk_buff *z_skb[18];
	struct nlmsghdr *nlh = NULL;
	char z_msg[32];
	int z_iEvent;
	u8 *payload = NULL;
	
	while ((z_skb[0] = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
		/* process netlink message pointed by skb->data */
		nlh = (struct nlmsghdr *)z_skb[0]->data;
		if(nlh) {
			payload = NLMSG_DATA(nlh);
		}
		kfree_skb(z_skb[0]);
	}
	// initialise netlink buffer
	for(z_iEvent=0;z_iEvent<18;z_iEvent++) {
		switch(z_iEvent) {
			case 2:
			case 4:
			case 5:
			case 8:
			case 11:
				z_skb[z_iEvent] = alloc_skb( MSGSIZE+32, priority);
				memcpy(z_msg,&z_iEvent,sizeof(int));
				memcpy(z_msg+sizeof(int),&f_EVTPROBE_EventValue[z_iEvent],sizeof(long));
				memcpy(skb_put(z_skb[z_iEvent],sizeof(int)+sizeof(long)), z_msg, sizeof(int)+sizeof(long));
				netlink_broadcast(nl_sock, z_skb[z_iEvent], 0, 1, priority);
				break;
			default:
				break;
		}
	}
}

static int __init k_init(void)
{
	int ret;
	int valPE5, valPC14;
	int event;
	long value;

	th = NULL;
	
	printk(KERN_INFO "evtprobe: loading\n");
	
	memset(f_EVTPROBE_EventValue,0,sizeof(f_EVTPROBE_EventValue));
	// init gpio driver
	mxc_gpio_init();

	// request PE5 & PC14 gpios
	ret = gpio_request_mux(PE5, GPIO_MUX_GPIO);
	if (ret < 0) {
		printk(KERN_WARNING "evtprobe: unable to register GPIO PE5\n");
	//	return ret;
	} else
		printk(KERN_WARNING "evtprobe: register GPIO PE5\n");

	ret = gpio_request_mux(PC14, GPIO_MUX_GPIO);
	if (ret < 0) {
		printk(KERN_WARNING "evtprobe: unable to register GPIO PC14\n");
	//	return ret;
	}else
		printk(KERN_WARNING "evtprobe: register GPIO PC14\n");

	// set direction : reading (input) = 1 |  writing (output) = 0
	mxc_set_gpio_direction(PE5,1);
	mxc_set_gpio_direction(PC14,1);

	// request associated irq
	ret = IOMUX_TO_IRQ(PE5);
	if(ret < 0) {
		printk(KERN_WARNING "evtprobe: unable to register GPIO_IRQ irq\n");
		return ret;
	} else	{
		irq_PE5 = ret;
	}
	
	ret = request_irq(irq_PE5, k_irq_handler, SA_SHIRQ, "GPIO", k_irq_handler);
	if (ret < 0) {
		printk(KERN_WARNING "evtprobe: unable to register GPIO_IRQ irq\n");
		return ret;
	}

	// configure interruption to detect good trasition
	if(mxc_get_gpio_datain(PE5) == 0)
		gpio_set_irq_type(irq_PE5, IRQT_RISING);
	else
		gpio_set_irq_type(irq_PE5, IRQT_FALLING);
	
   // initialise netlink socket with callback function
	nl_sock = netlink_kernel_create(NETLINK_EVTPROBE, 1, f_EVTPROBE_Callback, THIS_MODULE);
	if(!nl_sock)	{
		printk(KERN_WARNING "evtprobe: cannot create NL socket\n");
	}
	
	valPE5  = mxc_get_gpio_datain(PE5);
	valPC14 = mxc_get_gpio_datain(PC14);
	// calculate corresponding event and value
	value = 0;		// default alim unplugged
	if(valPE5 == 1)	{	// if valPE5 => alim plugged
		value = 1;
	}
	printk(KERN_INFO "evtprobe: value pour alim : %ld\n",value);
	event = 11;		// default craddle alim
	if(valPC14 == 1)	{	// else side alim
		event = 8;
	}
	f_EVTPROBE_EventValue[event] = value;
	
	th = kthread_run(nl_thread, NULL, "nl_th");

	printk(KERN_INFO "evtprobe: loaded with success\n");
	return 0;
}

//int request_irq(unsigned int irq, irqreturn_t (*handler)(int, void *, struct pt_regs *), unsigned long flags /* SA_SHIRQ */, const char *device, void *dev_id);
//void free_irq(unsigned int irq, void *dev_id);

static void __exit k_exit(void)
{
	kthread_stop(th);
	th = NULL;
	
	// close netlink socket
	sock_release(nl_sock->sk_socket);

	// free gpio irq
	free_irq(irq_PE5, k_irq_handler);

	gpio_free_mux(PE5);
	gpio_free_mux(PC14);
	
	printk(KERN_INFO "evtprobe: successfully unloaded\n");
}


	EXPORT_SYMBOL(nl_sock);
	EXPORT_SYMBOL(send_netlink_msg);

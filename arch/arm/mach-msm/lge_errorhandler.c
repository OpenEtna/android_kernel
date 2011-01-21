/****************************************************************************
 * Created by [bluerti@lge.com]
 * 2009-07-06
 * Made this file for implementing LGE Error Hanlder 
 * *************************************************************************/
#include "lge_errorhandler.h"

#include <mach/msm_smd.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>
#include <linux/io.h>
#include <linux/syscalls.h>

#include "smd_private.h"
#include "proc_comm.h"
#include "modem_notifier.h"

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

int LG_ErrorHandler_enable = 0;
static int user_keypress =0;

 int LGE_ErrorHandler_Main( int crash_side, char * message)
 {
	 char * kmem_buf;

	LG_ErrorHandler_enable = 1;

	/* LGE_CHANGE_S [EVE:FW:james.jang@lge.com] 2010-04-22, 
	   fixed bugs whitch raised other exception due to scheduling in LGE_ErrorHandler_Main().
	   set to enable preempt mode. */ 
  preempt_enable();
	raw_local_irq_enable();
	
	kmem_buf = kmalloc(LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN, GFP_ATOMIC);
	memcpy(kmem_buf, message, LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN);
	switch(crash_side) {
		case MODEM_CRASH:
			display_info_LCD(crash_side, message);
			//store_info_file(crash_side, kmem_buf);
			break;

		case APPL_CRASH:
			display_info_LCD(crash_side, message);
			break;


	}
	kfree(kmem_buf);
	raw_local_irq_disable();
#if 0	
	while(1)
	{
		if (user_keypress == 0x73) { // Volume up key 
			raw_local_irq_disable();
			return SMSM_SYSTEM_DOWNLOAD;
		} else if (user_keypress == 0x72) {  //Volume down key 
			raw_local_irq_disable();
			return SMSM_SYSTEM_REBOOT;
		}
		 mdelay(100);
	}
#else 
	while(1)
	{
		// 1. Check Volume Down Key 
		gpio_set_value(31,0);	//volume down
		gpio_set_value(32,1);	//volume up

		if(gpio_get_value(40)==0)
		{
			return SMSM_SYSTEM_REBOOT; //volume down key is pressed
		}
		mdelay(100);

		// 2. Check Volume Up Key 
		gpio_set_value(31,1);	//volume down
		gpio_set_value(32,0);	//volume up

		if(gpio_get_value(40)==0)
		{
			return SMSM_SYSTEM_DOWNLOAD; //volume down key is pressed
		}

		mdelay(100);
	}
#endif
	//raw_local_irq_disable();
 }

int display_info_LCD( int crash_side, char * message)
{
	unsigned short * buffer;
	
	buffer = kmalloc(LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN*sizeof(short), GFP_ATOMIC);


	if(buffer)
		expand_char_to_shrt(message,buffer);
	else
		printk("Memory Alloc failed!!\n");


	display_errorinfo_byLGE(crash_side, buffer,LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN );

	kfree(buffer);

	return 0;
}

int store_info_file(int crash_side, char * message)
{
	int fd = -1;
	int ret;
	if(crash_side ==MODEM_CRASH) {
		//fd = sys_open((const char __user *) LG_ERROR_FILE_ARM9, O_RDWR | O_CREAT, 0);	// Open file in T-Flash
		//if ( fd < 0) { 
		//	printk(KERN_WARNING "Warning: unable to open file!!.\n");
		
			fd =sys_open((const char __user *) LG_ERROR_DEFAULT_FILE, O_RDWR | O_CREAT, 0);
			if (fd <0) {
				printk(KERN_WARNING "Critical: unable to open file!!.\n");
				return -1;
			}
		//}
			
	} else if (crash_side == APPL_CRASH) {
			//Todo:: Implementing for Arm11 Crash case.

	}

	if (fd >= 0 ) {
		if ( (ret =sys_write(fd, (const char __user *)message, LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN) ) < 0)
			printk(KERN_WARNING " Can not write data to file!!, reason = %d \n",ret);

		sys_close(fd);
	}
	return 0;
}

void expand_char_to_shrt(char * message,unsigned short *buffer)
{
	char * src = message;
	unsigned char  * dst = (unsigned char *)buffer;
	int i=0;


	for(i=0;i<LGE_ERROR_MAX_ROW*LGE_ERROR_MAX_COLUMN; i++) {
		*dst++ = *src++;
		*dst++ = 0x0;
	}
	
}

void ramdump_reset_func(int key_value)
{
	printk("Ramdump_reset func , Key_value = %d\n",key_value);

	user_keypress =key_value;
	return;	
}


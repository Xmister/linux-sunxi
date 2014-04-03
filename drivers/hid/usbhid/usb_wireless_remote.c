/*
*************************************************************************************
*                         			      Linux
*					                 USB Host Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_wireless_remote.c
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-9-28            1.0          create this file
*
*************************************************************************************
*/
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

/*
 * Version Information
 */
#define DRIVER_VERSION  "20080411"
#define DRIVER_AUTHOR   "SoftWinner USB Developer"
#define DRIVER_DESC     "USB 2.4G Wireless Receiver"
#define DRIVER_LICENSE  "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

//------------------------------------------------
//
//------------------------------------------------


//------------------------------------------------
//
//------------------------------------------------
#define  USB_WIRELESS_REMOTE_EP_NUM     3

#define G_SENSOR_GRAVITY_EARTH          9806550
#define G_SENSOR_ABSMIN_2G              (-G_SENSOR_GRAVITY_EARTH * 2)
#define G_SENSOR_ABSMAX_2G              (G_SENSOR_GRAVITY_EARTH * 2)

//------------------------------------------------
//
//------------------------------------------------
struct usb_wireless_g_sensor {
	struct usb_device *usbdev;
	struct input_dev *dev;
	struct urb *irq;
	char name[128];
	char phys[64];

	int pipe;
	int maxp;

	signed char *data;
	unsigned int data_size;
	dma_addr_t data_dma;
};

struct usb_wireless_remote{
	struct usb_wireless_g_sensor g_sensor;
};

//------------------------------------------------
//
//------------------------------------------------
static struct usb_device_id usb_wireless_remote_id_table [] = {
	{ USB_DEVICE (0x1915, 0xAF11), },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_wireless_remote_id_table);

#if  0

static void wireless_g_sensor_irq(struct urb *urb)
{
	struct usb_wireless_g_sensor *g_sensor = urb->context;
	signed char *data = g_sensor->data;
	struct input_dev *dev = g_sensor->dev;
	int status;
	s16 x = 0;
	s16 y = 0;
	s16 z = 0;

	switch (urb->status) {
    	case 0:			/* success */
    	break;

    	case -ECONNRESET:	/* unlink */
    	case -ENOENT:
    	case -ESHUTDOWN:
    		return;

    	/* -EPIPE:  should clear the halt */
    	default:		/* error */
    		goto resubmit;
	}

	x = ((s16)(data[3] << 8) | (s16)data[4]);
	y = ((s16)(data[5] << 8) | (s16)data[6]);
	z = ((s16)(data[7] << 8) | (s16)data[8]);

	input_report_abs(dev, ABS_X, x);
	input_report_abs(dev, ABS_Y, y);
	input_report_abs(dev, ABS_Z, z);

	input_sync(dev);

	//printk("x:%d, y:%d, z:%d\n", x, y, z);

resubmit:
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		err ("can't resubmit intr, %s-%s/input0, status %d",
				g_sensor->usbdev->bus->bus_name,
				g_sensor->usbdev->devpath, status);
	}

	return;
}

#else

#define  USB_REMOTE_DELAY_TIME		2
static int remote_cnt = 0;
static void wireless_g_sensor_irq(struct urb *urb)
{
	struct usb_wireless_g_sensor *g_sensor = urb->context;
	signed char *data = g_sensor->data;
	struct input_dev *dev = g_sensor->dev;
	int status;
	s16 x = 0;
	s16 y = 0;
	s16 z = 0;

	switch (urb->status) {
    	case 0:			/* success */
    	break;

    	case -ECONNRESET:	/* unlink */
    	case -ENOENT:
    	case -ESHUTDOWN:
    		return;

    	/* -EPIPE:  should clear the halt */
    	default:		/* error */
    		goto resubmit;
	}

	remote_cnt++;
	if(remote_cnt > USB_REMOTE_DELAY_TIME){
		remote_cnt = 0;
		x = ((s16)(data[3] << 8) | (s16)data[4]);
		y = ((s16)(data[5] << 8) | (s16)data[6]);
		z = ((s16)(data[7] << 8) | (s16)data[8]);

		input_report_abs(dev, ABS_X, x);
		input_report_abs(dev, ABS_Y, y);
		input_report_abs(dev, ABS_Z, z);

		input_sync(dev);
	}
	
	//printk("x:%d, y:%d, z:%d\n", x, y, z);

resubmit:
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		err ("can't resubmit intr, %s-%s/input0, status %d",
				g_sensor->usbdev->bus->bus_name,
				g_sensor->usbdev->devpath, status);
	}

	return;
}

#endif

/*
*******************************************************************************
*                     usb_remote_open
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int wireless_g_sensor_open(struct input_dev *dev)
{
    struct usb_wireless_g_sensor *g_sensor = input_get_drvdata(dev);

    printk("open %s\n", g_sensor->name);

	g_sensor->irq->dev = g_sensor->usbdev;
	if(usb_submit_urb(g_sensor->irq, GFP_KERNEL)) {
	    err("err: usb_submit_urb failed\n");
		return -EIO;
    }

	return 0;
}

/*
*******************************************************************************
*                     usb_remote_close
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void wireless_g_sensor_close(struct input_dev *dev)
{
	struct usb_wireless_g_sensor *g_sensor = input_get_drvdata(dev);

	printk("close %s\n", g_sensor->name);

	usb_kill_urb(g_sensor->irq);

	return;
}

/*
*******************************************************************************
*                     init_wireless_g_sensor
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int wireless_g_sensor_init(struct usb_interface *intf,
                                  struct usb_device *dev,
                                  struct usb_endpoint_descriptor *endpoint,
                                  struct usb_wireless_g_sensor *g_sensor)
{
	int error = -ENOMEM;
	struct input_dev *input_dev = NULL;

    if(intf == NULL || dev == NULL || endpoint == NULL || g_sensor== NULL){
        err("err: invalid argument\n");
        return -EINVAL;
    }

	if(!usb_endpoint_is_int_in(endpoint)){
        err("err: ep is not interrupt endpoint\n");
		return -ENODEV;
    }

    /* alloc urb and initialize */
	g_sensor->pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	g_sensor->maxp = usb_maxpacket(dev, g_sensor->pipe, usb_pipeout(g_sensor->pipe));

    g_sensor->data_size = 29;
	g_sensor->data = usb_alloc_coherent(dev, g_sensor->data_size, GFP_ATOMIC, &g_sensor->data_dma);
	if(!g_sensor->data){
        err("err: usb_alloc_coherent failed\n");
		goto fail1;
    }

	g_sensor->irq = usb_alloc_urb(0, GFP_KERNEL);
	if(!g_sensor->irq){
        err("err: usb_alloc_urb failed\n");
		goto fail2;
    }

	g_sensor->usbdev = dev;

    /* alloc input_dev and initialize */
	input_dev = input_allocate_device();
	if(input_dev == NULL){
        err("err: input_allocate_device failed\n");
		goto fail3;
    }

	g_sensor->dev = input_dev;

#if 0
	if(dev->manufacturer){
		strlcpy(g_sensor->name, dev->manufacturer, sizeof(g_sensor->name));
    }

	if(dev->product){
		if(dev->manufacturer){
			strlcat(g_sensor->name, " ", sizeof(g_sensor->name));
	    }

		strlcat(g_sensor->name, dev->product, sizeof(g_sensor->name));
	}

	if(!strlen(g_sensor->name)){
		snprintf(g_sensor->name, sizeof(g_sensor->name),
    			 "USB G_Sensor %04x:%04x",
    			 le16_to_cpu(dev->descriptor.idVendor),
    			 le16_to_cpu(dev->descriptor.idProduct));
    }
#else
	snprintf(g_sensor->name, sizeof(g_sensor->name),"USBRemote_Sensor_%04x_%04x",
    			 le16_to_cpu(dev->descriptor.idVendor),
    			 le16_to_cpu(dev->descriptor.idProduct));
#endif

	usb_make_path(dev, g_sensor->phys, sizeof(g_sensor->phys));
	strlcat(g_sensor->phys, "/input0", sizeof(g_sensor->phys));

	input_dev->name = g_sensor->name;
	input_dev->phys = g_sensor->phys;
	usb_to_input_id(dev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_X, G_SENSOR_ABSMIN_2G, G_SENSOR_ABSMAX_2G, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, G_SENSOR_ABSMIN_2G, G_SENSOR_ABSMAX_2G, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, G_SENSOR_ABSMIN_2G, G_SENSOR_ABSMAX_2G, 0, 0);
	input_set_drvdata(input_dev, g_sensor);

	input_dev->open = wireless_g_sensor_open;
	input_dev->close = wireless_g_sensor_close;

    /* fill urb */
	usb_fill_int_urb(g_sensor->irq, dev, g_sensor->pipe, g_sensor->data,
			 (g_sensor->maxp > g_sensor->data_size ? g_sensor->data_size : g_sensor->maxp),
			 wireless_g_sensor_irq, g_sensor, endpoint->bInterval);
	g_sensor->irq->transfer_dma = g_sensor->data_dma;
	g_sensor->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    /* register input device */
	error = input_register_device(g_sensor->dev);
	if(error){
        err("err: input_register_device failed\n");
		goto fail4;
    }

	return 0;

fail4:
    input_free_device(input_dev);

fail3:
    usb_free_urb(g_sensor->irq);

fail2:
    usb_free_coherent(dev, g_sensor->data_size, g_sensor->data, g_sensor->data_dma);

fail1:
    return error;
}

/*
*******************************************************************************
*                     free_wireless_g_sensor
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int wireless_g_sensor_exit(struct usb_interface *intf,
                                  struct usb_device *dev,
                                  struct usb_wireless_g_sensor *g_sensor)
{
    if(intf == NULL || dev == NULL || g_sensor== NULL){
        err("err: invalid argument\n");
        return -EINVAL;
    }

    usb_kill_urb(g_sensor->irq);
    input_unregister_device(g_sensor->dev);
    usb_free_urb(g_sensor->irq);
    usb_free_coherent(interface_to_usbdev(intf), g_sensor->data_size, g_sensor->data, g_sensor->data_dma);

	return 0;
}

/*
*******************************************************************************
*                     usb_wireless_remote_probe
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int usb_wireless_remote_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface = NULL;
	struct usb_wireless_remote *remote = NULL;
	int ret = 0;

    /* check argment */
    if(intf == NULL){
        err("err: invalid argument\n");
        return -EINVAL;
    }

	interface = intf->cur_altsetting;

	printk("probe usb wireless remote\n");

    /* alloc wireless remote */
	remote = kzalloc(sizeof(struct usb_wireless_remote), GFP_KERNEL);
	if(remote == NULL){
	    err("err: kzalloc failed\n");
	    return -ENOMEM;
	}

	memset(remote, 0, sizeof(struct usb_wireless_remote));

    /* init g_sensor */
	ret = wireless_g_sensor_init(intf, dev, &interface->endpoint[0].desc, &remote->g_sensor);
	if(ret != 0){
	    err("err: kzalloc failed\n");
	    goto fail1;
	}

	usb_set_intfdata(intf, remote);

	return 0;

fail1:
    return -ENODEV;
}

/*
*******************************************************************************
*                     usb_wireless_remote_disconnect
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_wireless_remote_disconnect(struct usb_interface *intf)
{
	struct usb_wireless_remote *remote = usb_get_intfdata(intf);
	struct usb_device *dev = interface_to_usbdev(intf);

    usb_set_intfdata(intf, NULL);

    if (remote) {
		wireless_g_sensor_exit(intf, dev, &remote->g_sensor);
		kfree(remote);
	}

    return;
}

static struct usb_driver usb_wireless_remote_driver = {
	.name		= "usb_wireless_remote",
	.probe		= usb_wireless_remote_probe,
	.disconnect	= usb_wireless_remote_disconnect,
	.id_table	= usb_wireless_remote_id_table,
};

/*
*******************************************************************************
*                     usb_wireless_remote_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int __init usb_wireless_remote_init(void)
{
	int retval = usb_register(&usb_wireless_remote_driver);
	if (retval == 0) {
		printk(KERN_INFO KBUILD_MODNAME ": " DRIVER_VERSION ":"
				DRIVER_DESC "\n");
	}

	return retval;
}

/*
*******************************************************************************
*                     usb_wireless_remote_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void __exit usb_wireless_remote_exit(void)
{
	usb_deregister(&usb_wireless_remote_driver);
}

module_init(usb_wireless_remote_init);
module_exit(usb_wireless_remote_exit);
























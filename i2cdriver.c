
//---------------------------------------------------------------------------------------------//
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>          
#include <linux/uaccess.h>     
#include <linux/kthread.h>        
#include <linux/sched.h>  
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
//---------------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------------//
#define I2C_BUS_AVAILABLE   (          0)              
#define SLAVE_DEVICE_NAME   ( "ETX_I2C_DEVICE" )              
#define I2C_DEVICE_SLAVE_ADDR (       0x40)     
//---------------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------------//
dev_t dev = 0;

struct thread_data
{
 int value;
};

static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static struct i2c_client *etx_i2c_client_device = NULL; 
static struct i2c_adapter *etx_i2c_adapter = NULL; 
static void __exit etx_driver_exit(void);
static int __init etx_driver_init(void);
static struct task_struct *etx_thread;
static bool thread_running= false;
static struct class *dev_class;
static struct cdev etx_cdev;
static int g_speed=0;

int thread_function(void *pv);
//---------------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------------//
static int I2C_Write(unsigned char *buf, unsigned int len)
{
    int ret = i2c_master_send(etx_i2c_client_device, buf, len);   
    return ret;
}

static void I2C_Device_Write(unsigned char reg, unsigned char data)
{
    unsigned char buf[2] = {0};
    int ret;
    buf[0] = reg;
    buf[1] = data;
    ret = I2C_Write(buf, 2);
}

int thread_function(void *pv)
{
    thread_running = true;
    int speed;
    switch(g_speed)
    {
        case 0: speed = 0x20; break;   // CW low
        case 1: speed = 0x80; break;   // CW normal
        case 2: speed = 0xFF; break;   // CW fast
        case 3: speed = 0x20; break;   // CCW low
        case 4: speed = 0x80; break;   // CCW normal
        case 5: speed = 0xFF; break;   // CCW fast
    }
    switch(g_speed)
    {
        case 0:
        case 1:
        case 2:   // Clockwise
            while(!kthread_should_stop())
            {
                I2C_Device_Write(0x06,speed);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,0);
                msleep(10);

                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,speed);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,0);
                msleep(10);

                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,speed);
                I2C_Device_Write(0x12,0);
                msleep(10);

                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,speed);
                msleep(10);
            }
        break;

        case 3:
        case 4:
        case 5:   // Anti-clockwise
            while(!kthread_should_stop())
            {
                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,speed);
                msleep(10);

                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,speed);
                I2C_Device_Write(0x12,0);
                msleep(10);

                I2C_Device_Write(0x06,0);
                I2C_Device_Write(0x0A,speed);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,0);
                msleep(10);

                I2C_Device_Write(0x06,speed);
                I2C_Device_Write(0x0A,0);
                I2C_Device_Write(0x0E,0);
                I2C_Device_Write(0x12,0);
                msleep(10);
            }
        break;
    }
    return 0;
}

static int etx_I2C_dev_probe(struct i2c_client *client)                      
{
    pr_info("Probed!!\n"); 
    return 0;
}

static const struct i2c_device_id etx_oled_id[] = {
        { SLAVE_DEVICE_NAME, 0 },
        { }
};

MODULE_DEVICE_TABLE(i2c, etx_oled_id);

static struct i2c_driver etx_oled_driver = {
        .driver = {
            .name   = SLAVE_DEVICE_NAME,
            .owner  = THIS_MODULE,
        },
        .probe          = etx_I2C_dev_probe,
        //.remove         = etx_oled_remove,
        .id_table       = etx_oled_id,
};


static struct i2c_board_info oled_i2c_board_info = {
        I2C_BOARD_INFO(SLAVE_DEVICE_NAME, I2C_DEVICE_SLAVE_ADDR)
    };


static struct file_operations fops =
{
    .owner          = THIS_MODULE,
    .write          = etx_write,
};
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    char buffer[10] = {0};
    ssize_t result = copy_from_user(buffer, buf, sizeof(int));
    if (result > 0) 
    {
       pr_err("Failed to copy data\n");
    } 
    
    if(thread_running)
    {
        kthread_stop(etx_thread);
        thread_running = false;
    }
    etx_thread = kthread_run(thread_function,NULL,"eTx Thread");
    if(!etx_thread)
    {
        pr_err("Failed to create thread\n");
    }
    return len;
}
//---------------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------------//
static int __init etx_driver_init(void)
{
    etx_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
    if( etx_i2c_adapter != NULL )
    {
        etx_i2c_client_device = i2c_new_client_device(etx_i2c_adapter, &oled_i2c_board_info);
        if( etx_i2c_client_device != NULL )
        {
            i2c_add_driver(&etx_oled_driver);
        }
        i2c_put_adapter(etx_i2c_adapter);
    }
    if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0)
    {
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    cdev_init(&etx_cdev,&fops);
    if((cdev_add(&etx_cdev,dev,1)) < 0)
    {
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }
    if(IS_ERR(dev_class = class_create("etx_class")))
    {
        goto r_class;
    }
    if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"ko_communication")))
    {
        goto r_device;
    } 
    return 0;
 
    r_device:
            class_destroy(dev_class);
    r_class:
            unregister_chrdev_region(dev,1);
            cdev_del(&etx_cdev);
            return -1;
}

static void __exit etx_driver_exit(void)
{
    i2c_unregister_device(etx_i2c_client_device);
    i2c_del_driver(&etx_oled_driver);
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
}
//---------------------------------------------------------------------------------------------//

//---------------------------------------------------------------------------------------------//
module_init(etx_driver_init);
module_exit(etx_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Akash B");
MODULE_DESCRIPTION("I2C Stepper Motor Driver using PCA9685");
MODULE_VERSION("1.0");
//--------------------------------------<-------------------->--------------------------------//


#ifndef __td3_i2c_dev_LIB_H
#define __td3_i2c_dev_LIB_H

#include <asm-generic/errno.h>
#include <asm-generic/errno-base.h>
#include <linux/cdev.h>              
#include <linux/fs.h>               
#include <linux/module.h>           
#include <linux/uaccess.h>          
#include <linux/of_address.h>       
#include <linux/platform_device.h>  
#include <linux/of.h>               
#include <linux/io.h>               
#include <linux/interrupt.h>        
#include <linux/delay.h>            
#include <linux/types.h>            
#include <linux/init.h>             
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include "../inc/BBB_I2C_reg.h"
#include "../inc/MPU6050.h"

#define NUM_MENOR   0
#define CANT_DISP   1
#define CLASS_NAME  "td3_ani_class"
#define COMPATIBLE  "td3_ani"

#define XRDY        0
#define ARDY        1
#define RRDY        2

#define NONE        0
#define CHIPID      _IO('N', 0x44)

#define OWN_ADDR    0xAA

static int __init i2c_init(void);
static void __exit i2c_exit(void);
static int change_permission_cdev(struct device *dev,struct kobj_uevent_env *env);

//FOPS
static int     my_open  (struct inode *inode, struct file *file);
static int     my_close (struct inode *inode, struct file *file);
static ssize_t my_read  (struct file *device_descriptor, char *buff, size_t dataLength, loff_t *offset);
//static ssize_t my_write (struct file *device_descriptor, const char *buff, size_t data_length, loff_t *offset);
//static long    my_ioctl (struct file *device_descriptor, unsigned int command, unsigned long arguments);


//Platform device
static int  i2c_probe      (struct platform_device* i2c_pd);
static int  i2c_remove     (struct platform_device* i2c_pd);
irqreturn_t Myi2cHandlerIRQ  (int IRQ, void *ID, struct pt_regs *REG);


static void i2c_softreset(void); 


/*
Volatile: prevents the compiler from applying any optimizations on objects that can change in ways that cannot be determined by the compiler. 
Objects declared as volatile are omitted from optimization because their values can be changed by code outside the scope of current code at any time. 
*/

volatile int I2CWakeUpCondition_Tx = 0;
volatile int I2CWakeUpCondition_Rx = 0;

/* Kernel sync primitive that allows a process to sleep until a specific condition is met and its initialization*/
wait_queue_head_t I2CWakeUp = __WAIT_QUEUE_HEAD_INITIALIZER(I2CWakeUp);

uint32_t        MPU6050_read    (uint32_t reg2Read);
uint32_t        MPU6050_write   (uint32_t reg2Write, uint32_t data);
uint32_t        MPU6050_init    (void);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Ana Nunez");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("TD3_MYI2C");


static struct 
{
    dev_t myi2c;                    // tipo de char device

    struct cdev * myi2c_cdev;       // necesarias para que cuando el sistema arranque matchee al device tree
    struct device * myi2c_device;
    struct class * myi2c_class;
} state;


/* Estructura of_device_id */
//Matcheo el device con el driver utilizando la propiedad "compatible" del devicetree
//así puedo ejecutar la función probe()

static struct of_device_id i2c_of_device_ids[] =
{
    {
        .compatible = COMPATIBLE,
    },
    {}
};

//Función que expone el ID table del driver, que describe qué devices soporta
//Ahora el driver es DT-compatble

MODULE_DEVICE_TABLE(of, i2c_of_device_ids);

//Así linkeo las funciones comunes con las que va a utilizar el driver
static struct file_operations i2c_ops = 
{
    .owner      = THIS_MODULE,
    .open       = my_open,
    .read       = my_read,
    //.write      = my_write,
    .release    = my_close
};


static struct platform_driver MyPlatformDriver =
{
    .probe  = i2c_probe,        // se ejecuta como evento a la llamada del platform driver register
    .remove = i2c_remove,       // desasociacion del platform driver 
    .driver =
                {
                    .name = COMPATIBLE,                                     
                    .of_match_table = of_match_ptr(i2c_of_device_ids),
                },
};

static void __iomem *I2C_ModuleInit, *CLK_ModuleInit, *CTRL_ModuleInit;

int virq;

static struct
{
    /*Variables de Rx*/
    uint8_t     * I2CRxData;
    uint32_t    I2CRxDataLen;
    uint32_t    I2CRxDataIndex;

    /*Variables de Tx*/
    uint8_t     * I2CTxData;
    uint32_t    I2CTxDataLen;
    uint32_t    I2CTxDataIndex;
} I2CData;

#endif
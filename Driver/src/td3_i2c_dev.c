#include "../inc/BBB_I2C_reg.h"
#include "../inc/MPU6050.h"
#include "../inc/td3_i2c_dev.h"

/*---------------------------------------------------------------------- INIT -------------------------------------------------------------------------------------*/


/* Método init */
/* Inicializa todo, se ejecuta al llamar a insmod */
/* Pasos que tengo que seguir:
    - Pedir major                       --> ok
    - Inicializo y registro el disp     --> ok
    - Crear clase                       --> ok   
    - Crear el device                   --> ok
    - Registrar platform                --> ok
*/

static int __init i2c_init(void)
{
    int status = 0;
    pr_info("[LOG] INIT: Set CharDriver\n");


    /* Reserving memory for char device */

    if((state.myi2c_cdev = cdev_alloc()) == NULL)// pide memoria y se la asigna al puntero
    {
        pr_err("[ERROR] INIT: Unable to alloc\n");
        return status;
    }
    pr_info("[LOG] INIT: cdev_alloc ok!\n");


    /* Reserving a char devices range. The major number is dynamically assigned */

    /*
        The major number identifies the driver associated with the device The major number is to identify the corresponding driver. 
        Many devices may use the same major number. So we need to assign a minor number to each device which is using the same major number.
    */

    // el alloc chrdev reg no trabaja directamente con el dev tree
    if((status = alloc_chrdev_region(&state.myi2c, NUM_MENOR, CANT_DISP, COMPATIBLE)) < 0) //le pasamos la cantidad de dispositivos en menor y nos tiene que devolver el numero mayor asignado para ese device tree
    {
        pr_err("[ERROR] INIT: alloc_chrdev_region failed \n");
        return status;
    }

    pr_info("[LOG] INIT: Major number asssigned %d 0x%X\n", MAJOR(state.myi2c), MAJOR(state.myi2c)); //lo logueamos


    /* Initialize char device */

    cdev_init(state.myi2c_cdev, &i2c_ops);  //inicializacion de cdev

    pr_info("[LOG] INIT: cdev_init ok!\n");


    /* Adding char device to system */

    if((status = cdev_add(state.myi2c_cdev, state.myi2c, CANT_DISP)) < 0)
    {
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        pr_err("[ERROR] INIT: Unable to register device \n");
        return status;
    }

    pr_info("[LOG] INIT: cdev_add ok!\n");


    /* Creating class */

    /* Creo la clase visible en /sys/class */
    /* Me devuelve puntero a struct class */


    if((state.myi2c_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL) // Creacion de la clase de dispositivo
    {
        /* Removing char device from system*/
        cdev_del(state.myi2c_cdev);

        /* Unregistering the amount of registered devices in the system */ 
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        
        pr_err("[ERROR] INIT: Unable to create device class \n");
            
        return EFAULT;
    }

    pr_info("[LOG] INIT: class_create ok!\n");


    /* Changing permissions /dev/td3_ani */

    state.myi2c_class->dev_uevent = change_permission_cdev; 


    /* Creating device */

    /* Creo el device node dentro de la clase, registra en sysfs */
    /* /dev */
    //parent -> NULL
    //sin data adicional -> NULL

    if((device_create(state.myi2c_class, NULL, state.myi2c, NULL, COMPATIBLE)) == NULL)
    {
        /* Deleting char device from the system */
        cdev_del(state.myi2c_cdev);

        /* Unregistering the amount of registered devices in the system */
        unregister_chrdev_region(state.myi2c, CANT_DISP);

        /* Removing registered device from the system */
        class_destroy(state.myi2c_class);
        
        pr_err("[ERROR] INIT: Unable to create device\n");
        
        return EFAULT;
    }

    pr_info("[LOG] INIT: device_create ok!\n");    


    /* Registering my platform driver */

    /* Registro el driver, una vez realizado esto ingresa a la función probe */

    if((status = platform_driver_register(&MyPlatformDriver)) < 0)
    {
        /* Deleting char device from the system */
        cdev_del(state.myi2c_cdev);

        /* Unregistering the amount of registered devices in the system */
        unregister_chrdev_region(state.myi2c, CANT_DISP);

        /* Removing registered device from the system */
        device_destroy(state.myi2c_class, state.myi2c);

        /* Deleting the class */
        class_destroy(state.myi2c_class);

        pr_err("[ERROR] INIT: Unable to register platform device \n");

        return status;
    }

    pr_info("[LOG] INIT: Module initialized correctly \n");

return 0;

}

static int change_permission_cdev(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVICE=%#o", 0666);
    return 0;
}
/*

sudo chmod 666 /dev/td3_ani 

*/


/*---------------------------------------------------------------------- EXIT -------------------------------------------------------------------------------------*/

/* Remueve todo, se ejecuta al llamar a rmmod */
// Tengo que hacer todo lo de init al revés

static void __exit i2c_exit(void)
{
    pr_info("[LOG] EXIT: Removing module \n");
    cdev_del(state.myi2c_cdev);

    pr_info("[LOG] EXIT: Unregistering devices \n");
    unregister_chrdev_region(state.myi2c, CANT_DISP);

    pr_info("[LOG] EXIT: Removing registered device from the system\n");
    device_destroy(state.myi2c_class, state.myi2c);

    pr_info("[LOG] EXIT: Deleting the class\n");
    class_destroy(state.myi2c_class);

    pr_info("[LOG] EXIT: Unregistering platform device \n");
    platform_driver_unregister(&MyPlatformDriver);

    pr_info("[EXLOGIT] EXIT: Module removed\n");
}

module_init(i2c_init);
module_exit(i2c_exit);


/*---------------------------------------------------------------------- HANDLER IRQ -------------------------------------------------------------------------------------*/

irqreturn_t Myi2cHandlerIRQ(int IRQ, void *ID, struct pt_regs *REG)
{
    static uint32_t aux_val;

    // Read interrupt status
    aux_val = ioread32(I2C_ModuleInit + I2C_IRQSTATUS);

    /* Leo si me llego una interrupccion... por a
        
        Bit 2 ARDY:
        Ver Pagina 4611
    */

    if(aux_val & ARDY_IE)    //  
    {        
        
        iowrite32(ARDY_IE, I2C_ModuleInit + I2C_IRQSTATUS);

        iowrite32(ARDY_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);

        /* Wake up the process for the scheduler to execute */
        I2CWakeUpCondition_Tx = 1;
        wake_up(&I2CWakeUp);
    }


    /* Leo si me llego una interrupccion... por recepcion
        Receive mode only (I2C mode)
        Bit 3 RRDY:
                0h = No data available
                1h = Receive data available
        Ver Pagina 4610
    */


    if(aux_val & RRDY_IE) //Rx Interrupt
    {

        /* Read data received from I2C module */
        
        // Leo de a un Byte e incremento

        I2CData.I2CRxData[ I2CData.I2CRxDataIndex] = ioread32(I2C_ModuleInit + I2C_DATA);
        I2CData.I2CRxDataIndex++;


        // Si ya lei todo lo que tenia que leer, limpio los flags de interrupcion, deshabilito la interrupcion y hago un wakeup

        if(I2CData.I2CRxDataIndex == I2CData.I2CRxDataLen)
        {
            /* Clear the interruption by writing the register */
            iowrite32(RRDY_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);

            /* Wake up the process for the scheduler to execute */
            I2CWakeUpCondition_Rx = 1;
            wake_up_interruptible(&I2CWakeUp);


            I2CData.I2CRxDataIndex = 0;
        }

        /* Clearing flags */
        iowrite32(RRDY_IE, I2C_ModuleInit + I2C_IRQSTATUS);
    }

   /* Leo si me llego una interrupccion... por Transmision
        
        Transmit data ready IRQ status.
        Bit 4 XRDY:
            0h = Transmission ongoing
            1h = Transmit data ready
        Set to '1' by core when transmitter and when new data is requested.
        When set to '1' by core, an interrupt is signaled to MPUSS.
        Write '1' to clear.

        Ver Pagina 4610
   */
    if(aux_val & XRDY_IE)
    {
        /* Sending data to I2C module */
        iowrite32(I2CData.I2CTxData[I2CData.I2CTxDataIndex], I2C_ModuleInit + I2C_DATA);
        
        I2CData.I2CTxDataIndex++;

        if(I2CData.I2CTxDataIndex == I2CData.I2CTxDataLen)
        {
            /* Clear the interruption by writing the register */
            iowrite32(XRDY_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);

        }    

        /* Clearing flags */
        iowrite32(XRDY_IE, I2C_ModuleInit + I2C_IRQSTATUS);
    }

   return IRQ_HANDLED;
}

/*---------------------------------------------------------------------- PROBE -------------------------------------------------------------------------------------*/

/* Para cuando alguien inserta o reclama el device */

static int i2c_probe(struct platform_device* i2c_pd)
{
    uint32_t aux_val;

    pr_info("[LOG] PROBE: Entre al PROBE\n");

    /*
      Los registros que vamos a usar deben ser mapeados antes que tiren 
      data abort o prefecth abort
   */


    /* Mapping base address of I2C module */

    if((I2C_ModuleInit = ioremap(I2C2_REG, I2C2_REG_LEN)) == NULL)
    {
        pr_err("[ERROR] PROBE: I2C2 register could not be mapped\n");
        
        return -1;
    }

    pr_info("[LOG] PROBE: Mapped register = 0x%x\n", (uint32_t)I2C_ModuleInit);
    
    
    /* Mapping base address of clock registers */

    if((CLK_ModuleInit = ioremap(CM_PER_REG, CM_PER_REG_LEN)) == NULL)  //Mapeo el registro CM_PER
    {
        pr_alert("[ERROR] PROBE: CM_PER register could not be mapped\n");
        
        iounmap(I2C_ModuleInit);
        
        return -1;
    }

    pr_info("[LOG] PROBE: Mapped register = 0x%x\n", (uint32_t)CLK_ModuleInit);


    /* Mapping base address of control module */

    if((CTRL_ModuleInit = ioremap(CTRL_MODULE_REG, CTRL_MODULE_REG_LEN)) == NULL) //Mapeo el registro CONTROL MODULE
    {
        pr_alert("[ERROR] PROBE: CTRL_MODULE register could not be mappedn");
        
        iounmap(I2C_ModuleInit);
        iounmap(CLK_ModuleInit);
        
        return -1;
    }
    pr_info("[LOG] PROBE: Mapped register = 0x%x\n", (uint32_t)CTRL_ModuleInit);

    
    /*
        Enabling I2c2 clock
        Bit 1-0 MODULEMODE
            Control the way mandatory clocks are managed.
            0x0 = DISABLED : Module is disable by SW. Any OCP access to module results in an error, except if resulting from a module wakeup (asynchronous wakeup).
            0x1 = RESERVED_1 : Reserved
            0x2 = ENABLE 
            0x3 = RESERVED : Reserved
        Ver Pagina 1267
    */

    pr_info("[LOG] PROBE: Reading...\n");
    aux_val = ioread32(CLK_ModuleInit + CM_PER_I2C2_CLKCTRL);
    aux_val |= 0x02;

    pr_info("[LOG] PROBE: Writing...\n");
    iowrite32(aux_val, CLK_ModuleInit + CM_PER_I2C2_CLKCTRL);
    
    pr_info("[LOG] PROBE: Sucesfully written \n");
    msleep(100);

    aux_val = 0x0; // por las dudas
    aux_val = ioread32(CLK_ModuleInit + CM_PER_I2C2_CLKCTRL); // Read the register that was just written
    pr_info("[PROBE] \n");
    
    if((aux_val & 0x03) != 0x02)
    {
        /* If the register doesn't have the expected value */
        pr_err("[ERROR] PROBE: Error writing I2C2 CLK register\n");
        iounmap(I2C_ModuleInit);
        iounmap(CLK_ModuleInit);
        iounmap(CTRL_ModuleInit);
        return -1;
    }

    
    /*
        Obtaining irq number from the device tree
    */
    
    if((virq = platform_get_irq(i2c_pd, 0)) < 0)
    {
        pr_err("[ERROR] PROBE: Could not get virtual interrupt request\n");
        iounmap(I2C_ModuleInit);
        iounmap(CLK_ModuleInit);
        iounmap(CTRL_ModuleInit);
        return -1;
    }

    /* Implanto el handler de IRQ */
	/* Asocio handler con interrupción */

    // Debo indicar el handler a llamar
    // Los flags de la interrupción -> sólo le pongo flanco ascendente
    // Nombre del device

    if(request_irq(virq, (irq_handler_t)Myi2cHandlerIRQ, IRQF_TRIGGER_RISING, COMPATIBLE, NULL))
    {
        pr_err("[ERROR] PROBE: TD3_I2C Could not assign virtual interrupt request\n");
        iounmap(I2C_ModuleInit);
        iounmap(CLK_ModuleInit);
        iounmap(CTRL_ModuleInit);
        return -1;
    }
    
    pr_info("[LOG] PROBE: Probe OK!\n");
    return 0;
}


/*---------------------------------------------------------------------- REMOVE -------------------------------------------------------------------------------------*/

static int i2c_remove(struct platform_device * i2c_pd)
{
    int status = 0;

    free_irq(virq, NULL);

    iounmap(I2C_ModuleInit);
    iounmap(CLK_ModuleInit);
    iounmap(CTRL_ModuleInit);
    
    pr_info("[LOG] REMOVE: Removed OK!\n");
    return status;
}




/*---------------------------------------------------------------------- SENSOR INIT -------------------------------------------------------------------------------------*/

uint32_t MPU6050_init(void)
{
    uint32_t aux = 0;
    uint32_t who_am_i_reg;

    pr_info("[LOG] MPU6050 INIT: Starting MPU6050 initialization\n"); 

    // Leo el device ID

    if((who_am_i_reg = MPU6050_read(WHO_AM_I)) != SLAVE_ADDRESS)
    {
        pr_err("[ERROR] MPU6050 INIT: Device ID different from Slave Addr. \n");
        return -1;
    }

    pr_info("[LOG] MPU6050 INIT: Device ID = 0x%x\n", who_am_i_reg);

    // Wakes up device from sleep mode
    MPU6050_write(PWR_MGMT_1, 0x0);  // Power Up device

    if((aux = MPU6050_read(PWR_MGMT_1)) != 0)
    {
        pr_err("[ERROR] MPU6050 INIT: Failed to Power up device \n");
        return -1;
    }


    /*
        Configuring gyro and accelerometer filter
        Accelerometer Fs=1Khz  |            Gyroscope
        Bandwidth(Hz) Delay(ms)| Bandwidth(Hz) Delay(ms) Fs(kHz)
        260            0       |       256       0.98         8
        Ver Pagina 13.
    */

    /*
    
    GYRO CONFIG:
        - Bits [0:2]: Reserved   
        - Bits [3:4] - FS_SEL (Full Scale Selection): Sets the gyroscope's full-scale range:  
            - 00: ±250°/s  
            - 01: ±500°/s  
            - 10: ±1000°/s  
            - 11: ±2000°/s  
        - Bit 5 - ZG_ST (Z-axis Gyroscope Self-Test):  
            - Set to 1 to enable self-test for the Z-axis gyroscope.  
            - Set to 0 to disable self-test.  
        - Bit 6 - YG_ST (Y-axis Gyroscope Self-Test):  
            - Set to 1 to enable self-test for the Y-axis gyroscope.  
            - Set to 0 to disable self-test.  
        - Bit 7 - XG_ST (X-axis Gyroscope Self-Test):  
            - Set to 1 to enable self-test for the X-axis gyroscope.  
            - Set to 0 to disable self-test.  
    
    */

    MPU6050_write(GYRO_CONFIG, 0x10);  // Configs 1000°/s
    aux = MPU6050_read(GYRO_CONFIG);

    pr_info("[LOG] MPU6050 INIT: GYRO_CONFIG = 0x%x", aux);

    /*
    
    ACCEL_CONFIG 
        - Bits [0:2]: Reserved   
        - Bits [3:4] - AFS_SEL (Accelerometer Full Scale Selection): Sets the accelerometer's full-scale range:  
            - 00: ±2g  
            - 01: ±4g  
            - 10: ±8g  
            - 11: ±16g  
        - Bit 5 - ZA_ST (Z-axis Accelerometer Self-Test):  
            - Set to 1 to enable self-test for the Z-axis accelerometer.  
            - Set to 0 to disable self-test.  
        - Bit 6 - YA_ST (Y-axis Accelerometer Self-Test):  
            - Set to 1 to enable self-test for the Y-axis accelerometer.  
            - Set to 0 to disable self-test.  
        - Bit 7 - XA_ST (X-axis Accelerometer Self-Test):  
            - Set to 1 to enable self-test for the X-axis accelerometer.  
            - Set to 0 to disable self-test.  

    */

    MPU6050_write(ACCEL_CONFIG, 0x00);  // Config 2G
    aux = MPU6050_read(ACCEL_CONFIG);

    pr_info("[LOG] MPU6050 INIT: ACCEL_CONFIG = 0x%x", aux);

    /*

    INT_ENABLE 
        - Bit 0 - DATA_RDY_EN (Data Ready Interrupt Enable):  
            - Set to 1 to enable an interrupt when new sensor data is available.  
            - Set to 0 to disable this interrupt.  

        - Bit 3 - I2C_MST_INT_EN (I²C Master Interrupt Enable):  
            - Set to 1 to enable interrupts from I²C Master operations.  
            - Set to 0 to disable these interrupts.  

        - Bit 4 - FIFO_OFLOW_EN (FIFO Overflow Interrupt Enable):  
            - Set to 1 to enable an interrupt when the FIFO buffer overflows.  
            - Set to 0 to disable this interrupt.  
    
    */

    pr_info("[LOG] MPU6050 INIT: Configuration OK!\n");
    
    return 0;
}

/*---------------------------------------------------------------------- SENSOR READ -------------------------------------------------------------------------------------*/

// Ver pag 37 MPU

uint32_t MPU6050_read(uint32_t reg2Read)
{
    uint32_t aux_val = 0, ret_val = 0;

    i2c_softreset();


    iowrite32(I2C_ALL_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);  // Disables all i2c interrupts
    iowrite32(I2C_ALL_IE, I2C_ModuleInit + I2C_IRQSTATUS);      // Clears any pending interrupt flags

    // Me fijo que no hayan interrupciones pendientes
    
    aux_val = ioread32(I2C_ModuleInit + I2C_IRQSTATUS_RAW);

	if((aux_val & 0x1000) != 0) 
    {
        pr_err("[ERROR] MPU6050 READ: IRQ not clear \n");
		return -1;
	}

    // Seteo las condiciones iniciales

    I2CData.I2CTxDataLen    = 1;
    I2CData.I2CRxDataLen    = 1;

    I2CData.I2CTxDataIndex  = 0;
    I2CData.I2CRxDataIndex  = 0;
    
    I2CWakeUpCondition_Rx = 0;
    I2CWakeUpCondition_Tx = 0;


    // Configuro el i2c como master transmitter
    /*
        Master mode: MST = 1
        Transmitter mode: TRX = 1
    */

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val = aux_val | I2C_CON_TRX | I2C_CON_MST;  
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);
    
    // Activo ARDY (Register access ready interrupt) y XRDY (Transmit data ready interrupt) 

    aux_val = ioread32(I2C_ModuleInit + I2C_IRQENABLE_SET);
    aux_val = aux_val | XRDY_IE | ARDY_IE;
    iowrite32(aux_val, I2C_ModuleInit + I2C_IRQENABLE_SET);

    // Seteo el address del device

    iowrite32(OWN_ADDRESS, I2C_ModuleInit + I2C_OA);        // Address del controlador de i2c
	iowrite32(SLAVE_ADDRESS, I2C_ModuleInit + I2C_SA);      // MPU6050 i2c address

    // Set un byte de Tx

    iowrite32(I2CData.I2CTxDataLen, I2C_ModuleInit + I2C_CNT);     // Data Count to transmit - 1 Byte
    iowrite32(reg2Read, I2C_ModuleInit + I2C_DATA);
    
    // Pido memoria para el buffer de transmision

    
    if((I2CData.I2CTxData = (uint8_t *)kmalloc(I2CData.I2CTxDataLen, GFP_KERNEL)) == NULL) 
    {
		pr_err("[ERROR] MPU6050 READ: Requesting allocation memory for Tx buffer \n");
		return -1;
	}


    /* 
        Empiezo la transmision

        I2C_CON_START: Start
        I2C_CON_STOP: Stop
    */

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val = aux_val | I2C_CON_START | I2C_CON_STOP;      
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

    // Espero el flag de Tx
    wait_event_interruptible(I2CWakeUp, I2CWakeUpCondition_Tx > 0);

    /*
        Paso a modo lectura, para poder escribir tengo que hacer un stop y cambiar la configuracion
    */

    I2CWakeUpCondition_Rx   = 0;
    I2CWakeUpCondition_Tx   = 0;
    I2CData.I2CTxDataIndex  = 0;

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val |= I2C_CON_STOP;                         // Envio el stop
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);


    // Configuro el i2c como master receiver
    /*
        Master mode: MST = 1
        Transmitter mode: TRX = 0
    */
    
    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val |= I2C_CON_MST;
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
	aux_val = aux_val & (~I2C_CON_TRX);
	iowrite32(aux_val, I2C_ModuleInit + I2C_CON);
	
    // Deshabilito XRDY IRQ y habilito RRDY IRQ

    iowrite32(XRDY_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);  // Disable XRDY IRQ
    iowrite32(RRDY_IE, I2C_ModuleInit + I2C_IRQENABLE_SET);  // Enable  RRDY IRQ
    
    // Repito un proceso similar al de antes

	iowrite32(I2CData.I2CRxDataLen, I2C_ModuleInit + I2C_CNT); // Data Count to receive - 1 Byte

	
    if((I2CData.I2CRxData = (uint8_t *)kmalloc(I2CData.I2CRxDataLen, GFP_KERNEL)) == NULL) 
    {
		pr_err("[ERROR] MPU6050 READ: Requesting allocation memory for Rx buffer \n");
		return -1;
	}

    /* 
        Empiezo la transmision

        I2C_CON_START: Start
        I2C_CON_STOP: Stop
    */
	
    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val = aux_val | I2C_CON_START | I2C_CON_STOP; 
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

    // Espero el flag de Rx
    wait_event_interruptible(I2CWakeUp, I2CWakeUpCondition_Rx > 0);

    /* Una vez que leo, libero/limpio todo */
    
    I2CWakeUpCondition_Rx   = 0;
    I2CWakeUpCondition_Tx   = 0;
    I2CData.I2CRxDataIndex  = 0;

    // Envio stop para dar por finalizado el intercambio

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val |= I2C_CON_STOP;
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

	ret_val = *I2CData.I2CRxData;

	kfree(I2CData.I2CRxData);
    kfree(I2CData.I2CTxData);
	
    iowrite32(I2C_ALL_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);    // Limpio todas las interrupciones
	    
	return ret_val;

}

/*---------------------------------------------------------------------- SENSOR WRITE -------------------------------------------------------------------------------------*/

// Ver pag 36 MPU

uint32_t MPU6050_write(uint32_t reg2Write, uint32_t data)
{
    uint32_t aux_val = 0;

    i2c_softreset();


    //iowrite32(0xFFFF, I2C_ModuleInit + I2C_IRQENABLE_CLR);  // Disables all i2c interrupts
    iowrite32(0xFFFF, I2C_ModuleInit + I2C_IRQSTATUS);      // Clears any pending interrupt flags

    // Me fijo que no hayan interrupciones pendientes
    
    aux_val = ioread32(I2C_ModuleInit + I2C_IRQSTATUS_RAW);

	if((aux_val & 0x1000) != 0) 
    {
        pr_err("[ERROR] MPU6050 WRITE: IRQ not clear \n");
		return -1;
	}

    // Seteo las condiciones iniciales

    I2CData.I2CTxDataLen    = 2;
    I2CData.I2CRxDataLen    = 0;

    I2CData.I2CTxDataIndex  = 0;
    I2CData.I2CRxDataIndex  = 0;
    
    I2CWakeUpCondition_Rx = 0;
    I2CWakeUpCondition_Tx = 0;

    // Seteo el address del device
    
    iowrite32(OWN_ADDRESS, I2C_ModuleInit + I2C_OA);        // Address del controlador de i2c
	iowrite32(SLAVE_ADDRESS, I2C_ModuleInit + I2C_SA);      // MPU6050 i2c address


    // Configuro el i2c como master transmitter
    /*
        Master mode: MST = 1
        Transmitter mode: TRX = 1
    */

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val = aux_val | I2C_CON_TRX | I2C_CON_MST;  
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);
    
    // Activo ARDY (Register access ready interrupt) y XRDY (Transmit data ready interrupt) 

    aux_val = ioread32(I2C_ModuleInit + I2C_IRQENABLE_SET);
    aux_val = aux_val | XRDY_IE | ARDY_IE;
    iowrite32(aux_val, I2C_ModuleInit + I2C_IRQENABLE_SET);

    // Pido memoria para el buffer de transmision

    
    if((I2CData.I2CTxData = (uint8_t *)kmalloc(I2CData.I2CTxDataLen, GFP_KERNEL)) == NULL) 
    {
		pr_err("[ERROR] MPU6050 READ: Requesting allocation memory for Tx buffer \n");
		return -1;
	}

    // Set 2 bytes de Tx

    iowrite32(I2CData.I2CTxDataLen, I2C_ModuleInit + I2C_CNT);     // Data Count to transmit - 2 Bytes
    
    I2CData.I2CTxData[0] = reg2Write;
	I2CData.I2CTxData[1] = data;


    /* 
        Empiezo la transmision

        I2C_CON_START: Start
        I2C_CON_STOP: Stop
    */

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val = aux_val | I2C_CON_START | I2C_CON_STOP;      
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

    // Espero el flag de Tx -- En la Tx no me pueden interrumpir hasta que termine
    wait_event(I2CWakeUp, I2CWakeUpCondition_Tx > 0);

    
    /* Una vez que leo, libero/limpio todo */
    
    I2CWakeUpCondition_Rx   = 0;
    I2CWakeUpCondition_Tx   = 0;
    I2CData.I2CTxDataIndex  = 0;

    // Envio stop para dar por finalizado el intercambio

    aux_val = ioread32(I2C_ModuleInit + I2C_CON);
    aux_val |= I2C_CON_STOP;
    iowrite32(aux_val, I2C_ModuleInit + I2C_CON);

    iowrite32(I2C_ALL_IE, I2C_ModuleInit + I2C_IRQENABLE_CLR);    // Limpio todas las interrupciones
	kfree(I2CData.I2CTxData);
	
    return 0;
   
}

/*---------------------------------------------------------------------- OPEN -------------------------------------------------------------------------------------*/


static int my_open(struct inode *inode, struct file *file)
{
    static uint32_t I2CComands;
    uint32_t aux_val;

    pr_info("[LOG] OPEN: Opening char device\n");


    /*
        Habilitacion pines SDA y SCL
            PIN 19 : UART1_Ctsn - I2C2_SCL
            PIN 20 : UART1_Ctsn - I2C2_SDA
            Ver pag 1515 y 44 AM335x
    */
    
    /*
        Configuro pin 20
    */
    
    pr_info("[LOG] OPEN: Configuration pins...\n");
    aux_val = ioread32(CTRL_ModuleInit + CONF_UART1_CSTN);
    aux_val = PIN_I2C_CFG;
    iowrite32(aux_val, CTRL_ModuleInit + CONF_UART1_CSTN);
    
    /*
        Configuro pin 19
    */
    
    aux_val = ioread32(CTRL_ModuleInit + CONF_UART1_RSTN); 
    aux_val = PIN_I2C_CFG;
    iowrite32(aux_val, CTRL_ModuleInit + CONF_UART1_RSTN);
    
    pr_info("[LOG] OPEN: SDA y SCL configured successfully\n");


    /* 
        I2C2 Clock enable 
    */

    iowrite32(0x02, CLK_ModuleInit + CM_PER_I2C2_CLKCTRL);

    // Espero a que el clock este listo

    do
    {
	    msleep(1);
	} while(ioread32(CLK_ModuleInit + CM_PER_I2C2_CLKCTRL) != 0x02);
    
    pr_info("[LOG] OPEN: Clock ready\n");

    // Espero a que se estabilice
    msleep(10);

    i2c_softreset();

    /*
        Configuring I2C module according to the Technical Reference Manual (Rev. P)
        Ver Pagina 4596
    */

    /*
        First step, turn of the i2c module
        Ver Pagina 4631
    */

    iowrite32(I2C_OFF, I2C_ModuleInit + I2C_CON);

    /*
        1. Setting prescale for obtaining a CLK frequency of 12Mhz, fclk frequency is divided by 2
        Ver Pagina 4598
    */

    iowrite32(PRESCALER_X2, I2C_ModuleInit + I2C_PSC);

    /*
        2. Calculating clock for obtaining a transmision speed of 400 KHz

        Ver Pagina 4638
    */

    I2CComands = 9;
    iowrite32(I2CComands, I2C_ModuleInit + I2C_SCLL);

    I2CComands = 11;
    iowrite32(I2CComands, I2C_ModuleInit + I2C_SCLH);

    /*    
        3. Defining master's address
        Ver Pagina 4643
    */

    iowrite32(OWN_ADDRESS, I2C_ModuleInit + I2C_OA);

    /*
        Configuring the slave adress for communication
        Ver pagina 4635
    */
    
    iowrite32(SLAVE_ADDRESS, I2C_ModuleInit + I2C_SA);

    /*
      4. Take the I2C module out of reset (I2C_CON:I2C_EN = 1).
      4. Getting the i2c module out of the rst condition
      Bit 15 I2C_EN:
                     0h = Controller in reset. FIFO are cleared and status bits are set to their default value.
                     1h = Module enabled
      Bit 10 MST:
                     0h = Slave mode
                     1h = Master mode
      Ver Pagina 4932

      Module is enabled and in master mode
    */

    I2CComands = 0x8000; 
    
    iowrite32(I2CComands, I2C_ModuleInit + I2C_CON);
    

    aux_val = ioread32(I2C_ModuleInit + I2C_IRQSTATUS);
	iowrite32(I2C_ALL_IE, I2C_ModuleInit + I2C_IRQSTATUS);


    /* Starting MPU6050 configuration */

    MPU6050_init();

    pr_info("[LOG] OPEN: Open ok! \n");

    return 0;
}

/*---------------------------------------------------------------------- CLOSE -------------------------------------------------------------------------------------*/


static int my_close(struct inode *inode, struct file *file)
{
    pr_info("[LOG] CLOSE: Cerrando! \n");

    // Disable I2C module
    iowrite32(0x0000, I2C_ModuleInit + I2C_CON);
    iowrite32(0x00, I2C_ModuleInit + I2C_PSC);
    iowrite32(0x00, I2C_ModuleInit + I2C_SCLL);  // Pongo el valor de reset
    iowrite32(0x00, I2C_ModuleInit + I2C_SCLH);  // Pongo el valor de reset
    iowrite32(0x00, I2C_ModuleInit + I2C_OA);    // Pongo el valor de reset
    return 0;
}

/*---------------------------------------------------------------------- READ -------------------------------------------------------------------------------------*/

static ssize_t my_read(struct file *device_descriptor, char *buff, size_t dataLength, loff_t *offset)
{
    uint32_t lastRequested = ACCELX_H;
    uint8_t reg2Read = 0, i;
 
    uint8_t *muestras;
    int copy2user = 0;

    pr_info("[LOG] READ: Starting to read!\n");

    if(dataLength < 0)
    {
        pr_alert("[ERROR] READ: Size to read is invalid.\n");
        return -1;
    }
    

    /* Request memory */

    if((muestras = kmalloc(dataLength * DATAFRAME_LEN, GFP_KERNEL)) == NULL)
    {
        pr_err("[ERROR] Could not assign memory to the samples buffer!\n");
        return -1;
    }


    for(i = 0; i < dataLength * DATAFRAME_LEN; i++) 
    {
        pr_info("[LOG] READ: lastRequested = %d\n", lastRequested);

        switch (lastRequested) 
        {
            case ACCELX_H:
                reg2Read = ACCEL_XOUT_H;		
                break;
            case ACCELX_L:
                reg2Read = ACCEL_XOUT_L;		
                break;
            case ACCELY_H:
                reg2Read = ACCEL_YOUT_H;		
                break;
            case ACCELY_L:
                reg2Read = ACCEL_YOUT_L;		
                break;
            case ACCELZ_H:
                reg2Read = ACCEL_ZOUT_H;		
                break;
            case ACCELZ_L:
                reg2Read = ACCEL_ZOUT_L;		
                break;
            case TEMP_H:
                reg2Read = TEMP_OUT_H;		
                break;
            case TEMP_L:
                reg2Read = TEMP_OUT_L;		
                break;
            case GYROX_H:
                reg2Read = GYRO_XOUT_H;		
                break;
            case GYROX_L:
                reg2Read = GYRO_XOUT_L;		
                break;
            case GYROY_H:
                reg2Read = GYRO_YOUT_H;		
                break;
            case GYROY_L:
                reg2Read = GYRO_YOUT_L;		
                break;
            case GYROZ_H:
                reg2Read = GYRO_ZOUT_H;		
                break;
            case GYROZ_L:
                reg2Read = GYRO_ZOUT_L;		
                break;
        }
        
        muestras[i] = (char) MPU6050_read(reg2Read);

        pr_info("[LOG] READ: Data number %d: 0x%x read from 0x%X", i, muestras[i], reg2Read);
        
        if(lastRequested >= GYROZ_L) 
        {
            lastRequested = ACCELX_H;
        } 
        else 
        {
            lastRequested++;
        }
    }

    
    if((copy2user = copy_to_user(buff, muestras, dataLength * DATAFRAME_LEN * sizeof(uint8_t))) < 0)
    {
        pr_err("[ERROR] READ: Error copy_to_user \n");
        kfree(muestras);
        return -1;
    }

    pr_info("[LOG] READ: Successfully copied to user!\n");

    kfree(muestras);

    pr_info("[LOG] READ: Read ok!\n");

    return i;

}

static void i2c_softreset(void) 
{
	iowrite32(0, I2C_ModuleInit + I2C_CON);         
	iowrite32(I2C_EN, I2C_ModuleInit + I2C_CON);    

	return;
}

/*

static ssize_t my_write(struct file *device_descriptor, const char *buff, size_t dataLength, loff_t *offset)
{
    return 0;
}

static long my_ioctl(struct file *device_descriptor, unsigned int command, unsigned long arguments)
{
    return 0;
}

*/
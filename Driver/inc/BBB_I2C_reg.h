#ifndef bbb_i2c_reg_h
#define bbb_i2c_reg_h

#define I2C2_REG                0x4819C000
#define I2C2_REG_LEN            0x00001000
#define I2C_IRQSTATUS           0x00000028
#define I2C_IRQENABLE_SET       0x0000002C
#define I2C_IRQENABLE_CLR       0x00000030
#define I2C_EN                  0x00008000

#define ARDY_IE                 0x00000004
#define RRDY_IE                 0x00000008
#define XRDY_IE                 0x00000010

#define I2C_DATA                0x0000009C
#define I2C_CON                 0x000000A4
#define I2C_CON_START           0x00000001
#define I2C_CON_STOP            0x00000002
#define I2C_CON_TRX             0x00000200
#define I2C_CON_MST             0x00000400

#define I2C_PSC                 0x000000B0
#define I2C_SCLL                0x000000B4
#define I2C_SCLH                0x000000B8    
#define I2C_OA                  0x000000A8
#define I2C_SA                  0x000000AC
#define I2C_IRQSTATUS_RAW       0x00000024
#define I2C_CNT                 0x00000098

#define CM_PER_REG              0x44E00000
#define CM_PER_REG_LEN          0x00000400
#define CM_PER_I2C2_CLKCTRL     0x00000044

#define CTRL_MODULE_REG         0x44E10000
#define CTRL_MODULE_REG_LEN     0x00002000
#define CONF_UART1_CSTN         0x00000978
#define CONF_UART1_RSTN         0x0000097C
#define PIN_I2C_CFG             0x0000003B

#define I2C_OFF                 0x00000000
#define I2C_ALL_IE              0x0000FFFF
#define PRESCALER_X2            0x01
#define OWN_ADDRESS             0x00000036
#define SLAVE_ADDRESS           0x00000068


#endif

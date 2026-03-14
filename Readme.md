# STM32MP1 PCA9685 Stepper Motor Driver

This project implements a Linux Character Device Driver to control a stepper motor
using the PCA9685 16-channel PWM controller over I2C on the STM32MP1 platform.

Users can control motor direction and speed by writing commands to the device file.

## Hardware
- STM32MP1 Board
- PCA9685 16-Channel PWM Controller
- Stepper Motor
- I2C Interface

+--------------------------------+
|           User Space           |
|--------------------------------|
| echo / write commands          |
| Example:                       |
| echo 2 > /dev/ko_communication |
+---------------|----------------+
                |
                v
+--------------------------------+
|    Character Device Driver     |
|--------------------------------|
| write()                        |
+---------------|----------------+
                |
                v
+--------------------------------+
|         I2C Interface          |
|--------------------------------|
| I2C_Device_Write()             |
+---------------|----------------+
                |
                v
+--------------------------------+
|       PCA9685 PWM Driver       |
|--------------------------------|
| Generates PWM signals          |
| for motor control              |
+---------------|----------------+
                |
                v
+--------------------------------+
|         Stepper Motor          |
+--------------------------------+

## Build

Compile the driver:

make

## Insert Module

sudo insmod i2cdriver.ko

The driver creates the device file:
/dev/ko_communication

## Control Motor

echo 0 > /dev/ko_communication   # slow clockwise
echo 1 > /dev/ko_communication   # normal clockwise
echo 2 > /dev/ko_communication   # fast clockwise
echo 3 > /dev/ko_communication   # slow anticlockwise
echo 4 > /dev/ko_communication   # normal anticlockwise
echo 5 > /dev/ko_communication   # fast anticlockwise
sudo rmmod mydriver

## Author

Akash B




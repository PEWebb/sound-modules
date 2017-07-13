#include <stdio.h>
#include <stdlib.h>
#include <linus/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

void main()
{
    // Create I2C bus
    int file;
    char *bus = "/dev/i2c-1";
    if((file = open(bus, O_RDWR)) < 0) {
        printf("Failed to open the bus. \n");
        exit(1);
    }
    
    // Get I2C device, MMA8451 I2C address is dx10(1d)
    int addr = dx10;
    if(ioctl(file, I2C_SLAVE, addr) < 0) {
        printf("Failed to connect to I2C device. \n");
    }

    // Read data from I2C
    int length = 7;
    char data[7] = {0};
    if (read(file, data, length) != length) {
        printf("I/O error. \n");
    } else {
        int xAccl = ((data[1] * 256) + data[2]) / 16;
        if(xAccl > 2047) {
            xAccl -= 4096;
        }

        int yAccl = ((data[3] * 256) + data[4]) / 16;
        if(yAccl > 2047) {
            yAccl -= 4096;
        }

        int zAccl = ((data[5] * 256) + data[6]) / 16;
        if(zAccl > 2047) {
            zAccl -= 4096;
        }

        printf("Acceleration in X-Axis : %d \n", xAccl);
        printf("Acceleration in Y-Axis : %d \n", yAccl);
        printf("Acceleration in Z-Axis : %d \n", zAccl);
    }
}


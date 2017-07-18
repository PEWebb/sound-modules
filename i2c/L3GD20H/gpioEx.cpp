#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    // Create I2C bus
    int file;
    char *bus = "/dev/i2c-1";
    if((file = open(bus, O_RDWR)) < 0) {
        printf("Failed to open the bus. \n");
        exit(1);
    }
    
    // Get I2C device, MMA8452 I2C address is dx10(1d)
    int addr = 0x6b;
    if(ioctl(file, I2C_SLAVE, addr) < 0) {
        printf("Failed to connect to I2C device. \n");
    }

    // Select mode register(0x2A)
	// Standby mode(0x00)
	char config[2] = {0};
	config[0] = 0x20;
	config[1] = 0x0F;
	write(file, config, 2);

	// Select mode register(0x2A)
	// Active mode(0x01)
	config[0] = 0x21;
	config[1] = 0x00;
	write(file, config, 2);
	
	// Select configuration register(0x0E)
	// Set range to +/- 2g(0x00)
	config[0] = 0x39;
	config[1] = 0x00;
	write(file, config, 2);
	sleep(0.5);

    for(;;){
        // Read data from I2C
        char regXlsb[1] = {0x28};
        char regXmsb[1] = {0x29};
        char regYlsb[1] = {0x2A};
        char regYmsb[1] = {0x2B};
        char regZlsb[1] = {0x2C};
        char regZmsb[1] = {0x2D};
        
        int length = 1;

        char dataXlsb[1] = {0};
        char dataXmsb[1] = {0};
        char dataYlsb[1] = {0};
        char dataYmsb[1] = {0};
        char dataZlsb[1] = {0};
        char dataZmsb[1] = {0};
        
        write(file, regXlsb, 1);
        read(file, dataXlsb, length);

        write(file, regXmsb, 1);
        read(file, dataXmsb, length);

        write(file, regYlsb, 1);
        read(file, dataYlsb, length);

        write(file, regYmsb, 1);
        read(file, dataYmsb, length);

        write(file, regZlsb, 1);
        read(file, dataZlsb, length);

        write(file, regZmsb, 1);
        read(file, dataZmsb, length);

        int xAngle = 256*dataXmsb[0]+dataXlsb[0];
        if (xAngle >= 32768) {
            xAngle -= 66536;
        }

        int yAngle = 256*dataYmsb[0]+dataYlsb[0];
        if (yAngle >= 32768) {
            yAngle -= 66536;
        }

        int zAngle = 256*dataZmsb[0]+dataZlsb[0];
        if (zAngle >= 32768) {
            zAngle -= 66536;
        }

        printf("X: %d, Y: %d, Z: %d \n", xAngle, yAngle, zAngle);
        // printf("Data: %d, %d, %d, %d, %d, %d \n", dataXlsb[0], dataXmsb[0],
        //                dataYlsb[0], dataYmsb[0], dataZlsb[0], dataZmsb[0]);

    }
    return 0;
}


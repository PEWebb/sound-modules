#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>

std::mutex i2cData;
int distortionFactor;

void i2cConnection()
{
    // Create I2C bus
    int file;
    char *bus = "/dev/i2c-1";
    if((file = open(bus, O_RDWR)) < 0) {
        printf("Failed to open the bus. \n");
        exit(1);
    }
    
    // Get I2C device, MMA8452 I2C address is dx10(1d)
    int addr = 0x1d;
    if(ioctl(file, I2C_SLAVE, addr) < 0) {
        printf("Failed to connect to I2C device. \n");
    }

    // Select mode register(0x2A)
	// Standby mode(0x00)
	char config[2] = {0};
	config[0] = 0x2A;
	config[1] = 0x00;
	write(file, config, 2);

	// Select mode register(0x2A)
	// Active mode(0x01)
	config[0] = 0x2A;
	config[1] = 0x01;
	write(file, config, 2);
	
	// Select configuration register(0x0E)
	// Set range to +/- 2g(0x00)
	config[0] = 0x0E;
	config[1] = 0x00;
	write(file, config, 2);
	sleep(0.5);

    for(;;){
        // Read data from I2C
        char reg[1] = {0x00};
        write(file, reg, 1);
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

            // printf("Acceleration in X/Y/Z-Axis : %d %d %d \n", xAccl, yAccl, zAccl);
            i2cData.lock();
            distortionFactor = xAccl + yAccl + (zAccl-1000);
            i2cData.unlock();
        }
    }
}

void audioProcessing() {
    for(;;) {
        i2cData.lock();
        printf("Distortion Factor: %d \n", distortionFactor);
        i2cData.unlock();
    }
};

int main()
{
    std::thread t1(i2cConnection);
    std::thread t2(audioProcessing);
    // std::cout << "main thread" << std::endl;
    t1.join();
    t2.join();
    return 0;
}

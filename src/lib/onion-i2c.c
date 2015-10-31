#include <onion-i2c.h>

// get a file handle to the device 
int _i2c_getFd(int adapterNum, int *devHandle)
{
	int 	status;
	char 	pathname[255];

	// define the path to open
	status = snprintf(pathname, sizeof(pathname), I2C_DEV_PATH, adapterNum);

	// check the filename
	if (status < 0 || status >= sizeof(pathname)) {
		// add errno
        return EXIT_FAILURE;
    }

	// create a file descriptor for the I2C bus
#ifdef I2C_ENABLED
  	*devHandle = open(pathname, O_RDWR);
#else
  	*devHandle = 0;
#endif

  	// check the defvice handle
  	if (*devHandle < 0) {
  		// add errno
  		return EXIT_FAILURE;
  	}

  	return EXIT_SUCCESS;
}

// release the device file handle
int _i2c_releaseFd(int devHandle)
{
#ifdef I2C_ENABLED
	if ( close(devHandle) < 0 ) {
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

// set the device address
int _i2c_setDevice(int devHandle, int addr)
{
#ifdef I2C_ENABLED
	// set to 7-bit addr
	if ( ioctl(devHandle, I2C_TENBIT, 0) < 0 ) {
		return EXIT_FAILURE;
	}

	// set the address
	if ( ioctl(devHandle, I2C_SLAVE, addr) < 0 ) {
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

// set the 10bit device address
int _i2c_setDevice10bit(int devHandle, int addr)
{
#ifdef I2C_ENABLED
	// set to 10-bit addr
	if ( ioctl(devHandle, I2C_TENBIT, 1) < 0 ) {
		return EXIT_FAILURE;
	}

	// set the address
	if ( _i2c_setDevice(devHandle, addr) != EXIT_SUCCESS ) {
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}

// generic function to write a buffer to the i2c bus
int _i2c_writebuffer(int devNum, int devAddr, int addr, char buffer[], int size)
{
	int 	status;
	int 	fd, index;

	// open the file handle
	status 	= _i2c_getFd(devNum, &fd);

	// set the device address
	if ( status == EXIT_SUCCESS ) {
		status 	= _i2c_setDevice(fd, devAddr);
	}

	// perform the write
	if ( status == EXIT_SUCCESS ) {
		// dbg
		for (index = 0; index < size; index++) {
			printf("i2c:: buffer[%d] = 0x%02x \n", index, buffer[index]);
		}
		printf("\tsize is %d\n", size);

#ifdef I2C_ENABLED
		// write to the i2c device
		status = write(fd, buffer, size);
		if (status != size) {
			printf("i2c:: write issue for register 0x%02x, errno is %d: %s\n", addr, errno, strerror(errno) );
			status 	= EXIT_FAILURE;
		}
#endif
 	}

 	// release the device file handle
 	status 	|= _i2c_releaseFd(fd);

	return (status);
}

// write n bytes to the i2c bus
int i2c_write(int devNum, int devAddr, int addr, int val)
{
	int 	status;
	int 	size, tmp, index;
	char 	buffer[32];

	//// buffer setup
	// clear the buffer
	memset( buffer, 0, sizeof(buffer) );
	// push the address and data values into the buffer
	buffer[0]	= (addr & 0xff);
	buffer[1]	= (val & 0xff);
	size 		= 2;

	// if value is more than 1-byte, add to the buffer
	tmp 	= (val >> 8);	// start with byte 1
	index	= 2;
	while (tmp > 0x00) {
		buffer[index] = (tmp & 0xff);

		tmp	= tmp >> 8; // advance the tmp data by a byte
		index++; 		// increment the index

		size++;			// increase the size
	}

	I2C_PRINT("i2c:: Writing to device 0x%02x: addr = 0x%02x, data = 0x%02x (data size: %d)\n", devAddr, addr, val, (size-1) );

	// write the buffer
 	status 	= _i2c_writebuffer(devNum, devAddr, addr, buffer, size);

	return (status);
}

// write a specified number of bytes to the i2c bus
int i2c_writeBytes(int devNum, int devAddr, int addr, int val, int numBytes)
{
	int 	status;
	int 	size, index;
	char 	buffer[32];

	//// buffer setup
	// clear the buffer
	memset( buffer, 0, sizeof(buffer) );
	// push the address and data values into the buffer
	buffer[0]	= (addr & 0xff);
	size 		= 1;

	// add all data bytes to buffer
	index	= 1;
	for (index = 0; index < numBytes; index++) {
		buffer[index+1] = ( (val >> (8*index)) & 0xff );

		size++;			// increase the size
	}

	I2C_PRINT("i2c:: Writing to device 0x%02x: addr = 0x%02x, data = 0x%02x (data size: %d)\n", devAddr, addr, val, (size-1) );

	// write the buffer
	status 	= _i2c_writebuffer(devNum, devAddr, addr, buffer, size);

	return (status);
}

// read a byte from the i2c bus
int i2c_read(int devNum, int devAddr, int addr, int *val, int numBytes)
{
	int 	status, size, index;
	int 	fd;
	char 	buffer[32];

	I2C_PRINT("i2c:: Reading %d bytes from device 0x%02x: addr = 0x%02x\n", numBytes, devAddr, addr);

	// open the device file handle
	status 	= _i2c_getFd(devNum, &fd);

	// set the device address
	if ( status == EXIT_SUCCESS ) {
		status 	= _i2c_setDevice(fd, devAddr);
	}

	// perform the read
	if ( status == EXIT_SUCCESS ) {
		//// set addr
		// clear the buffer
		memset( buffer, 0, sizeof(buffer) );
		// push the address and data values into the buffer
		buffer[0]	= (addr & 0xff);
		size 		= 1;

#ifdef I2C_ENABLED
		// write to the i2c device
		status = write(fd, buffer, size);
		if (status != size) {
			printf("i2c:: write issue for register 0x%02x, errno is %d: %s\n", addr, errno, strerror(errno) );
		}
#endif

		//// read data
		// clear the buffer
		memset( buffer, 0, sizeof(buffer) );

#ifdef I2C_ENABLED
		// read from the i2c device
		size 	= numBytes;
		status 	= read(fd, buffer, size);
		if (status != size) {
			printf("i2c:: read issue for register 0x%02x, errno is %d: %s\n", addr, errno, strerror(errno) );
			status 	= EXIT_FAILURE;
		}
#else
		buffer[0] 	= 0x0;
#endif		

		//// return the data
		I2C_PRINT("\tread %d bytes, value: 0x", size);
		for (index = (size-1); index >= 0; index--) {
			I2C_PRINT("%02x", buffer[index]);
			*val 	&= (buffer[index] << (8*index) );
		}
		I2C_PRINT("\n");
 	}

 	// release the device file handle
 	status 	|= _i2c_releaseFd(fd);

	return (status);
}

// read a single byte from the i2c bus
int i2c_readByte(int devNum, int devAddr, int addr, int *val)
{
	int 	status;

	status	= i2c_read	(	devNum, 
							devAddr, 
							addr, 
							val,
							1
						);

	return (status);
}

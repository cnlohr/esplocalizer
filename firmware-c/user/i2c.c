#include <ctype.h>
#include <esp82xxutil.h>
#include <osapi.h>

#define I2CSPEEDBASE 1
#define I2CNEEDGETBYTE
#define DSDA 5
#define DSCL 4

#include "static_i2c.h"
#include "bmp280.h"
#include "lsm9ds1.h"

struct calData
{
	uint16_t T1;
	int16_t  T2;
	int16_t  T3;
	uint16_t P1;
	int16_t  P2;
	int16_t  P3;
	int16_t  P4;
	int16_t  P5;
	int16_t  P6;
	int16_t  P7;
	int16_t  P8;
	int16_t  P9;
};

static struct calData CD;

int16_t mag[3];
int16_t linrottemp[7];


void ICACHE_FLASH_ATTR InitI2Cs()
{
	int i;
	int r;
	ConfigI2C();
/*	while(1)
	{
		printf( "A\n" );
		os_delay_us(600000);
		PIN_DIR_OUTPUT = _BV(DSDA) | _BV(DSCL);
		printf( "B\n" );
		os_delay_us(600000);
		PIN_DIR_INPUT = _BV(DSDA) | _BV(DSCL);
	}*/
	r = InitBMP280();
	printf( "BMP280: %d\n", r );
	r = InitLSM9DS1();
	printf( "LSM9DS1: %d\n", r );
	r = GetBMPCalVals( (uint16_t*)&CD.T1 );
	printf( "BMP Cal Data: %d\n", r );
	for( i = 0; i < 12; i++ )
	{
		printf( " %d", ((int16_t*)&CD.T1)[i] );
	}
	printf( "\n" );
}

#define BMP280_S32_t int32_t
#define BMP280_S64_t long long
#define BMP280_U32_t uint32_t
#define BMP280_U64_t unsigned long long

BMP280_S32_t t_fine;

BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
	BMP280_S32_t var1, var2, T;
	var1 = ((((adc_T>>3) - ((BMP280_S32_t)CD.T1<<1))) * ((BMP280_S32_t)CD.T2)) >> 11;
	var2 = (((((adc_T>>4) - ((BMP280_S32_t)CD.T1)) * ((adc_T>>4) - ((BMP280_S32_t)CD.T1))) >> 12) *
		((BMP280_S32_t)CD.T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P)
{
	BMP280_S64_t var1, var2, p;
	var1 = ((BMP280_S64_t)t_fine) - 128000;
	var2 = var1 * var1 * (BMP280_S64_t)CD.P6;
	var2 = var2 + ((var1*(BMP280_S64_t)CD.P5)<<17);
	var2 = var2 + (((BMP280_S64_t)CD.P4)<<35);
	var1 = ((var1 * var1 * (BMP280_S64_t)CD.P3)>>8) + ((var1 * (BMP280_S64_t)CD.P2)<<12);
	var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)CD.P1)>>33;
	if (var1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576-adc_P;
	p = (((p<<31)-var2)*3125)/var1;
	var1 = (((BMP280_S64_t)CD.P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((BMP280_S64_t)CD.P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)CD.P7)<<4);

	return (BMP280_U32_t)p;
}

void ReadI2Cs()
{
	uint8_t bmptelem[6];

	ReadM( mag );
	ReadAG( linrottemp );
	GetBMPTelem( bmptelem );

	int bmpP = ( (bmptelem[0]<<16)|(bmptelem[1]<<8)|bmptelem[2])>>4;
	int bmpT = ( (bmptelem[3]<<16)|(bmptelem[4]<<8)|bmptelem[5])>>4;

	int bmpTo = bmp280_compensate_T_int32( bmpT );
	int bmpPo = bmp280_compensate_P_int64( bmpP );

	printf( " %5d %5d %5d / %5d %5d %5d / %5d %5d %5d / %10d %10d\n", mag[0], mag[1], mag[2], linrottemp[0], linrottemp[1], linrottemp[2], linrottemp[3], linrottemp[4], linrottemp[5], bmpPo, bmpTo );

}



int ICACHE_FLASH_ATTR ReadFastAcc( short * data )
{
	int r, status;

	SendStart();
	r = SendByte( AG_ADDY );
	SendByte( 0x2f );
	SendStop();

	if( r ) { printf( "SRR: %d\n",r )  return -1; }

	SendStart();
	SendByte( AG_ADDY | 1 );
	status = GetByte( 1 ) & 0x3f;
	SendStop();

	if( status == 0 ) return 0;
	printf( "." );


	//Got data!
	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); printf( "AGF\n" ); return -1; }
	SendByte( 0x18 ); //Gyro
	SendStop();

	SendStart();
	SendByte( AG_ADDY | 1 );
	data[0] = LSM9LR16(0);
	data[1] = LSM9LR16(0);
	data[2] = LSM9LR16(1);
	SendStop();

	//Got data!
	SendStart();
	r = SendByte( AG_ADDY );
	if( r ) { SendStop(); printf( "AGF\n" ); return -1; }
	SendByte( 0x28 );
	SendStop();


	SendStart();
	SendByte( AG_ADDY | 1 );
	data[3] = LSM9LR16(0);
	data[4] = LSM9LR16(0);
	data[5] = LSM9LR16(1);
	SendStop();




	return 1;
}

void ICACHE_FLASH_ATTR SetupForFastAcc( int yes )
{
	int r;
	if( yes )
	{
		SendStart();
		r = SendByte( AG_ADDY );
		if( r ) { SendStop(); SendString(PSTR("AG Fault")); return; }
		SendByte( 0x1E );
		SendByte( 0b00111000 ); //0x1E: Disable Gyro
		SendByte( 0b00111000 ); //0x1F: Turn on Accelerometer
		SendByte( 0b10111100 ); //0x20: Accelerometer, 476 Hz, full-scale=+/-4G, Bandwidth determined by Acc speed.
		SendByte( 0b00000000 ); //0x21: Dubious: Disable high-resolution mode.
		SendByte( 0b01000100 ); //0x22: LSB in lower, Autoincrement, push-pull, etc.  TODO: Block update?

		//              v DRDY mask bit?
		SendByte( 0b00011010 ); //0x23: DA Timer Enabled, FIFO Enabled Don't stop on FIFO.
		SendStop();


		SendStart();
		r = SendByte( AG_ADDY );
		if( r ) { SendStop(); SendString(PSTR("AG FaultC")); return; }
		SendByte( 0x10 );
		SendByte( 0b10111011 ); //Gyro ODR=467Hz, Cutoff=100Hz, 2000DPS
		SendByte( 0b00000000 ); //XXX Consider making outsel 10, to enable LPF2
		SendByte( 0b00000000 ); //Highpass = off.
		SendStop();


	}
	else
	{
		int r;
		SendStart();
		r = SendByte( AG_ADDY );
		if( r ) { SendStop(); SendString(PSTR("AG Fault")); return; }
		SendByte( 0x1E );
		SendByte( 0b00111000 ); //0x1E: Enable Gyro
		SendByte( 0b00111000 ); //0x1F: Turn on Accelerometer
		SendByte( 0b01011000 ); //0x20: Accelerometer, 50Hz, +/- 8g, (less than 50Hz) bandwidth
		SendByte( 0b00000000 ); //0x21: Dubious: Disable high-resolution mode.
		SendByte( 0b01000100 ); //0x22: LSB in lower, Autoincrement, push-pull, etc.  TODO: Block update?
		SendByte( 0b00011010 ); //0x23: Temp in FIFO. DA Timer Enabled, FIFO Enabled Don't stop on FIFO.
		SendStop();


		SendStart();
		r = SendByte( AG_ADDY );
		if( r ) { SendStop(); SendString(PSTR("AG FaultC")); return; }
		SendByte( 0x10 );
		SendByte( 0b01011011 ); //Gyro ODR=59.5Hz, Cutoff=19Hz, 2000DPS
		SendByte( 0b00000000 ); //XXX Consider making outsel 10, to enable LPF2
		SendByte( 0b00000000 ); //Highpass = off.
		SendStop();

	}
}


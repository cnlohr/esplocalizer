#ifndef _I2C_H
#define _I2C_H

void InitI2Cs();
void ReadI2Cs();

void SetupForFastAcc( int yes );
int ReadFastAcc( short * data ); //Returns 0 if no samples read, 1 if samples read, -1 on error.

#endif


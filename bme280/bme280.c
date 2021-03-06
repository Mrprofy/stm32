#include "i2c.h"
#include "bme280.h"




// Carries fine temperature as global value for pressure and humidity calculation
static int32_t t_fine;




// Write new value to BME280 register
// input:
//   reg - register number
//   value - new register value
void BME280_WriteReg(uint8_t reg, uint8_t value) {
	uint8_t buf[2] = { reg, value };

	I2Cx_Write(BME280_I2C_PORT,buf,2,BME280_ADDR,I2C_STOP);
}

// Read BME280 register
// input:
//   reg - register number
// return:
//   register value
uint8_t BME280_ReadReg(uint8_t reg) {
	uint8_t value = 0; // Initialize value in case of I2C timeout

	// Send register address
	I2Cx_Write(BME280_I2C_PORT,&reg,1,BME280_ADDR,I2C_NOSTOP);
	// Read register value
	I2Cx_Read(BME280_I2C_PORT,&value,1,BME280_ADDR);

	return value;
}

// Check if BME280 present on I2C bus
// return:
//   BME280_SUCCESS if BME280 present, BME280_ERROR otherwise (not present or it was an I2C timeout)
BME280_RESULT BME280_Check(void) {
	return (BME280_ReadReg(BME280_REG_ID) == 0x60) ? BME280_SUCCESS : BME280_ERROR;
}

// Order BME280 to do a software reset
// note: after reset the chip will be unaccessible during 3ms
void BME280_Reset(void) {
	BME280_WriteReg(BME280_REG_RESET,BME280_SOFT_RESET_KEY);
}

// Get version of the BME280 chip
// return:
//   BME280 chip version or zero if no BME280 present on the I2C bus or it was an I2C timeout
uint8_t BME280_GetVersion(void) {
	return BME280_ReadReg(BME280_REG_ID);
}

// Get current status of the BME280 chip
// return:
//   Status of the BME280 chip or zero if no BME280 present on the I2C bus or it was an I2C timeout
uint8_t BME280_GetStatus(void) {
	return BME280_ReadReg(BME280_REG_STATUS) & BME280_STATUS_MSK;
}

// Get current sensor mode of the BME280 chip
// return:
//   Sensor mode of the BME280 chip or zero if no BME280 present on the I2C bus or it was an I2C timeout
uint8_t BME280_GetMode(void) {
	return BME280_ReadReg(BME280_REG_CTRL_MEAS) & BME280_MODE_MSK;
}

// Set sensor mode of the BME280 chip
// input:
//   mode - new mode (BME280_MODE_SLEEP, BME280_MODE_FORCED or BME280_MODE_NORMAL)
void BME280_SetMode(uint8_t mode) {
	uint8_t reg;

	// Read the 'ctrl_meas' (0xF4) register and clear 'mode' bits
	reg = BME280_ReadReg(BME280_REG_CTRL_MEAS) & ~BME280_MODE_MSK;

	// Configure new mode
	reg |= mode & BME280_MODE_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CTRL_MEAS,reg);
}

// Set coefficient of the IIR filter
// input:
//   filter - new coefficient value (one of BME280_FILTER_x values)
void BME280_SetFilter(uint8_t filter) {
	uint8_t reg;

	// Read the 'config' (0xF5) register and clear 'filter' bits
	reg = BME280_ReadReg(BME280_REG_CONFIG) & ~BME280_FILTER_MSK;

	// Configure new filter value
	reg |= filter & BME280_FILTER_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CONFIG,reg);
}

// Set inactive duration in normal mode (Tstandby)
// input:
//   tsb - new inactive duration (one of BME280_STBY_x values)
void BME280_SetStandby(uint8_t tsb) {
	uint8_t reg;

	// Read the 'config' (0xF5) register and clear 'filter' bits
	reg = BME280_ReadReg(BME280_REG_CONFIG) & ~BME280_STBY_MSK;

	// Configure new standby value
	reg |= tsb & BME280_STBY_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CONFIG,reg);
}

// Set oversampling of temperature data
// input:
//   osrs - new oversampling value (one of BME280_OSRS_T_Xx values)
void BME280_SetOSRST(uint8_t osrs) {
	uint8_t reg;

	// Read the 'ctrl_meas' (0xF4) register and clear 'osrs_t' bits
	reg = BME280_ReadReg(BME280_REG_CTRL_MEAS) & ~BME280_OSRS_T_MSK;

	// Configure new oversampling value
	reg |= osrs & BME280_OSRS_T_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CTRL_MEAS,reg);
}

// Set oversampling of pressure data
// input:
//   osrs - new oversampling value (one of BME280_OSRS_P_Xx values)
void BME280_SetOSRSP(uint8_t osrs) {
	uint8_t reg;

	// Read the 'ctrl_meas' (0xF4) register and clear 'osrs_p' bits
	reg = BME280_ReadReg(BME280_REG_CTRL_MEAS) & ~BME280_OSRS_P_MSK;

	// Configure new oversampling value
	reg |= osrs & BME280_OSRS_P_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CTRL_MEAS,reg);
}

// Set oversampling of humidity data
// input:
//   osrs - new oversampling value (one of BME280_OSRS_H_Xx values)
void BME280_SetOSRSH(uint8_t osrs) {
	uint8_t reg;

	// Read the 'ctrl_hum' (0xF2) register and clear 'osrs_h' bits
	reg = BME280_ReadReg(BME280_REG_CTRL_HUM) & ~BME280_OSRS_H_MSK;

	// Configure new oversampling value
	reg |= osrs & BME280_OSRS_H_MSK;

	// Write value back to the register
	BME280_WriteReg(BME280_REG_CTRL_HUM,reg);

	// Changes to 'ctrl_hum' register only become effective after a write to 'ctrl_meas' register
	// Thus read a value of the 'ctrl_meas' register and write it back after write to the 'ctrl_hum'

	// Read the 'ctrl_meas' (0xF4) register
	reg = BME280_ReadReg(BME280_REG_CTRL_MEAS);

	// Write back value of 'ctrl_meas' register to activate changes in 'ctrl_hum' register
	BME280_WriteReg(BME280_REG_CTRL_MEAS,reg);
}

// Read calibration data
BME280_RESULT BME280_Read_Calibration(void) {
	uint8_t buf[7];

	// Read pressure and temperature calibration data (calib00..calib23)
	buf[0] = BME280_REG_CALIB00; // calib00 register address
	if (I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP) != I2C_SUCCESS) return BME280_ERROR;
	if (I2Cx_Read(BME280_I2C_PORT,(uint8_t *)&cal_param,24,BME280_ADDR) != I2C_SUCCESS) return BME280_ERROR;

	// Skip one byte (calib24) and read H1 (calib25)
	cal_param.dig_H1 = BME280_ReadReg(BME280_REG_CALIB25);

	// Read humidity calibration data (calib26..calib41)
	buf[0] = BME280_REG_CALIB26; // calib26 register address
	if (I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP) != I2C_SUCCESS) return BME280_ERROR;
	if (I2Cx_Read(BME280_I2C_PORT,buf,7,BME280_ADDR) != I2C_SUCCESS) return BME280_ERROR;

	// Unpack data
	cal_param.dig_H2 = (int16_t)((((int8_t)buf[1]) << 8) | buf[0]);
	cal_param.dig_H3 = buf[2];
	cal_param.dig_H4 = (int16_t)((((int8_t)buf[3]) << 4) | (buf[4] & 0x0f));
	cal_param.dig_H5 = (int16_t)((((int8_t)buf[5]) << 4) | (buf[4]  >>  4));
	cal_param.dig_H6 = (int8_t)buf[6];

	return BME280_SUCCESS;
}

// Read the raw pressure value
// input:
//   UP = pointer to store value
// return:
//   BME280_ERROR in case of I2C timeout, BME280_SUCCESS otherwise
// note: '0x80000' result means no data for pressure (measurement skipped or not ready yet)
BME280_RESULT BME280_Read_UP(int32_t *UP) {
	uint8_t buf[3];

	// Clear result value
	*UP = 0;

	// Send 'press_msb' register address
	buf[0] = BME280_REG_PRESS_MSB;
	if (!I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP)) return BME280_ERROR;

	// Read the 'press' register (_msb, _lsb, _xlsb)
	if (I2Cx_Read(BME280_I2C_PORT,&buf[0],3,BME280_ADDR)) {
		*UP = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));

		return BME280_SUCCESS;
	}

	return BME280_ERROR;
}

// Read the raw temperature value
// input:
//   UT = pointer to store value
// return:
//   BME280_ERROR in case of I2C timeout, BME280_SUCCESS otherwise
// note: '0x80000' result means no data for temperature (measurement skipped or not ready yet)
BME280_RESULT BME280_Read_UT(int32_t *UT) {
	uint8_t buf[3];

	// Clear result value
	*UT = 0;

	// Send 'temp_msb' register address
	buf[0] = BME280_REG_TEMP_MSB;
	if (!I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP)) return BME280_ERROR;

	// Read the 'temp' register (_msb, _lsb, _xlsb)
	if (I2Cx_Read(BME280_I2C_PORT,&buf[0],3,BME280_ADDR)) {
		*UT = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));

		return BME280_SUCCESS;
	}

	return BME280_ERROR;
}


// Read the raw humidity value
// input:
//   UH = pointer to store value
// return:
//   BME280_ERROR in case of I2C timeout, BME280_SUCCESS otherwise
// note: '0x8000' result means no data for humidity (measurement skipped or not ready yet)
BME280_RESULT BME280_Read_UH(int32_t *UH) {
	uint8_t buf[2];

	// Clear result value
	*UH = 0;

	// Send 'hum_msb' register address
	buf[0] = BME280_REG_HUM_MSB;
	if (!I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP)) return BME280_ERROR;

	// Read the 'hum' register (_msb, _lsb)
	if (I2Cx_Read(BME280_I2C_PORT,&buf[0],2,BME280_ADDR)) {
		*UH = (int32_t)((buf[0] << 8) | buf[1]);

		return BME280_SUCCESS;
	}

	return BME280_ERROR;
}

// Read all raw values
// input:
//   UT = pointer to store temperature value
//   UP = pointer to store pressure value
//   UH = pointer to store humidity value
// return:
//   BME280_ERROR in case of I2C timeout, BME280_SUCCESS otherwise
// note: 0x80000 value for UT and UP and 0x8000 for UH means no data
BME280_RESULT BME280_Read_UTPH(int32_t *UT, int32_t *UP, int32_t *UH) {
	uint8_t buf[8];

	// Clear result values
	*UT = 0;
	*UP = 0;
	*UH = 0;

	// Send 'press_msb' register address
	buf[0] = BME280_REG_PRESS_MSB;
	if (!I2Cx_Write(BME280_I2C_PORT,&buf[0],1,BME280_ADDR,I2C_NOSTOP)) return BME280_ERROR;

	// Read the 'press', 'temp' and 'hum' registers
	if (I2Cx_Read(BME280_I2C_PORT,&buf[0],8,BME280_ADDR)) {
		*UP = (int32_t)((buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4));
		*UT = (int32_t)((buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4));
		*UH = (int32_t)((buf[6] <<  8) |  buf[7]);

		return BME280_SUCCESS;
	}

	return BME280_ERROR;
}

// Calculate temperature from raw value, resolution is 0.01 degree
// input:
//   UT - raw temperature value
// return: temperature in Celsius degrees (value of '5123' equals '51.23C')
// note: code from the BME280 datasheet (rev 1.1)
int32_t BME280_CalcT(int32_t UT) {
	t_fine  = ((((UT >> 3) - ((int32_t)cal_param.dig_T1 << 1))) * ((int32_t)cal_param.dig_T2)) >> 11;
	t_fine += (((((UT >> 4) - ((int32_t)cal_param.dig_T1)) * ((UT >> 4) - ((int32_t)cal_param.dig_T1))) >> 12) * ((int32_t)cal_param.dig_T3)) >> 14;

	return ((t_fine * 5) + 128) >> 8;
}

// Calculate pressure from raw value
// input:
//   UP - raw pressure value
// return: pressure in Pa as unsigned 32-bit integer in Q24.8 format (24 integer and 8 fractional bits)
// note: output value of '24674867' represents 24674867/256 = 96386.2 Pa = 963.862 hPa
// note: BME280_CalcT must be called before calling this function
// note: using 64-bit calculations
// note: code from the BME280 datasheet (rev 1.1)
uint32_t BME280_CalcP(int32_t UP) {
	int64_t v1,v2,p;

	v1 = (int64_t)t_fine - 128000;
	v2 = v1 * v1 * (int64_t)cal_param.dig_P6;
	v2 = v2 + ((v1 * (int64_t)cal_param.dig_P5) << 17);
	v2 = v2 + ((int64_t)cal_param.dig_P4 << 35);
	v1 = ((v1 * v1 * (int64_t)cal_param.dig_P3) >> 8) + ((v1 * (int64_t)cal_param.dig_P2) << 12);
	v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)cal_param.dig_P1) >> 33;
	if (v1 == 0) return 0; // avoid exception caused by division by zero
	p = 1048576 - UP;
	p = (((p << 31) - v2) * 3125) / v1;
	v1 = (((int64_t)cal_param.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	v2 = (((int64_t)cal_param.dig_P8) * p) >> 19;
	p = ((p + v1 + v2) >> 8) + ((int64_t)cal_param.dig_P7 << 4);

	return (uint32_t)p;
}

// Calculate humidity from raw value
// input:
//   UH - raw humidity value
// return: humidity in %RH as unsigned 32-bit integer in Q22.10 format (22 integer and 10 fractional bits)
// note: output value of '47445' represents 47445/1024 = 46.333 %RH
// note: BME280_CalcT must be called before calling this function
// note: code from the BME280 datasheet (rev 1.1)
uint32_t BME280_CalcH(int32_t UH) {
	int32_t vx1;

	vx1  = t_fine - (int32_t)76800;
	vx1  = ((((UH << 14) - ((int32_t)cal_param.dig_H4 << 20) - ((int32_t)cal_param.dig_H5 * vx1)) +	(int32_t)16384) >> 15) *
			(((((((vx1 * (int32_t)cal_param.dig_H6) >> 10) * (((vx1 * (int32_t)cal_param.dig_H3) >> 11) +
			(int32_t)32768)) >> 10) + (int32_t)2097152) * ((int32_t)cal_param.dig_H2) + 8192) >> 14);
	vx1 -= ((((vx1 >> 15) * (vx1 >> 15)) >> 7) * (int32_t)cal_param.dig_H1) >> 4;
	vx1  = (vx1 < 0) ? 0 : vx1;
	vx1  = (vx1 > 419430400) ? 419430400 : vx1;

	return (uint32_t)(vx1 >> 12);
}

// Fixed point Pa to mmHg conversion (Pascals to millimeters of mercury)
// 1 Pa = 0.00750061683 mmHg
// input:
//    hPa - pressure in pascals (Q24.8 format, result of BME280_CalcP function)
// return:
//    pressure in millimeter of mercury
// note: return value of '746225' represents 746.225 mmHg
uint32_t BME280_Pa_to_mmHg(uint32_t PQ24_8) {
	uint64_t p_mmHg;

	// Multiply Q24.8 pressure value by Q0.20 constant (~0.00750061683)
	// The multiply product will be Q24.28 pressure value (mmHg)
	p_mmHg = (uint64_t)PQ24_8 * BME_MMHG_Q0_20;

	// (p_mmHg >> 28) -> get integer part from Q24.28 value
	// ((uint32_t)p_mmHg << 4) >> 19 -> get fractional part and trim it to 13 bits
	// (XXX * 122070) / 1000000 is rough integer equivalent of float (XXX / 8192.0) * 1000
	return ((uint32_t)(p_mmHg >> 28) * 1000) + (((((uint32_t)p_mmHg << 4) >> 19) * 122070) / 1000000);
}

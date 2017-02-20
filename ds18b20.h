#include <delay.h>

//-----------------------------------------

#define DS18B20_FAMILY_CODE 0x28
#define DS18B20_SEARCH_ROM_CMD 0xf0
#define DS18B20_ALARM_SEARCH_CMD 0xec
//-------------------------------------------------
#define DS18B20_9BIT_RES 0  // 9 bit thermometer resolution
#define DS18B20_10BIT_RES 1 // 10 bit thermometer resolution
#define DS18B20_11BIT_RES 2 // 11 bit thermometer resolution
#define DS18B20_12BIT_RES 3 // 12 bit thermometer resolution
//-------------------------------------------------


//-------------------------------------------------
struct ds18b20_scratch_pad_struct
       {
       unsigned char temp_lsb,
       			temp_msb,
                temp_high_alarm,
                temp_low_alarm,
                conf_register,
                r1,
                r2,
                r3,
                crc;
       } ds18b20_scratch;
//-------------------------------------------------

typedef  struct 
       {
       	unsigned char valid;
       	unsigned char minusChar;
       	unsigned char halfDegree;
       	unsigned char temperatureIntValue;
       } ds18b20_temperature_data_struct;

//-------------------------------------------------
// temp. conversion time [ms] depending on the resolution
static flash int conv_delay[4]={100,200,400,800};
// valid temp. bit mask depending on the resolution
static flash unsigned bit_mask[4]={0xFFF8,0xFFFC,0xFFFE,0xFFFF};
//-------------------------------------------------

//-------------------------------------------------                     
unsigned char ds18b20_select(unsigned char *addr){
	unsigned char i;
	if (w1_init()==0) return 0;
	if (addr) {
		#asm("cli")
		w1_write(0x55);
		#asm("sei")
		i=0;
		do {
			#asm("cli")
			w1_write(*(addr++));
			#asm("sei")
		} while (++i<8);
	} else {
		#asm("cli")
		w1_write(0xcc);
		#asm("sei")
	}
	return 1;
}
//-------------------------------------------------

//-------------------------------------------------
unsigned char ds18b20_read(unsigned char *addr){
	unsigned char i;
	unsigned char *p;

	if (ds18b20_select(addr)==0) return 0;

	#asm("cli")
	w1_write(0xbe);
	#asm("sei")

	i=0;
	p=(char *) &ds18b20_scratch;
	do {
		#asm("cli")
		*(p++)=w1_read();
		#asm("sei")
	} while (++i<9);
	return !w1_dow_crc8(&ds18b20_scratch,9);
}
//-------------------------------------------------


//-------------------------------------------------
int ds18b20_temperature(unsigned char *addr){
	unsigned char resolution;
	
	if (ds18b20_select(addr)==0) return -9999;


	resolution=(ds18b20_scratch.conf_register>>5) & 3;

	#asm("cli")
	w1_write(0x44);
	#asm("sei")

	
	delay_ms(conv_delay[resolution]);
	
	
	if (ds18b20_read(addr)==0) return -9999;

	#asm("cli")
	w1_init();
	#asm("sei")
	
	return (*((int *) &ds18b20_scratch.temp_lsb) & bit_mask[resolution]);

}
//-------------------------------------------------



//-------------------------------------------------
ds18b20_temperature_data_struct ds18b20_temperature_struct(unsigned char *addr){
	int temperature;
	unsigned int temp;

    unsigned char j;
    ds18b20_temperature_data_struct result;
    
    result.valid = 0;
	result.minusChar = 0;
	result.halfDegree = 0;
	result.temperatureIntValue = 0;
    
    temperature = ds18b20_temperature(addr);

	if (temperature != -9999) { 
        result.valid = 1;
	 	temp = (unsigned int) temperature;
        if(temperature < 0){
            result.minusChar = 1;
            temp = ( ~temp ) + 0x0001;
        }
        result.halfDegree =  (temp & 0x0F) >> 3;
        temp = temp >> 4;   
        result.temperatureIntValue = 0; 
        for(j=0;j<8;j++){  
            if(temp & 0x01 == 1){  
                result.temperatureIntValue = result.temperatureIntValue + (1<<j);
            }
            temp = temp >> 1;
        }
	 }
	return result;
}
//-------------------------------------------------


#ifndef __MEASUREMENT_H__
#define __MEASUREMENT_H__

class Channel;
class Picoscope;

typedef enum picoVoltage
{
	U_50mV,
	U_100mV,
	U_200mV,
	U_500mV,
	U_1V,
	U_2V,
	U_5V,
	U_10V,
	U_20V,
	U_50V,
	U_100V,
	U_MAX
} PICO_VOLTAGE;

#include "picoscope.h"
// #include "channel.h"

/*
	- filename
	- are we going to use type 4000 or 6000 osciloscope?
	- which channels have to be enabled?
	- what rate (either "samples per second" or "time difference")
	- what length
	- what's the maximum memory usage
	- voltage
	- do we want to store in text or binary format or both?
 */


class Measurement {
public:
	Measurement(Picoscope *pico);
	~Measurement();

	// TODO: create a complete set
	// void SetMaxTimebase();
	void SetTimebaseInPs(unsigned long);
	void FixTimebase();

	void SetTimebaseInPicoscope(); // sends the information to picoscope

	void SetLength(unsigned long);

	unsigned long      GetTimebase()   const { return timebase; };
	unsigned long      GetLength()     const { return length;   };
	// unsigned long GetFileLength() const;
	unsigned long      GetMemLength()  const;

	void EnableChannels(bool a, bool b, bool c, bool d);
	int  GetNumberOfChannels() const;
	int  GetNumberOfEnabledChannels();

	void          SetMaxMemoryConsumption(unsigned long bytes);
	unsigned long GetMaxMemoryConsuption() const { return max_memory_consumption; };

	Picoscope*  GetPicoscope()  const { return picoscope; };
	PICO_SERIES GetSeries()     const { return GetPicoscope()->GetSeries(); };
	short       GetHandle()     const { return GetPicoscope()->GetHandle(); };
	Channel*    GetChannel(int);

	void RunBlock();

private:
	Picoscope         *picoscope;
	unsigned long      timebase;
	unsigned long      length;
	// unsigned long      fileLength; // length of a single file
	unsigned long      max_memory_consumption;    // in bytes
	unsigned long      max_trace_length_to_fetch; // max_memory_consumption / (sizeof(short) * number_of_channels)

	Channel *channels[PICOSCOPE_N_CHANNELS];
	short *data[PICOSCOPE_N_CHANNELS];

	// PICO_STATUS return_status;
};

#endif
#include <math.h>
#include <stdio.h>

#include "trigger.h"
#include "picoscope.h"
#include "channel.h"
#include "log.h"

#include "picoStatus.h"
#include "ps4000Api.h"
#include "ps6000Api.h"

Trigger::Trigger(Channel *ch, double x_fraction, double y_fraction)
{
	FILE_LOG(logDEBUG3) << "Trigger::Trigger (channel=" << ch << " (" << ch->GetIndex() << "), x_frac=" << x_fraction << ", y_frac=" << y_fraction << ")";

	channel = ch;

	if(channel == NULL) {
		throw "empty channel for trigger is not allowed.";
	}

	// default value of trigger position in the window
	// if trace is long, x_frac may not be too much
	if(x_fraction >= 0 && x_fraction <= 1) {
		x_frac = x_fraction;
	} else {
		throw "x fraction of trigger has to be between 0 and 1.";
	}
	if(y_fraction >= -1 && y_fraction <= 1) {
		y_frac = y_fraction;
	} else {
		throw "y fraction of trigger has to be between -1 and 1.";
	}

}

Channel*     Trigger::GetChannel()     const { return channel; };
Measurement* Trigger::GetMeasurement() const { return GetChannel()->GetMeasurement(); };
Picoscope*   Trigger::GetPicoscope()   const { return GetMeasurement()->GetPicoscope(); };
PICO_SERIES  Trigger::GetSeries()      const { return GetPicoscope()->GetSeries(); };
short        Trigger::GetHandle()      const { return GetPicoscope()->GetHandle(); };

short Trigger::GetThreshold()
{
	if(GetSeries() == PICO_4000) {
		return (short)round(GetYFraction()*(double)PS4000_MAX_VALUE);
	} else if(GetSeries() == PICO_6000) {
		return (short)round(GetYFraction()*(double)PS6000_MAX_VALUE/256) << 8;
	} else {
		throw "unknown pico series";
	}
}

double Trigger::GetThresholdInVolts()
{
	if(GetSeries() == PICO_4000) {
		return GetThreshold() / (double)PS4000_MAX_VALUE * GetChannel()->GetVoltageInVolts();
	} else if(GetSeries() == PICO_6000) {
		return GetThreshold() / (double)PS6000_MAX_VALUE * GetChannel()->GetVoltageInVolts();
	} else {
		throw "unknown pico series";
	}
}

void Trigger::SetTriggerInPicoscope()
{
	int   ch_index  = GetChannel()->GetIndex();
	short threshold = 0;
	// 256 is the minimum number that ever appears;
	// the multiplication factor in hysteresis is an approximation for noise
	// to make sure that we don't include noise in the trigger
	short hysteresis;


	// we will use just a single condition
	if(GetSeries() == PICO_6000) {
		hysteresis = 256 * 2;
		threshold = GetThreshold();
		fprintf(stderr, "-- Setting trigger threshold to %d (%g V = %g %% of %g V)\n", threshold,
			threshold/(double)PS6000_MAX_VALUE*GetChannel()->GetVoltageInVolts(),
			threshold/(double)PS6000_MAX_VALUE, GetChannel()->GetVoltageInVolts());

		struct tPS6000TriggerConditions conditions6000 = {
			(ch_index == 0) ? PS6000_CONDITION_TRUE : PS6000_CONDITION_DONT_CARE,
			(ch_index == 1) ? PS6000_CONDITION_TRUE : PS6000_CONDITION_DONT_CARE,
			(ch_index == 2) ? PS6000_CONDITION_TRUE : PS6000_CONDITION_DONT_CARE,
			(ch_index == 3) ? PS6000_CONDITION_TRUE : PS6000_CONDITION_DONT_CARE,
			PS6000_CONDITION_DONT_CARE,
			PS6000_CONDITION_DONT_CARE,
			PS6000_CONDITION_DONT_CARE };
		PS6000_THRESHOLD_DIRECTION direction6000 = (GetYFraction() > 0) ? PS6000_RISING : PS6000_FALLING;
		// struct tTriggerDirections directions6000;
		struct tPS6000TriggerChannelProperties properties6000 = {
			threshold, hysteresis,     // threshold & hysteresis upper
			threshold, hysteresis,     // threshold & hysteresis lower
			(PS6000_CHANNEL)ch_index,  // channel
			PS6000_LEVEL};             // thresholdMode

		GetPicoscope()->SetStatus(ps6000SetTriggerChannelConditions(
			GetHandle(),     // handle
			&conditions6000, // * conditions
			1));             // nConditions
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger conditions." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}

		GetPicoscope()->SetStatus(ps6000SetTriggerChannelDirections(
			GetHandle(),                                   // handle
			(ch_index == 0) ? direction6000 : PS6000_NONE, // channelA
			(ch_index == 1) ? direction6000 : PS6000_NONE, // channelB
			(ch_index == 2) ? direction6000 : PS6000_NONE, // channelC
			(ch_index == 3) ? direction6000 : PS6000_NONE, // channelD
			PS6000_NONE,                                   // ext
			PS6000_NONE));                                 // aux
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger directions." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}

		GetPicoscope()->SetStatus(ps6000SetTriggerChannelProperties(
			GetHandle(),     // handle
			&properties6000, // * channelProperties
			1,               // nChannelProperties
			0,               // auxOutputEnable (not used)
			0));             // autoTriggerMilliseconds (whether to autotrigger in case of no event)
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger properties." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}
	} else {
		threshold = GetThreshold();
		hysteresis = 256 * 3;
		fprintf(stderr, "-- Setting trigger threshold to %d\n", threshold);

		struct tPS4000TriggerConditions conditions4000 = {
			(ch_index == 0) ? PS4000_CONDITION_TRUE : PS4000_CONDITION_DONT_CARE,
			(ch_index == 1) ? PS4000_CONDITION_TRUE : PS4000_CONDITION_DONT_CARE,
			(ch_index == 2) ? PS4000_CONDITION_TRUE : PS4000_CONDITION_DONT_CARE,
			(ch_index == 3) ? PS4000_CONDITION_TRUE : PS4000_CONDITION_DONT_CARE,
			PS4000_CONDITION_DONT_CARE,
			PS4000_CONDITION_DONT_CARE,
			PS4000_CONDITION_DONT_CARE };
		PS4000_THRESHOLD_DIRECTION direction4000 = (GetYFraction() > 0) ? PS4000_RISING : PS4000_FALLING;
		// struct tTriggerDirections directions6000;
		struct tPS4000TriggerChannelProperties properties4000 = {
			threshold, hysteresis,     // threshold & hysteresis upper
			threshold, hysteresis,     // threshold & hysteresis lower
			(PS4000_CHANNEL)ch_index,  // channel
			PS4000_LEVEL};             // thresholdMode

		GetPicoscope()->SetStatus(ps4000SetTriggerChannelConditions(
			GetHandle(),     // handle
			&conditions4000, // * conditions
			1));             // nConditions
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger conditions." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}

		GetPicoscope()->SetStatus(ps4000SetTriggerChannelDirections(
			GetHandle(),                                   // handle
			(ch_index == 0) ? direction4000 : PS4000_NONE, // channelA
			(ch_index == 1) ? direction4000 : PS4000_NONE, // channelB
			(ch_index == 2) ? direction4000 : PS4000_NONE, // channelC
			(ch_index == 3) ? direction4000 : PS4000_NONE, // channelD
			PS4000_NONE,                                   // ext
			PS4000_NONE));                                 // aux
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger directions." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}

		GetPicoscope()->SetStatus(ps4000SetTriggerChannelProperties(
			GetHandle(),     // handle
			&properties4000, // * channelProperties
			1,               // nChannelProperties
			0,               // auxOutputEnable (not used)
			0));             // autoTriggerMilliseconds (whether to autotrigger in case of no event)
		if(GetPicoscope()->GetStatus() != PICO_OK) {
			std::cerr << "Unable to set trigger properties." << std::endl;
			throw Picoscope::PicoscopeException(GetPicoscope()->GetStatus());
		}
	}

}

/*
    Library to create Timers based on time_t calculations
    Author: Tobias Schenk
*/

#ifndef SimpleTimer_h
#define SimpleTimer_h

#include "Arduino.h"
#include <Time.h>
#include <TimeLib.h>

class SimpleTimer {
    public:
        // Start simple single Timer
        void startSingle(tmElements_t time,
                        unsigned long seconds);
        // Start Timer with multiple timing elements.
        void startDouble(tmElements_t time,
                        unsigned long secondsOne,
                        unsigned long secondsTwo,
                        int repeat);
        // Update the Timer based on the current Time
        void update(tmElements_t time);

        int getHours();
        int getMinutes();
        int getSeconds();
    
    private:
        // updates over time
        time_t _currentTime;
        // endTime of currentTimer    
        time_t _endTime;
        // Seconds of each Timer
        unsigned long _timerOne;
        unsigned long _timerTwo;
        // counter on which timer to apply
        int _timerCounter;
        // how many times to repeat Timer
        int _repeats;
        // Bool to determine multiple Timers
        bool _multipleTimers;
        // Simply to hold information about time
        int _hours;
        int _minutes;
        int _seconds;
};


#endif
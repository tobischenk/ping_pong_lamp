#include "Arduino.h"
#include "SimpleTimer.h"
#include <Time.h>
#include <TimeLib.h>


void SimpleTimer::startSingle(
    tmElements_t time,
    unsigned long timerOne) {
        _currentTime = makeTime(time);
        _endTime = _currentTime + timerOne;

        _timerOne = timerOne;

        _multipleTimers = false;

        update(time);
}

void SimpleTimer::startDouble(
    tmElements_t time,
    unsigned long timerOne,
    unsigned long timerTwo,
    int repeat) {
        _currentTime = makeTime(time);
        _endTime = _currentTime + timerOne;

        _timerOne = timerOne;
        _timerTwo = timerTwo;

        _multipleTimers = true;

        _repeats = repeat * 2;
        _timerCounter = 0;
    
        update(time);
}

void SimpleTimer::update(tmElements_t time) {
    _currentTime = makeTime(time);

    if(_currentTime >= _endTime) {
        if(_multipleTimers && (_repeats > _timerCounter)) {
            if(_timerCounter % 2 == 0) _endTime += _timerTwo;
            else _endTime += _timerOne;

            _timerCounter++;
            update(time);
        } else {
            _hours = 0;
            _minutes = 0;
            _seconds = 0;
        }
    } else {
        _hours = ((_endTime - _currentTime) / SECS_PER_HOUR) % 24;
        _minutes = ((_endTime - _currentTime) / SECS_PER_MIN) % 60;
        _seconds = (_endTime - _currentTime) % 60; 
    }
}

int SimpleTimer::getHours() {
    return _hours;
}

int SimpleTimer::getMinutes() {
    return _minutes;
}

int SimpleTimer::getSeconds() {
    return _seconds;
}
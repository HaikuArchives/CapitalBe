/*
 * Copyright 2009. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *	darkwyrm (Jon Yoder)
 */
#ifndef TIMESUPPORT_H
#define TIMESUPPORT_H

#include <String.h>

#include <time.h>

#define SECONDS_PER_DAY 86400
#define SECONDS_PER_WEEK 604800
#define SECONDS_PER_YEAR 31557600

time_t IncrementDateByDay(const time_t& t);
time_t DecrementDateByDay(const time_t& t);

time_t IncrementDateByWeek(const time_t& t);
time_t IncrementDateByMonth(const time_t& t);
time_t DecrementDateByMonth(const time_t& t);
time_t IncrementDateByQuarter(const time_t& t);
time_t IncrementDateByYear(const time_t& t);
time_t DecrementDateByYear(const time_t& t);

time_t GetCurrentDate();
time_t GetCurrentMonth();
time_t GetCurrentQuarter();
time_t GetCurrentYear();

time_t GetLastMonth();
time_t GetLastQuarter();
time_t GetLastYear();

BString GetShortMonthName(const uint8& number);

int GetQuarterForDate(const time_t& t);
int GetQuarterMonthForDate(const time_t& t);

bool IsLeapYear(int year);
int DaysInMonth(int month, int year);
int DayOfWeek(int day, int month, int year);
int DayOfYear(int day, int month, int year);

#endif	// TIMESUPPORT_H

#ifndef UTC_CLOCK_H
#define UTC_CLOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
// number of seconds since 0 hrs, 0 minutes, 0 seconds, on the
// 1st of January 2000 UTC
typedef uint32_t UTCTime;

// UTC time structs broken down until standard components.
typedef struct
{
  uint8_t seconds;  // 0-59
  uint8_t minutes;  // 0-59
  uint8_t hour;     // 0-23
  uint8_t day;      // 0-30
  uint8_t month;    // 0-11
  uint16_t year;    // 2000+
} UTCTimeStruct;

enum
{
	Year_err = -6,
	Month_err = -7,
	Day1_err = -8,
	Day2_err = -9,
	Day3_err = -10,
	Day4_err = -11,
	Hour_err = -12,
	Minutes_err = -13,
	Seconds_err = -14,
};


extern UTCTime UTC_convertUTCSecs( UTCTimeStruct *tm );
extern int Get_vck_tmsec(uint8_t *time_stamp, UTCTime* tmsec);
extern void Filter_time(UTCTimeStruct* ptTime , uint8_t* src_tm);

#ifdef __cplusplus
}
#endif

#endif /* UTC_CLOCK_H */

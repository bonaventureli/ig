/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include "utc_time.h"
#include "../spi/public.h"

/*********************************************************************
 * MACROS
 */
#define	YearLength(yr)	(IsLeapYear(yr) ? 366 : 365)
#define IsLeapYear(yr)     (!((yr) % 400) || (((yr) % 100) && !((yr) % 4)))
#define	BEGYEAR	           2000            //2000     // UTC started at 00:00:00 January 1, 2000
#define	DAY                86400UL  // 24 hours * 60 minutes * 60 seconds

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/
/*********************************************************************
 * @fn      UTC_monthLength
 *
 * @param   lpyr - 1 for leap year, 0 if not
 *
 * @param   mon - 0 - 11 (jan - dec)
 *
 * @return  number of days in specified month
 */
static uint8_t UTC_monthLength(uint8_t lpyr, uint8_t mon)
{
  uint8_t days = 31;

  if (mon == 1) // feb
  {
    days = (28 + lpyr);
  }
  else
  {
    if (mon > 6) // aug-dec
    {
      mon--;
    }

    if (mon & 1)
    {
      days = 30;
    }
  }

  return (days);
}


/*********************************************************************
 * @fn      UTC_convertUTCSecs
 *
 * @brief   Converts a UTCTimeStruct to UTCTime (from exact date to total
 *          seconds).
 *
 * @param   tm - pointer to provided struct.
 *
 * @return  number of seconds since 00:00:00 on 01/01/2000 (UTC).
 */
UTCTime UTC_convertUTCSecs(UTCTimeStruct *tm)
{
  uint32_t seconds;

  // Seconds for the partial day.
  seconds = (((tm->hour * 60UL) + tm->minutes) * 60UL) + tm->seconds;

  // Account for previous complete days.
  {
    // Start with complete days in current month.
    uint16_t days = tm->day;

    // Next, complete months in current year.
    {
      int8_t month = tm->month;
      while (--month >= 0)
      {
        days += UTC_monthLength(IsLeapYear(tm->year), month);
      }
    }

    // Next, complete years before current year.
    {
      uint16_t year = tm->year;
      while (--year >= BEGYEAR)
      {
        days += YearLength(year);
      }
    }

    // Add total seconds before partial day.
    seconds += (days * DAY);
  }

  return (seconds);
}


int Check_time(UTCTimeStruct* ptTime)
{
	int Ret = 0;
	
	if((ptTime->year < 2000 || ptTime->year > 2099)){
		Ret = Year_err;
		return Ret;
	}

	if(ptTime->month < 0 || ptTime->month > 11){
		Ret = Month_err;
		return Ret;
	}

	switch(ptTime->month)
	{
		case 1: 
		case 3: 
		case 5: 
		case 7: 
		case 8: 
		case 10: 
		case 12:
			if(ptTime->day < 0 || ptTime->day > 30){
				Ret=Day1_err;
			}
		break;
		case 4:
		case 6: 
		case 9:
		case 11:
			if(ptTime->day < 0 || ptTime->day > 29){
				Ret=Day2_err;
			}
		break;
		case 2:
			if(IsLeapYear(ptTime->year)){
				if(ptTime->day < 0 || ptTime->day > 28){
					Ret=Day3_err;
				}
			}
			else{
				if(ptTime->day < 0 || ptTime->day > 27){
					Ret=Day4_err;
				}
			}
		break;
	}
	
	if(ptTime->hour < 0 || ptTime->hour > 23){
		Ret = Hour_err;
		return Ret;
	}
	
	if(ptTime->minutes < 0 || ptTime->minutes > 59){
		Ret = Minutes_err;
		return Ret;
	}
	
	if(ptTime->seconds < 0 || ptTime->seconds > 59){
		Ret = Seconds_err;
		return Ret;
	}
	
	return Ret;
}


void Filter_time(UTCTimeStruct* ptTime , uint8_t* src_tm)
{
	int i = 0;
	uint8_t *p = src_tm;
	uint8_t pChar[14]={0};
	
	while(*p != 'Z'){
		if(*p == 'T'){
			p++;
			continue;
		}
		pChar[i]=*p;
		i++;
		p++;
	}

	DEBUG_PRINTF("pChar=%s\n",pChar);

	ptTime->year	= (pChar[0] - '0') * 1000 + (pChar[1] - '0') * 100 + (pChar[2] - '0') * 10 + (pChar[3] - '0');
	ptTime->month	= (pChar[4] - '0') * 10 + (pChar[5] - '0');
	ptTime->day		= (pChar[6] - '0') * 10 + (pChar[7] - '0');
	ptTime->hour 	= (pChar[8] - '0') * 10 + (pChar[9] - '0');
	ptTime->minutes	= (pChar[10] - '0') * 10 + (pChar[11] - '0');
	ptTime->seconds	= (pChar[12] - '0') * 10 + (pChar[13] - '0');
	
}


int Get_vck_tmsec(uint8_t *time_stamp, UTCTime* tmsec)
{
	int Ret = 0;
	UTCTimeStruct Utc_time;
	
	Filter_time(&Utc_time, time_stamp);

	Ret=Check_time(&Utc_time);
	if(Ret != 0)
	{
		return Ret;
	}
	
	Utc_time.month--;
	Utc_time.day--;
	*tmsec = UTC_convertUTCSecs(&Utc_time);
	return Ret;
}


#if 0
int main()
{
	int Ret=0; 
	uint8_t *time_stamp= NULL;
	UTCTime tmsec_start=0,tmsec_end=0;
	
	time_stamp="20000101T000000Z";
	Ret = Get_vck_tmsec(time_stamp, &tmsec_start);	
	printf("Ret=%d,tmsec_start=%ds\n",Ret,tmsec_start);
	
	time_stamp="20010207T040500Z";
	Ret = Get_vck_tmsec(time_stamp, &tmsec_end);	
	printf("Ret=%d,tmsec_end=%ds\n",Ret,tmsec_end);
	
	printf("diff_time=%ds\n",tmsec_end-tmsec_start);
	
	return 0;
}
#endif

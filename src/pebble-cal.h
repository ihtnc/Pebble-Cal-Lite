#include "pebble_os.h"
#include <string.h>

// Languages
#define LANG_DUTCH 0
#define LANG_ENGLISH 1
#define LANG_FRENCH 2
#define LANG_GERMAN 3
#define LANG_SPANISH 4
#define LANG_ITALIAN 5
#define LANG_FILIPINO 6
#define LANG_MAX 7

// Non Working Days Country
#define NWD_NONE 0
#define NWD_FRANCE 1
#define NWD_USA 2
#define NWD_PHILIPPINES 3
#define NWD_MAX 4

// Compilation-time options
#define LANG_CUR LANG_FILIPINO
#define NWD_COUNTRY NWD_PHILIPPINES
#define WEEK_STARTS_ON_SUNDAY true
#define SHOW_WEEK_NUMBERS true

#if LANG_CUR == LANG_DUTCH
#define APP_NAME "Kalender"
#elif LANG_CUR == LANG_FRENCH
#define APP_NAME "Calendrier"
#elif LANG_CUR == LANG_GERMAN
#define APP_NAME "Kalender"
#elif LANG_CUR == LANG_SPANISH
#define APP_NAME "Calendario"
#elif LANG_CUR == LANG_ITALIAN
#define APP_NAME "Calendario"
#elif LANG_CUR == LANG_FILIPINO
#define APP_NAME "Kalendaryo"
#else // Defaults to English
#define APP_NAME "Calendar"
#endif

#define SUN 0
#define MON 1
#define TUE 2
#define WED 3
#define THU 4
#define FRI 5
#define SAT 6

#define JAN 0
#define FEB 1
#define MAR 2
#define APR 3
#define MAY 4
#define JUN 5
#define JUL 6
#define AUG 7
#define SEP 8
#define OCT 9
#define NOV 10
#define DEC 11

static const char *monthNames[] = {
#if LANG_CUR == LANG_DUTCH
	"Januari", "Februari", "Maart", "April", "Mei", "Juni", "Juli", "Augustus", "September", "Oktober", "November", "December"
#elif LANG_CUR == LANG_FRENCH
	"Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"
#elif LANG_CUR == LANG_GERMAN
	"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"
#elif LANG_CUR == LANG_SPANISH
	"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Augusto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
#elif LANG_CUR == LANG_ITALIAN
	"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"
#elif LANG_CUR == LANG_FILIPINO
	"Enero", "Pebrero", "Marso", "Abril", "Mayo", "Hunyo", "Hulyo", "Agosto", "Setyembre", "Oktubre", "Nobyembre", "Disyembre"
#else // Defaults to English
	"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"
#endif
};

static const char *weekDays[] = {
#if LANG_CUR == LANG_DUTCH
	"Zo", "Ma", "Di", "Wo", "Do", "Vr", "Za"	// Dutch
#elif LANG_CUR == LANG_FRENCH
	"Di", "Lu", "Ma", "Me", "Je", "Ve", "Sa"	// French
#elif LANG_CUR == LANG_GERMAN
	"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"	// German
#elif LANG_CUR == LANG_SPANISH
	"Do", "Lu", "Ma", "Mi", "Ju", "Vi", "Sá"	// Spanish
#elif LANG_CUR == LANG_ITALIAN
	"Do", "Lu", "Ma", "Me", "Gi", "Ve", "Sa"	// Italian
#elif LANG_CUR == LANG_FILIPINO
	"Li", "Lu", "Ma", "Me", "Hu", "Bi", "Sa"	// Filipino
#else // Defaults to English
	"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"	// English
#endif
};

typedef struct 
{
	int day;
	int month;
	int year;
} Date;

static int julianDay(const Date *theDate) 
{
	int a = (int)((13-theDate->month)/12);
	int y = theDate->year+4800-a;
	int m = theDate->month + 12*a - 2;
	
	int day = theDate->day + (int)((153*m+2)/5) + y*365 + (int)(y/4) - (int)(y/100) + (int)(y/400) - 32045;
	return day;
}

static int dayOfWeek(const Date *theDate) 
{
	int J = julianDay(theDate);
	return (J+1)%7;
}

static bool isLeapYear(const int Y) 
{
	return (((Y%4 == 0) && (Y%100 != 0)) || (Y%400 == 0));
}

static int numDaysInMonth(const int M, const int Y) 
{
	static const int nDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	
	return nDays[M] + (M == FEB)*isLeapYear(Y);
}

static void dateAddDays(Date *date, int numDays) 
{
	int i;
	
	for (i=0; i<numDays; i++) {
		if (date->day == numDaysInMonth(date->month, date->year))
		{
			if (date->month == 11) 
			{
				date->year++;
				date->month = 0;
				date->day = 1;
			}
			else 
			{
				date->month++;
				date->day = 1;
			}
		} 
		else 
		{
			date->day++;
		}
	}
}

static void nthWeekdayOfMonth(const int Y, const int M, const int weekday, const int n, Date *theDate) 
{
	int firstDayOfMonth, curWeekday, count = 0;

	theDate->day = 1;
	theDate->month = M;
	theDate->year = Y;
	
	firstDayOfMonth = curWeekday = dayOfWeek(theDate);

	if (firstDayOfMonth == weekday) 
	{
		count = 1;
	}
	while (count < n) 
	{
		theDate->day++;
		curWeekday = (curWeekday+1)%7;
		if (curWeekday == weekday) 
		{
			count++;
		}
	}
}

static void lastWeekdayOfMonth(const int Y, const int M, const int weekday, Date *theDate) 
{
	int curWeekday;

	theDate->day = numDaysInMonth(M, Y);
	theDate->month = M;
	theDate->year = Y;

	curWeekday = dayOfWeek(theDate);
	while (curWeekday != weekday) 
	{
		theDate->day--;
		curWeekday = (curWeekday+6)%7;
	}
}

static int compareDates(Date *d1, Date *d2) 
{
	if (d1->year < d2->year) 
	{
		return -1;
	}
	else if (d1->year > d2->year) 
	{
		return 1;
	}
	else 
	{
		if (d1->month < d2->month) 
		{
			return -1;
		}
		else if (d1->month > d2->month) 
		{
			return 1;
		}
		else
		{
			if (d1->day < d2->day) 
			{
				return -1;
			}
			else if (d1->day > d2->day) 
			{
				return 1;
			}
			else 
			{
				return 0;
			}
		}
	}
}
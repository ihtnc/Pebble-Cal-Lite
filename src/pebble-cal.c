#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include <string.h>
#include "xprintf.h"
#include "pebble-cal.h"

static const char *windowName = APP_NAME;

#define MY_UUID { 0xC8, 0xB5, 0x9F, 0x6C, 0x76, 0xEC, 0x4B, 0x70, 0x9A, 0x35, 0xB4, 0xC4, 0xC9, 0x54, 0xE9, 0x84 }
PBL_APP_INFO(MY_UUID,
			 APP_NAME, "ihopethisnamecounts",
			 1, 0, /* App version */
			 RESOURCE_ID_IMAGE_MENU_ICON,
			 APP_INFO_WATCH_FACE);

#define SCREENW 144
#define SCREENH 168
#define MENUBAR_HEIGHT 16
#define MONTHNAME_LAYER_HEIGHT 20
#define MONTH_LAYER_HEIGHT (SCREENH-MENUBAR_HEIGHT-MONTHNAME_LAYER_HEIGHT)

Window window;
Layer monthLayer;
TextLayer monthNameLayer;
Date today;
int displayedMonth, displayedYear;
//GFont myFont;

static int DX, DY, DW, DH;

#define NUM_NON_WORKING_DAYS 11
typedef struct 
{
	char name[30];
	Date date;
} nonWorkingDay;

static char monthName[40] = "";

#if SHOW_WEEK_NUMBERS
static int weekNumber(const Date *theDate) 
{
	int J = julianDay(theDate);
	
	int d4 = (J+31741-(J%7))%146097%36524%1461;
	int L = (int)(d4/1460);
	int d1 = ((d4-L)%365)+L;
	
	return (int)(d1/7)+1;
}
#endif

#define Date(d, m, y) ((Date){ (d), (m), (y) })

#if NWD_COUNTRY == NWD_USA
static void MLKBirthday(const int Y, Date *theDate) 
{
	// Third monday in January
	nthWeekdayOfMonth(Y, JAN, MON, 3, theDate);
}

static void presidentDay(const int Y, Date *theDate) 
{
	// Third monday in February
	nthWeekdayOfMonth(Y, FEB, MON, 3, theDate);
}

static void memorialDay(const int Y, Date *theDate) 
{
	// Last monday in May
	lastWeekdayOfMonth(Y, MAY, MON, theDate);
}

static void laborDay(const int Y, Date *theDate) 
{
	// First monday in September
	nthWeekdayOfMonth(Y, SEP, MON, 1, theDate);
}

static void columbusDay(const int Y, Date *theDate) 
{
	// Second monday in October
	nthWeekdayOfMonth(Y, OCT, MON, 2, theDate);
}

static void thanksgivingThursday(const int Y, Date *theDate) 
{
	// Fourth thursday in november
	nthWeekdayOfMonth(Y, NOV, THU, 4, theDate);
}

static void thanksgivingFriday(const int Y, Date *theDate) 
{
	// Friday next to the fourth thursday in november
	thanksgivingThursday(Y, theDate);
	dateAddDays(theDate, 1);
}

static bool isNonWorkingDay(const Date *theDate) 
{
	Date d;
	
	if (theDate->day == 1  && theDate->month == JAN) return true; // New year's day
	if (theDate->day == 11 && theDate->month == NOV) return true; // Armistice 1918 // Veteran's day
	if (theDate->day == 25 && theDate->month == DEC) return true; // Noël // Christmas

	if (theDate->day == 4 && theDate->month == JUL) return true; // Independence day

	switch (theDate->month) 
	{
	case JAN:
		MLKBirthday(theDate->year, &d); 
		if (theDate->day == d.day && theDate->month == d.month) return true; // Martin Luther King Jr.'s Birthday
		break;
	case FEB:
		presidentDay(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // President's day
		break;
	case MAY:
		memorialDay(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // Memorial Day
		break;
	case SEP:
		laborDay(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // Labor Day
		break;
	case OCT:
		columbusDay(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // Columbus Day
		break;
	case NOV:
		thanksgivingThursday(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // Thanksgiving thursday
		thanksgivingFriday(theDate->year, &d);
        if (theDate->day == d.day && theDate->month == d.month) return true; // Thanksgiving friday
		break;
	}
	
	return false;
}
#endif //  NWD_COUNTRY == NWD_USA
	
#if NWD_COUNTRY == NWD_FRANCE
static void easterMonday(const int Y, Date *theDate) 
{
	int a = Y-(int)(Y/19)*19;
	int b = (int)(Y/100);
	int C = Y-(int)(Y/100)*100;
	int P = (int)(b/4);
	int E = b-(int)(b/4)*4;
	int F = (int)((b + 8) / 25);
	int g = (int)((b - F + 1) / 3);
	int h = (19 * a + b - P - g + 15)-(int)((19 * a + b - P - g + 15)/30)*30;
	int i = (int)(C / 4);
	int K = C-(int)(C/4)*4;
	int r = (32 + 2 * E + 2 * i - h - K) - (int)((32 + 2 * E + 2 * i - h - K)/7)*7;
	int N = (int)((a + 11 * h + 22 * r) / 451);
	int M = (int)((h + r - 7 * N + 114) / 31);
	int D = ((h + r - 7 * N + 114)-(int)((h + r - 7 * N + 114)/31)*31) + 1;
	
	if (D == numDaysInMonth(M-1, Y)) 
	{
		theDate->day = 1;
		theDate->month = M;
	} else {
		theDate->day = D+1;
		theDate->month = M-1;
	}
	theDate->year = Y;
}

static void ascensionDay(const int Y, Date *theDate) 
{
	easterMonday(Y, theDate);
	dateAddDays(theDate, 38);
}

static void whitMonday(const int Y, Date *theDate) 
{
	easterMonday(Y, theDate);
	dateAddDays(theDate, 49);
}

static bool isNonWorkingDay(const Date *theDate) {
	Date d;

	if (theDate->day == 1  && theDate->month == JAN) return true; // New year's day
	if (theDate->day == 11 && theDate->month == NOV) return true; // Armistice 1918 // Veteran's day
	if (theDate->day == 25 && theDate->month == DEC) return true; // Noël // Christmas
	
	if (theDate->day == 1  && theDate->month == MAY) return true; // Fête du travail
	if (theDate->day == 8  && theDate->month == MAY) return true; // Armistice 1945
	if (theDate->day == 14 && theDate->month == JUL) return true; // Fête nationale
	if (theDate->day == 15 && theDate->month == AUG) return true; // Assomption
	if (theDate->day == 1  && theDate->month == NOV) return true; // Toussaint

	easterMonday(theDate->year, &d);
	if (theDate->day == d.day && theDate->month == d.month) return true; // Lundi de Pâques
	ascensionDay(theDate->year, &d);
	if (theDate->day == d.day && theDate->month == d.month) return true; // Jeudi de l'ascension
	whitMonday(theDate->year, &d);
	if (theDate->day == d.day && theDate->month == d.month) return true; // Lundi de Pentecôte

	return false;
}
#endif // NWD_COUNTRY == NWD_FRANCE
	
#if NWD_COUNTRY == NWD_PHILIPPINES
static void easterMonday(const int Y, Date *theDate) 
{
	int a = Y-(int)(Y/19)*19;
	int b = (int)(Y/100);
	int C = Y-(int)(Y/100)*100;
	int P = (int)(b/4);
	int E = b-(int)(b/4)*4;
	int F = (int)((b + 8) / 25);
	int g = (int)((b - F + 1) / 3);
	int h = (19 * a + b - P - g + 15)-(int)((19 * a + b - P - g + 15)/30)*30;
	int i = (int)(C / 4);
	int K = C-(int)(C/4)*4;
	int r = (32 + 2 * E + 2 * i - h - K) - (int)((32 + 2 * E + 2 * i - h - K)/7)*7;
	int N = (int)((a + 11 * h + 22 * r) / 451);
	int M = (int)((h + r - 7 * N + 114) / 31);
	int D = ((h + r - 7 * N + 114)-(int)((h + r - 7 * N + 114)/31)*31) + 1;
	
	if (D == numDaysInMonth(M-1, Y)) 
	{
		theDate->day = 1;
		theDate->month = M;
	} else {
		theDate->day = D+1;
		theDate->month = M-1;
	}
	theDate->year = Y;
}

static void maundyThursday(const int Y, Date *theDate) 
{
	//adapted from France's NWD code
	//easter monday is the monday after easter sunday
	//maundy thursday is the thursday before easter sunday
	easterMonday(Y, theDate); 
	dateAddDays(theDate, -4); 
}

static void goodFriday(const int Y, Date *theDate) 
{
	//adapted from France's NWD code
	//easter monday is the monday after easter sunday
	//good friday is the friday before easter sunday
	easterMonday(Y, theDate);
	dateAddDays(theDate, -3);
}

static bool isNonWorkingDay(const Date *theDate) {
	Date d;

	if (theDate->day == 1 && theDate->month == JAN) return true; // New year's day
	if (theDate->day == 9 && theDate->month == APR) return true; // Day of valor
	if (theDate->day == 1  && theDate->month == MAY) return true; // Labor day
	if (theDate->day == 12  && theDate->month == JUN) return true; // Independence day
	if (theDate->day == 26 && theDate->month == AUG) return true; // National heroes day
	if (theDate->day == 30 && theDate->month == NOV) return true; // Bonifacio day
	if (theDate->day == 25  && theDate->month == DEC) return true; // Christmas day
	if (theDate->day == 30  && theDate->month == DEC) return true; // Rizal day

	maundyThursday(theDate->year, &d);
	if (theDate->day == d.day && theDate->month == d.month) return true; // Maundy thursday
	goodFriday(theDate->year, &d);
	if (theDate->day == d.day && theDate->month == d.month) return true; // Good friday
	
	return false;
}
#endif // NWD_COUNTRY == NWD_PHILIPPINES

#if NWD_COUNTRY == NWD_NONE
static bool isNonWorkingDay(const Date *theDate) 
{ 
	return false;
}
#endif //NWD_COUNTRY == NWD_NONE
	
void updateMonthText() 
{
	xsprintf(monthName, "%s %d", monthNames[displayedMonth], displayedYear);
	text_layer_set_text(&monthNameLayer, monthName);
}

void updateMonth(Layer *layer, GContext *ctx) 
{
	static char numStr[3] = "";
	int i, x, s, numWeeks, dy, firstday, numDays, l=0, c=0, w;
	Date first, d;
	GFont f, fontNorm, fontBold;
	GRect rect, fillRect;
	
	first = Date(1, displayedMonth, displayedYear);
#if WEEK_STARTS_ON_SUNDAY
	firstday = dayOfWeek(&first);
#else
	firstday = (dayOfWeek(&first)+6)%7;
#endif
	numDays = numDaysInMonth(displayedMonth, displayedYear);
	
	numWeeks = (firstday+6+numDays)/7;
	
	dy = DY + DH*(6-numWeeks)/2;
	
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	// Calendar Grid
#if SHOW_WEEK_NUMBERS
	x = DX+DW;
#else
	x = DX;
#endif

	// Black Top Line with days of week
	graphics_fill_rect(ctx, GRect(x, dy, DW*7+1, DH), 4, GCornersTop);

#if SHOW_WEEK_NUMBERS
	// Black left column for week numbers
	graphics_fill_rect(ctx, GRect(DX, dy+DH, DW, numWeeks*DH+1), 4, GCornersLeft);
#endif

#if SHOW_WEEK_NUMBERS
	x = DX+DW;
	w = DW*7;
#else
	x = DX+1;
	w = DW*7-1;
#endif
	// Double line on the outside
	graphics_draw_round_rect(ctx, GRect(x, dy+DH, w, numWeeks*DH), 0);
	
	// Column(s) for the week-end or sunday
#if WEEK_STARTS_ON_SUNDAY
	x = DX+DW+1;
#else
	x = DX+5*DW+1;
#endif

#if SHOW_WEEK_NUMBERS
	x += DW;
#endif

	graphics_draw_line(ctx, GPoint(x, dy+DH), GPoint(x, dy+DH+numWeeks*DH-1));
	
#if SHOW_WEEK_NUMBERS
	x = 1;
#else
	x = 0;
#endif

	// Vertical lines
	for (i=x; i<=x+7; i++) 
	{
		graphics_draw_line(ctx, GPoint(DX+DW*i,dy+DH), GPoint(DX+DW*i,dy+(numWeeks+1)*DH));
	}
	// Horizontal lines
	for (i=1; i<=(numWeeks+1); i++) 
	{
		graphics_draw_line(ctx, GPoint(DX+x*DW,dy+DH*i), GPoint(DX+DW*(7+x),dy+DH*i));
	}
	
	fontNorm = fonts_get_system_font(FONT_KEY_GOTHIC_18);
	fontBold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
	f = fontNorm;
	
#if WEEK_STARTS_ON_SUNDAY
	s = 0;
#else
	s = 1;
#endif

#if SHOW_WEEK_NUMBERS
	x = 1;
#else
	x = 0;
#endif

	// Days of week
	graphics_context_set_text_color(ctx, GColorWhite);
	
	for (i=s; i<s+7; i++) 
	{
		graphics_text_draw(ctx, weekDays[i%7], fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX+DW*(i+x-s), dy, DW+2, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
	
#if SHOW_WEEK_NUMBERS
	// Week numbers
	for (i=0, d=first; i<=numWeeks; i++, d.day+=7) 
	{
		xsprintf(numStr, "%d", weekNumber(&d));
		graphics_text_draw(ctx, numStr, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(DX, dy+DH*(i+1), DW, DH+1), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
#endif
	
	// Day numbers
	graphics_context_set_text_color(ctx, GColorBlack);
	
	for (i=1; i<=numDays; i++) 
	{
		c = (firstday - 1 + i)%7;
		if (c == 0 && i != 1) 
		{
			l++;
		}
		
		xsprintf(numStr, "%d", i);

		if (isNonWorkingDay(&Date(i, displayedMonth, displayedYear))) 
		{
			f = fontBold;
		}
		else 
		{
			f = fontNorm;
		}
		
		fillRect = GRect(DX+DW*(c+x), dy+DH*(l+1), DW, DH);
		rect = GRect(DX+DW*(c+x), dy+DH*(l+1)-3, DW+1, DH+1);
		
		if (today.day == i && today.month == displayedMonth && today.year == displayedYear) 
		{
			graphics_fill_rect(ctx, fillRect, 0, GCornerNone);
			graphics_context_set_text_color(ctx, GColorWhite);
			graphics_text_draw(ctx, numStr, f, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		}
		else
		{
			graphics_context_set_text_color(ctx, GColorBlack);
			graphics_text_draw(ctx, numStr, f, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		}
	}
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t) 
{
	updateMonthText();
	layer_mark_dirty(&monthLayer);
}

void handle_init(AppContextRef ctx) 
{
	PblTm now;
	
	window_init(&window, windowName);
	window_stack_push(&window, true /* Animated */);

	resource_init_current_app(&APP_RESOURCES);

	get_time(&now);
	today.day = now.tm_mday;
	today.month = now.tm_mon;
	today.year = now.tm_year + 1900;
	
	displayedMonth = today.month;
	displayedYear = today.year;
	
#if SHOW_WEEK_NUMBERS
	DW = (SCREENW-2)/8;
	DX = 1 + (SCREENW-2 - 8*DW)/2;
#else
	DW = (SCREENW-2)/7;
	DX = 1 + (SCREENW-2 - 7*DW)/2;
#endif
	DH = MONTH_LAYER_HEIGHT/7;
	DY = (MONTH_LAYER_HEIGHT - 7*DH)/2;

	text_layer_init(&monthNameLayer, GRect(0, 0, SCREENW, MONTHNAME_LAYER_HEIGHT));
	text_layer_set_background_color(&monthNameLayer, GColorWhite);
	text_layer_set_text_color(&monthNameLayer, GColorBlack);
	text_layer_set_font(&monthNameLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(&monthNameLayer, GTextAlignmentCenter);
	layer_add_child(&window.layer, &monthNameLayer.layer);
	
	updateMonthText();
	
	layer_init(&monthLayer, GRect(0, MONTHNAME_LAYER_HEIGHT, SCREENW, MONTH_LAYER_HEIGHT));
	layer_set_update_proc(&monthLayer, &updateMonth);
	layer_add_child(&window.layer, &monthLayer);
}

void handle_deinit(AppContextRef ctx) 
{
}

void pbl_main(void *params) 
{
	PebbleAppHandlers handlers = 
	{
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		
		.tick_info = 
		{
			.tick_handler = &handle_tick,
			.tick_units = DAY_UNIT
		}
	};
	app_event_loop(params, &handlers);
}

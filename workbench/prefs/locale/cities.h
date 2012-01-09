/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Array of Cities with their time difference to GMT
    Lang: English
*/

/*********************************************************************************************/


struct CityType
{
  char  *city_name;
  LONG         timediff;
};

static const struct CityType CityArray[] =
{
 {"Abu Dhabi", +400},
 {"Adelaide", +930},
 {"Alaska (YST)", -900},
 {"Almaty (ZP6)", +600},
 {"Amsterdam", +100},
 {"Athens", +200},
 {"Azores", -100},
 {"Baghdad", +300},
 {"Bangkok", +700},
 {"Beijing", +800},
 {"Berlin", +100},
 {"Bombay", +530},
 {"Budapets", +100},
 {"Buenos Aires", -300},
 {"Cairo", +200},
 {"Calcutta", +530},
 {"Canada Atlantic Time (AST/EDT)", -400},
 {"Cape Verde Island (WAT)", -100},
 {"Casablanca (GMT/UTC)", +0},
 {"Darwin", +930},
 {"Dhaka", +600},
 {"Eastern Europe", +200},
 {"Eniwetok", -1200},
 {"Georgetown", -300},
 {"Guam", +1000},
 {"Hanoi", +700},
 {"Hawaii (HST)", -1000},
 {"Hobart (EAST)", +1000},
 {"Hong Kong", +800},
 {"Islamabad", +500},
 {"Israel", +200},
 {"Istanbul (EET)", +200},
 {"Jakarta (WAST)", +700},
 {"Kabul", +430},
 {"Karachi (ZP5)", +500},
 {"Kuwait (BT)", +300},
 {"Kwajalein (IDLW)", -1200},
 {"London", +0},
 {"Marshall Islands (IDLE)", +1200},
 {"Mexico City (CST/MDT)", -600},
 {"Mid Atlantic (AT)", -200},
 {"Midway Islands", -1100},
 {"Moscow", +300},
 {"Munich", +100},
 {"Muscat", +400},
 {"New Caledonia", +1000},
 {"New Delhi", +530},
 {"Newfoundland", -330},
 {"New Zealand", +1200},
 {"Paris", +100},
 {"Rome (CET)", +100},
 {"Samoa (NZST)", +1200},
 {"Seoul", +900},
 {"Singapore (CCT)", +800},
 {"Solomon Islands", +1000},
 {"Stockholm", +100},
 {"St. Petersburg", +300},
 {"Sydney", +1000},
 {"Tbilisi(ZP4)", +400},
 {"Tehran", +330},
 {"Tijuana (PST)", -800},
 {"Tokyo (JST)", +900},
 {"USA Central Daylight (CDT)", -500},
 {"USA Central Standard", -600},
 {"USA Eastern Standard (EST)", -500},
 {"USA Mountain Daylight", -600},
 {"USA Mountain Standard (MST)", -700},
 {"USA Pacific Daylight (PDT)", -700},
 {"USA Pacific Standard", -800},
 { NULL, 0}
};

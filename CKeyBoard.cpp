/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * ProWo
 * Copyright (C) root 2014 <root@raspberrypi>
 * 
ProWo is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ProWo is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ProWo.h"

int CKeyBoard::getbutton()
{
    int iRet = 0;
    
 	// Button Pfeil nach oben
	if(!check_gpio (22))
        iRet = BUTTONUP;
	// Button Pfeil nach unten
	if(!check_gpio(23))
        iRet = BUTTONDOWN;
	// Button OK
	if(!check_gpio (24))
        iRet = BUTTONOK;
	// Button Abbruch
	if(!check_gpio (25))
        iRet = BUTTONCANCEL;
    
    return iRet;
}
int CKeyBoard::getch()
{
	int button;
	
	// Button Pfeil nach oben
	buttonup = !check_gpio (22);
	// Button Pfeil nach unten
	buttondown = !check_gpio(23);
	// Button OK
	buttonok = !check_gpio (24);
	// Button Abbruch
	buttoncancel = !check_gpio (25);

	button = 0;
	if(buttonok)
	{   
		if(!lastbuttonok)
		{
			button = BUTTONOK;
			lastbuttonok = 1;
		}
		else
		{   buttonokhold++;
			if(buttonokhold == 1000)
				button = BUTTONOKHOLD;
		}
	}
	else
	{   lastbuttonok = 0;
		buttonokhold = 0;
	}
	
	if(buttoncancel)
	{
		if(!lastbuttoncancel)
		{
			button = BUTTONCANCEL;
			lastbuttoncancel = 1;
		}
		else
		{   buttoncancelhold++;
			if(buttoncancelhold == 1000)
				button = BUTTONCANCELHOLD;
		}
	}
	else
	{
		lastbuttoncancel = 0;
		buttoncancelhold = 0;
	}
	
	if(buttonup)
	{
		if(!lastbuttonup && !button)
		{
			button = BUTTONUP;
			lastbuttonup = 1;
		}
	}
	else
		lastbuttonup = 0;
	
	if(buttondown)
	{
		if(!lastbuttondown && !button)
		{
			button = BUTTONDOWN;
			lastbuttondown = 1;
		}
	}
	else
		lastbuttondown = 0;

	if(buttonup && buttondown)
		return BUTTONUPDOWN;

	if(buttoncancel && buttonok)
		return BUTTONOKCANCEL;

	if(buttonup && button == BUTTONCANCELHOLD)
		return BUTTONUPCANCEL;

	return button;
}
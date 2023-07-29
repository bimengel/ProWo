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

#ifndef _CKEYBOARD_H_
#define _CKEYBOARD_H_

#define BUTTONUP	 		1
#define BUTTONDOWN	 		2
#define BUTTONOK	 		3
#define BUTTONCANCEL 		4
#define BUTTONOKHOLD 		5
#define BUTTONCANCELHOLD  	6
#define BUTTONUPDOWN		7 // Stop program
#define BUTTONOKCANCEL		8 // Reboot
#define BUTTONUPCANCEL		9 // 22.06.16 für Fehleranalyse

class CKeyBoard
{
public:
	int getch();
    int getbutton();
protected:

private:
	int buttonup, buttondown, buttonok, buttoncancel;
	int lastbuttonup, lastbuttondown, lastbuttonok, lastbuttoncancel;
	int buttonokhold, buttoncancelhold;
};

#endif // _CKEYBOARD_H_

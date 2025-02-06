/****************************************************************************
FileName    : kernel/arch/arm/mach-tcc893x/include/mach/tcc_fts_ioctl.h
Description : 

Copyright (C) 2013 Telechips Inc.

This program is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA 02111-1307 USA
****************************************************************************/

#ifndef _FTS_IOCTL_H
#define _FTS_IOCTL_H

#define CRASH_COUNTER_RESET		0x1000
#define CRASH_COUNTER_SET		0x1001
#define CRASH_COUNTER_GET		0x1002

#define OUTPUT_SETTING_SET		0x2000
#define OUTPUT_SETTING_GET		0x2001

#endif

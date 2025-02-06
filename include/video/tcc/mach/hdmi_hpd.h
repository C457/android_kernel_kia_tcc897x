/****************************************************************************
 * FileName    : kernel/arch/arm/mach-tcc893x/include/mach/hdmi_hpd.h
 * Description : hdmi hpd driver
 *
 * Copyright (C) 2013 Telechips Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 * ****************************************************************************/

#ifndef _LINUX_HPD_H_
#define _LINUX_HPD_H_

#define HPD_IOC_MAGIC        'p'

/**
 * HPD device request code to set logical address.
 */
#define HPD_IOC_START	 _IO(HPD_IOC_MAGIC, 0)
#define HPD_IOC_STOP	 _IO(HPD_IOC_MAGIC, 1)

#define HPD_IOC_BLANK	 _IOW(HPD_IOC_MAGIC,50, unsigned int)

#endif /* _LINUX_HPD_H_ */

/****************************************************************************
 * FileName    : kernel/arch/arm/mach-tcc893x/include/mach/cec.h
 * Description : hdmi cec driver
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

#ifndef _LINUX_CEC_H_
#define _LINUX_CEC_H_

#define CEC_IOC_MAGIC        'c'

/**
 * CEC device request code to set logical address.
 */
#define CEC_IOC_SETLADDR     _IOW(CEC_IOC_MAGIC, 0, unsigned int)
#define CEC_IOC_SENDDATA	 _IOW(CEC_IOC_MAGIC, 1, unsigned int)
#define CEC_IOC_RECVDATA	 _IOW(CEC_IOC_MAGIC, 2, unsigned int)

#define CEC_IOC_START	 _IO(CEC_IOC_MAGIC, 3)
#define CEC_IOC_STOP	 _IO(CEC_IOC_MAGIC, 4)

#define CEC_IOC_GETRESUMESTATUS _IOR(CEC_IOC_MAGIC, 5, unsigned int)
#define CEC_IOC_CLRRESUMESTATUS _IOW(CEC_IOC_MAGIC, 6, unsigned int)

#define CEC_IOC_BLANK			   _IOW(CEC_IOC_MAGIC,50, unsigned int)

#endif /* _LINUX_CEC_H_ */

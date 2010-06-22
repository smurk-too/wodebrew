/*
 *  Copyright (C) 2008 Nuke (wiinuke@gmail.com)
 *
 *  this file is part of GeckoOS for USB Gecko
 *  http://www.usbgecko.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>

#include "ocarina.h"

extern void patchhook(u32 address, u32 len);

static const u32 viwiihooks[4] = {
	0x7CE33B78,0x38870034,0x38A70038,0x38C7004C 
};

void dogamehooks(void *addr, u32 len){
	void *addr_start = addr;
	void *addr_end = addr+len;

	while(addr_start < addr_end){

		//switch(hooktype){
			//case 0:
			//	break;

			//case 1:
				if(memcmp(addr_start, viwiihooks, sizeof(viwiihooks))==0){
				//	printf("\n\n\n");
				//	printf("found at address %x\n", addr_start);
				//	sleep(2);
					patchhook((u32)addr_start, len);
				//	patched = 1;
				//	hooktype = 1;
				}
			//break;

		//}
		addr_start += 4;
	}
}

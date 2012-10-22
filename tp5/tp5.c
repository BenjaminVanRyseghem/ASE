#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tp5.h"
#include "include/hardware.h"
#include "dump.h"

#define DISK_SECT_SIZE_MASK 0x0000FF

static void
empty_it()
{
    return;
}

void gotoSector(int cyl, int sect){
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		printf("Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(cyl<0 || cyl > HDA_MAXSECTOR){
		printf("Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	
	_out(HDA_DATAREGS, (cyl>8) & 0xFF);
	_out(HDA_DATAREGS+1, cyl & 0xFF);
	_out(HDA_DATAREGS+2, (sect>8) & 0xFF);
	_out(HDA_DATAREGS+3, sect & 0xFF);
	_out(HDA_CMDREG, CMD_SEEK);
	printf("goto: before sleep\n");
	_sleep(HDA_IRQ);
}

void read_sector(unsigned int cyl, unsigned int sect, unsigned char *buffer){
	int i;
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		printf("Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(cyl<0 || cyl > HDA_MAXSECTOR){
		printf("Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	
	gotoSector(cyl, sect);
	_out(HDA_DATAREGS, 0);
	_out(HDA_DATAREGS+1, 1);
	_out(HDA_CMDREG, CMD_READ);
	_sleep(HDA_IRQ);
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		buffer[i] = MASTERBUFFER[i];
	}
}

void write_sector(unsigned int cyl, unsigned int sect, const unsigned char *buffer){
	int i;
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		printf("Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(cyl<0 || cyl > HDA_MAXSECTOR){
		printf("Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	printf("Goto\n");
	gotoSector(cyl, sect);
	printf("Initialization\n");
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		MASTERBUFFER[i] = buffer[i];
	}
	printf("Set args\n");
	_out(HDA_DATAREGS, 0);
	_out(HDA_DATAREGS+1, 1);
	printf("Write\n");
	_out(HDA_CMDREG, CMD_WRITE);
	printf("before sleep\n");
	_sleep(HDA_IRQ);
}

void format(){
	int i;
	for(i = 0 ; i < HDA_MAXCYLINDER; i ++){
		printf("cylinder: %d\n",i);
		format_sector(i, 0, HDA_MAXSECTOR, 0);
	}
	
}

void format_sector(unsigned int cylinder, unsigned int sector, unsigned int nsector,
unsigned int value){
	int i;
	unsigned char buffer[HDA_SECTORSIZE];
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		buffer[i] = value;
	}
	for(i = sector ; i < sector+ nsector; i ++){
		printf("sector: %d\n",i);
		write_sector(cylinder,i,buffer);
	}
}

/* return 1 if the size is correct, 0 otherwise */
int check_sector_size(){
	int real_value;
	_out(HDA_CMDREG, CMD_DSKINFO);
	real_value = ((int)MASTERBUFFER) & DISK_SECT_SIZE_MASK;
	return real_value == HDA_SECTORSIZE;
}

int main(){
	int i;
	if(init_hardware("etc/hardware.ini") == 0) {
		fprintf(stderr, "Error in hardware initialization\n");
		exit(EXIT_FAILURE);
    }
	
	if(!check_sector_size()){
		exit(EXIT_FAILURE);
	}
	
	 /* Interreupt handlers */
    for(i=0; i<16; i++) IRQVECTOR[i] = empty_it;

    /* Allows all IT */
    _mask(1);
	
	unsigned char buffer[HDA_SECTORSIZE];
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		buffer[i] = 0;
	}
	write_sector(0, 0, buffer);
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		buffer[i] = 1;
	}
	read_sector(0, 0, buffer);
	dump(buffer, HDA_SECTORSIZE, 1, 1);
	format();
	return EXIT_SUCCESS;
}
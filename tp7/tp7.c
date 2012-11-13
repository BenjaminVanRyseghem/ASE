#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tp7.h"
#include "include/hardware.h"
#include "dump.h"

#define DISK_SECT_SIZE_MASK 0x0000FF
#define MAX_VOL 8
#define MBR_MAGIC 0xCAFE
#define SPR_MAGIC 0xBABE
#define SUPER_INDEX 0
#define BLOC_NULL 0

enum vd_type_e {VT_BASE, VT_ANX, VT_OTH};

struct vd_descr_s{
	unsigned vd_first_sector;
	unsigned vd_first_cylinder;
	unsigned vd_n_sector;
	enum vd_type_e vd_val_type;
};

struct mbr_s{
	unsigned mbr_nvol;
	struct vd_descr_s mbr_vols[MAX_VOL];
	unsigned mbr_magic;
};

struct first_free_bloc_s{
	unsigned int fb_nbloc;
	unsigned int fb_next;
};

struct superbloc_s{
	unsigned super_magic;
	unsigned int super_first_free;
	unsigned int super_occupation;
};

struct mbr_s mbr;
struct superbloc_s current_super;
unsigned current_volume;

void init_super(unsigned int vol){
	struct superbloc_s super;
	super.super_first_free = 1;
	super.super_occupation = 0;
	super.super_magic = SPR_MAGIC;
	struct first_free_bloc_s first;
	first.fb_nbloc = mbr.mbr_vols[vol].vd_n_sector -1;
	first.fb_next = BLOC_NULL;
	
	write_bloc_n(vol, SUPER_INDEX, (char*)&super, sizeof(struct superbloc_s));
	write_bloc_n(vol, SUPER_INDEX+1, (char*)&first, sizeof(struct first_free_bloc_s));
}

void mkfs(){
	unsigned vol = current_volume;
	init_super(vol);
}

int load_super(){
	unsigned vol = current_volume;
	if(vol<0 || vol>= mbr.mbr_nvol){
		fprintf(stderr,"Can't load because the current volume index is wrong\n");
		return 1;
	}
	read_bloc_n(vol, SUPER_INDEX,(char*)&current_super, sizeof(struct superbloc_s));
}

int save_super(){
	unsigned vol = current_volume;
	if(vol<0 || vol>= mbr.mbr_nvol){
		return 1;
	}
	write_bloc_n(vol, SUPER_INDEX,(char*)&current_super, sizeof(struct superbloc_s));
}

// void printVolume(){
// 	printf("| S ");
// 	
// 	
// }



unsigned ask_for(char* phrase){
	unsigned result;
	printf("%s",phrase);

	if(scanf("%i",&result) == 0) return -1;
	return result;
}

void set_current_volume(unsigned vol){
	if(vol<0 || vol >= mbr.mbr_nvol){
		fprintf(stderr, "Wrong volume value\n\tentered value: %d\n\trange: 0 - %d\n",vol,mbr.mbr_nvol-1);
		return ;
	}
	current_volume = vol;
}

void interactive_set_current_volume(){
	unsigned vol = ask_for("Choose new volume index\n");
	set_current_volume(vol);
}

unsigned int new_bloc_on(unsigned vol){
	
	if(vol<0 || vol >= mbr.mbr_nvol){
		fprintf(stderr, "Wrong volume value\n\tentered value: %d\n\trange: 0 - %d\n",vol,mbr.mbr_nvol-1);
		return 0;
	}
	
	if(current_super.super_magic != SPR_MAGIC){ 
		fprintf(stderr, "Load a super bloc first\n");
		return 0;
	}
	struct first_free_bloc_s first;
	unsigned int n_bloc = current_super.super_first_free;
	if(n_bloc == BLOC_NULL) return BLOC_NULL; 
	read_bloc_n(vol, n_bloc, (char*)&first, sizeof(struct first_free_bloc_s));
	if(first.fb_nbloc == 1){
		current_super.super_first_free = first.fb_next;
	}
	else {
		current_super.super_first_free++;
		first.fb_nbloc--;
		write_bloc_n(vol, current_super.super_first_free, (char*)&first, sizeof(struct first_free_bloc_s));
	}
	current_super.super_occupation++;
	return n_bloc;
}

unsigned int interactive_new_bloc(){
	unsigned vol = ask_for("Enter volume index:\n");
	return new_bloc_on(vol);
}

unsigned int new_bloc(){
	unsigned vol = current_volume;
	return new_bloc_on(vol);
}

void free_bloc(unsigned int bloc){
	unsigned vol = current_volume;
	if(current_super.super_magic != SPR_MAGIC){ 
		fprintf(stderr, "Load a super bloc first\n");
		return;
	}
	
	struct first_free_bloc_s first;
	first.fb_nbloc = 1;
	first.fb_next = current_super.super_first_free;
	current_super.super_first_free = bloc;
	write_bloc_n(vol, bloc, (char*)&first, sizeof(struct first_free_bloc_s));
	current_super.super_occupation--;	
}

int create_new_volume(unsigned f_s, unsigned f_c, unsigned n_s, enum vd_type_e type){
	if(mbr.mbr_nvol == MAX_VOL){
		fprintf(stderr, "Max number of volumes reached\n");
		return -1;
	}
	struct vd_descr_s new_vol;
	new_vol.vd_first_sector = f_s;
	new_vol.vd_first_cylinder = f_c;
	new_vol.vd_n_sector = n_s;
	new_vol.vd_val_type = type;
	
	mbr.mbr_vols[mbr.mbr_nvol] = new_vol;
	printf("%d, %d,  %d\n\n",f_c,f_s,n_s);
	
	printf_vol(mbr.mbr_nvol);
	return mbr.mbr_nvol++;
}

enum vd_type_e retrieve_type_from_string(char *buffer){
	if (!strcmp(buffer, "VT_BASE")) return VT_BASE;
	if (!strcmp(buffer, "VT_ANX")) return VT_ANX;
	if (!strcmp(buffer, "VT_OTH")) return VT_OTH;
	return -1;
}

int interactive_create_new_volume(){
	unsigned f_s;
	unsigned f_c;
	unsigned n_s;
	enum vd_type_e type;
	char buffer[1024];

	printf("Please enter the first cylinder:\n");
	if(scanf("%i",&f_c) == 0) return -1;
	printf("Please enter the first sector:\n");
	if(scanf("%i",&f_s) == 0) return -1;
	printf("Please enter the number of sectors:\n");
	if(scanf("%i",&n_s) == 0) return -1;
	printf("Please enter the volume type (VT_BASE, VT_ANX, VT_OTH):\n");
	if(scanf("%s",buffer) == 0) return -1;
	
	type = retrieve_type_from_string(buffer);
	
	return create_new_volume(f_s, f_c, n_s, type);
}

int delete_volume(unsigned int vol){
	int i;
	if (vol<0 || vol >= mbr.mbr_nvol){
		fprintf(stderr, "Can't delete this volume, the index is wrong\n");
		return 1;
	}
	for ( i = vol+1 ; i < mbr.mbr_nvol; i ++){
		mbr.mbr_vols[i-1] = mbr.mbr_vols[i];
	}
	mbr.mbr_nvol--;	
	return 0;
}

int interactive_delete_volume(){
	unsigned int vol;
	
	printf("Please enter the volume index to remove:\n");
	if(scanf("%i",&vol) == 0) return 1;
	return delete_volume(vol);
}

static void empty_it(){
    return;
}

void gotoSector(int cyl, int sect){
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		fprintf(stderr, "Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(sect<0 || sect > HDA_MAXSECTOR){
		fprintf(stderr, "Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	
	_out(HDA_DATAREGS, (cyl>>8) & 0xFF);
	_out(HDA_DATAREGS+1, cyl & 0xFF);
	_out(HDA_DATAREGS+2, (sect>>8) & 0xFF);
	_out(HDA_DATAREGS+3, sect & 0xFF);
	_out(HDA_CMDREG, CMD_SEEK);
	_sleep(HDA_IRQ);
}

void read_sector_n(unsigned int cyl, unsigned int sect, unsigned char *buffer, int size){
	int i;
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		fprintf(stderr, "Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(sect<0 || sect > HDA_MAXSECTOR){
		fprintf(stderr, "Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	
	if(size<0 || size > HDA_SECTORSIZE){
		fprintf(stderr, "Wrong size\n");
		exit(EXIT_FAILURE);
	}
	
	gotoSector(cyl, sect);
	_out(HDA_DATAREGS, 0);
	_out(HDA_DATAREGS+1, 1);
	_out(HDA_CMDREG, CMD_READ);
	_sleep(HDA_IRQ);
	for (i = 0; i < size ; i ++){
		buffer[i] = MASTERBUFFER[i];
	}
}

void read_sector(unsigned int cyl, unsigned int sect, unsigned char *buffer){
	read_sector_n(cyl, sect, buffer, HDA_SECTORSIZE);
}

void write_sector_n(unsigned int cyl, unsigned int sect, const unsigned char *buffer, int size){
	int i;
	if(cyl<0 || cyl > HDA_MAXCYLINDER){
		fprintf(stderr, "Wrong cylinder index\n");
		exit(EXIT_FAILURE);
	}
	if(sect<0 || sect > HDA_MAXSECTOR){
		fprintf(stderr, "Wrong sector index\n");
		exit(EXIT_FAILURE);
	}
	
	if(size<0 || size > HDA_SECTORSIZE){
		fprintf(stderr, "Wrong size\n");
		exit(EXIT_FAILURE);
	}
	
	gotoSector(cyl, sect);
	
	for (i = 0; i < size ; i ++){
		MASTERBUFFER[i] = buffer[i];
	}
	
	_out(HDA_DATAREGS, 0);
	_out(HDA_DATAREGS+1, 1);
	_out(HDA_CMDREG, CMD_WRITE);
	_sleep(HDA_IRQ);
}

void write_sector(unsigned int cyl, unsigned int sect, const unsigned char *buffer){
	write_sector_n(cyl, sect, buffer, HDA_SECTORSIZE);
}

void format(){
	int i;
	
	for(i = 0 ; i < HDA_MAXCYLINDER; i ++){
		format_sector(i, 0, HDA_MAXSECTOR, 0);
	}
}

void format_sector(unsigned int cylinder, unsigned int sector, unsigned int nsector, unsigned int value){
	int i;
	unsigned char buffer[HDA_SECTORSIZE];
	
	for (i = 0; i < HDA_SECTORSIZE ; i ++){
		buffer[i] = value;
	}
	
	// if((sector+nsector) > HDA_MAXSECTOR){
	// 	fprintf(stderr, "Too much sectors to format\n");
	// 	exit(EXIT_FAILURE);
	// }
	
	for(i = sector ; i < (sector + nsector) ; i++){
		write_sector(cylinder,i,buffer);
	}
}

int load_mbr(){
	read_sector_n(0,0, ((unsigned char*)(&mbr)), sizeof(struct mbr_s));
	if(mbr.mbr_magic == MBR_MAGIC){
		return 0;
	}
	mbr.mbr_nvol = 0;
	mbr.mbr_magic = MBR_MAGIC;
	return 1;
}

void save_mbr(){
	save_super();
	write_sector(0, 0, ((unsigned char*)(&mbr)));
}

unsigned cylinder_of_bloc(int vol, int n_bloc){
	if (vol<0 || vol > mbr.mbr_nvol){
		fprintf(stderr, "Wrong volume number\n");
		exit(EXIT_FAILURE);
	}
	if (n_bloc<0 || vol > mbr.mbr_vols[vol].vd_n_sector){
		fprintf(stderr, "Wrong bloc number\n");
		exit(EXIT_FAILURE);
	}
	
	unsigned fc = mbr.mbr_vols[vol].vd_first_cylinder;
	unsigned fs = mbr.mbr_vols[vol].vd_first_sector;
	return (fc+(fs+n_bloc)/HDA_MAXSECTOR);
}

unsigned sector_of_bloc(int vol, int n_bloc){
	if (vol<0 || vol > mbr.mbr_nvol){
		fprintf(stderr, "Wrong volume number\n");
		exit(EXIT_FAILURE);
	}
	if (n_bloc<0 || vol > mbr.mbr_vols[vol].vd_n_sector){
		fprintf(stderr, "Wrong bloc number\n");
		exit(EXIT_FAILURE);
	}
	unsigned fs = mbr.mbr_vols[vol].vd_first_sector;
	return ((fs+n_bloc)%HDA_MAXSECTOR);	
}

void read_bloc(int vol, int n_bloc, unsigned char* buffer){
	unsigned int cyl = cylinder_of_bloc(vol, n_bloc);
	unsigned int sec = sector_of_bloc(vol, n_bloc);
	read_sector(cyl, sec, buffer);
}

void read_bloc_n(int vol, int n_bloc, unsigned char* buffer, int size){
	unsigned int cyl = cylinder_of_bloc(vol, n_bloc);
	unsigned int sec = sector_of_bloc(vol, n_bloc);
	read_sector_n(cyl, sec, buffer, size);
}

void write_bloc_n(unsigned int vol, unsigned int n_bloc, const unsigned char *buffer, int size){
	unsigned int cyl = cylinder_of_bloc(vol, n_bloc);
	unsigned int sec = sector_of_bloc(vol, n_bloc);
	write_sector_n(cyl, sec, buffer, size);
}

void write_bloc(unsigned int vol, unsigned int n_bloc, const unsigned char *buffer){
	unsigned int cyl = cylinder_of_bloc(vol, n_bloc);
	unsigned int sec = sector_of_bloc(vol, n_bloc);
	write_sector(cyl, sec, buffer);
}

void format_vol(unsigned int vol){
	int n_bloc = mbr.mbr_vols[vol].vd_n_sector;
	unsigned int cyl = mbr.mbr_vols[vol].vd_first_cylinder;
	unsigned int sec = mbr.mbr_vols[vol].vd_first_sector;
	format_sector(cyl, sec, n_bloc, 0);
}

char* printType(enum vd_type_e type){
	switch (type){
		case VT_BASE: 	return "VT_BASE";
		case VT_ANX:	return "VT_ANX";
		case VT_OTH:	return "VT_OTH";
	}
}

void printf_vol(unsigned int i){
	printf("Volume %d:\n",i);
		printf("\tsize: %d\n",mbr.mbr_vols[i].vd_n_sector);
		printf("\t(fc, fs):(%d,%d)\n",mbr.mbr_vols[i].vd_first_cylinder,mbr.mbr_vols[i].vd_first_sector);
		printf("\ttype: %s\n", printType(mbr.mbr_vols[i].vd_val_type));
		printf("\t(lc,ls); (%d,%d)\n\n",cylinder_of_bloc(i, mbr.mbr_vols[i].vd_n_sector), sector_of_bloc(i,mbr.mbr_vols[i].vd_n_sector-1));
}

printf_current_vol(){
	unsigned vol = current_volume;
	printf_vol(vol);
	int rapport = 100*current_super.super_occupation / mbr.mbr_vols[vol].vd_n_sector;
	printf("\ttaux d'occupation: %d %%\n", rapport);
}

void dfs(){
	int i;
	unsigned vol = current_volume;
	for ( i = 0 ; i < mbr.mbr_nvol; i++){
		if ( i == vol) printf_current_vol();
		else printf_vol(i);
	}
}

void listVolumes(){
	int i;
	for ( i = 0 ; i < mbr.mbr_nvol; i++){
		printf_vol(i);
	}
}


/* exit if the size is wrong */
void check_sector_size(){
	int real_value;
	_out(HDA_CMDREG, CMD_DSKINFO);
	real_value = (_in(HDA_DATAREGS+4)<<8)|_in(HDA_DATAREGS+5);
	if(real_value != HDA_SECTORSIZE){
		fprintf(stderr, "Error in sectors size\n\tsize expected: %d\n\tsize used: %d\n", HDA_SECTORSIZE, real_value);
		exit(EXIT_FAILURE);
	}
}

void check_sizes(){
	int value;
	value = sizeof(struct superbloc_s);
	if(value>HDA_SECTORSIZE){
		fprintf(stderr, "Error in sectors size\n\tminimal size needed: %d\n\tsize used: %d\n", HDA_SECTORSIZE, value);
		exit(EXIT_FAILURE);
	}
	
	value = sizeof(struct mbr_s);
	if(value>HDA_SECTORSIZE){
		fprintf(stderr, "Error in sectors size\n\tminimal size needed: %d\n\tsize used: %d\n", HDA_SECTORSIZE, value);
		exit(EXIT_FAILURE);
	}
	
}

void init(){
	int i;
	char* env_current_volume;
	unsigned vol = -1;
	if(init_hardware("etc/hardware.ini") == 0) {
		fprintf(stderr, "Error in hardware initialization\n");
		exit(EXIT_FAILURE);
	}

	check_sector_size();
	
	for(i=0; i<16; i++) IRQVECTOR[i] = empty_it;
	_mask(1);
	
	check_sizes();
	
	load_mbr();
	
	env_current_volume = getenv("CURRENT_VOLUME");
	if(env_current_volume != NULL){
		vol = atoi(env_current_volume);
	}
	current_volume = vol;
	if(env_current_volume != NULL){
		load_super();
	}
	else{
		printf("First use the command \"edit\" to set a volume index\n");
	}
}
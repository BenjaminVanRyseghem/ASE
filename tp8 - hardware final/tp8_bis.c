#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "include/hardware.h"
#include "dump.h"
#include "inode.h"

unsigned ask_for(char* s);

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


void read_inode(unsigned int inumber, struct inode_s *inode){
	read_bloc_n(current_volume, inumber, (unsigned char*)inode, sizeof(struct inode_s));
}
void write_inode(unsigned int inumber, const struct inode_s *inode){
	write_bloc_n(current_volume, inumber, (unsigned char*)inode, sizeof(struct inode_s));
}

void initialize_inode(struct inode_s inode, enum file_type_e type){
	int i;
	inode.in_type = type;
	inode.in_size = 0;
	for (i = 0 ; i < NDIRECT ; i++){
		inode.in_direct[i] = 0;		
	}
	inode.in_indirect = BLOC_NULL;
	inode.in_db_indirect = BLOC_NULL;
}

unsigned int create_inode(enum file_type_e type){
	const struct inode_s inode;
	unsigned inumber = new_bloc();
	if(inumber==BLOC_NULL){
		return BLOC_NULL;
	}
	
	initialize_inode(inode, type);
	write_inode(inumber, &inode);
	return inumber;
}

int delete_inode(unsigned int inumber){
	int i;
	struct inode_s inode;
	read_inode(inumber, &inode);
	for ( i = 0; i < NDIRECT ; i++){
		if( inode.in_direct[i] != 0) free_bloc(inode.in_direct[i]);
	}
	int indirect[N_NBLOC_PER_BLOC];
	read_bloc(current_volume, inode.in_indirect, (unsigned char*)indirect);
	for (i = 0; i < N_NBLOC_PER_BLOC; i++){
		if (indirect[i] != 0){
			free_bloc(indirect[i]);
		}
	}
	free_bloc(inode.in_indirect);
	
	int db_indirect[N_NBLOC_PER_BLOC];
	read_bloc(current_volume, inode.in_db_indirect, (unsigned char*)db_indirect);
	for (i = 0; i < N_NBLOC_PER_BLOC; i++){
		int j;
		int tmp[N_NBLOC_PER_BLOC];
		read_bloc(current_volume, db_indirect[i], (unsigned char*)tmp);
		for (j = 0; j < N_NBLOC_PER_BLOC; j++){
			if (tmp[i] != 0){
				free_bloc(tmp[i]);
			}
		}
		free_bloc(db_indirect[i]);
	}
	free_bloc(inode.in_db_indirect);
	
	free_bloc(inumber);
	return inumber;
}

unsigned initialize_bloc(unsigned adress){
	int new_bloc[N_NBLOC_PER_BLOC];	
	int i;
	
	read_bloc(current_volume, adress, (unsigned char*)new_bloc);
	for (i = 0; i < N_NBLOC_PER_BLOC; i++){
		new_bloc[i] = 0;
	}
	write_bloc(current_volume, adress, (unsigned char*)new_bloc);
	return adress;
}

unsigned int vbloc_of_fbloc(unsigned int inumber, unsigned int fbloc, bool_t do_allocate){
	struct inode_s inode;
	unsigned int bloc_index = fbloc;
	read_inode(inumber, &inode);
//	printf("fbloc: %d | ", bloc_index);


//	printf("do_allocate: %d | ", do_allocate);
	//direct
	if (bloc_index < NDIRECT){
		if (inode.in_direct[bloc_index] == 0)
		{
			if(do_allocate){
			//	printf("allocate inode.in_direct[bloc_index]\n");
				inode.in_direct[bloc_index] = initialize_bloc(new_bloc());
				write_inode(inumber,&inode);
				
			}
			else{
//				printf( "access an unallocated bloc\n");
			 	return BLOC_NULL;
			}
		}
//		printf("direct vbloc: %d\n",inode.in_direct[bloc_index]);
		return inode.in_direct[bloc_index];
	}
	
	bloc_index -= NDIRECT;

	// indirect simple
	if (bloc_index < N_NBLOC_PER_BLOC){
		// if the indirect entry in the inode is not allocated yet
		if (inode.in_indirect == 0)
		{
			if(do_allocate){
//				printf("allocate inode.in_indirect\n");
				inode.in_indirect = initialize_bloc(new_bloc());;
				write_inode(inumber,&inode);
			}
			else { 
//				printf( "access an unallocated bloc\n");
			 	return BLOC_NULL; }
		}
		
		int indirect[N_NBLOC_PER_BLOC];	
		read_bloc(current_volume, inode.in_indirect, (unsigned char*)indirect);
		
		if (indirect[bloc_index] == 0)
		{
			if(do_allocate){
//				printf("allocate indirect[bloc_index]\n");
				indirect[bloc_index] = initialize_bloc(new_bloc());;
				write_bloc(current_volume, inode.in_indirect, (unsigned char*)indirect);
			}
			else { 
//				printf( "access an unallocated bloc\n");
				return BLOC_NULL; }
		}
		
//		printf("indirect vbloc: %d\n",indirect[bloc_index]);
		return indirect[bloc_index];
	}
	
	bloc_index -= N_NBLOC_PER_BLOC;
	
	//indirect double
	if(bloc_index < N_NBLOC_PER_BLOC*N_NBLOC_PER_BLOC){
		if (inode.in_db_indirect == 0)
		{
			if(do_allocate){
//				printf("allocate inode.in_db_indirect\n");
				inode.in_db_indirect = initialize_bloc(new_bloc());;
				write_inode(inumber,&inode);
			}
			else {
//				printf( "access an unallocated bloc\n");
			 	return BLOC_NULL; }
		}
		
		int db_indirect_index = bloc_index / N_NBLOC_PER_BLOC;
		int indirect_index = bloc_index % N_NBLOC_PER_BLOC; 
		
		int db_indirect[N_NBLOC_PER_BLOC];
		read_bloc(current_volume, inode.in_db_indirect, (unsigned char*)db_indirect);
		
		if (db_indirect[db_indirect_index] == 0)
		{
			if(do_allocate){
//				printf("allocate db_indirect[db_indirect_index]\n");
				db_indirect[db_indirect_index] = initialize_bloc(new_bloc());;
				write_bloc(current_volume, inode.in_db_indirect, (unsigned char*)db_indirect);
			}
			else { 
//				printf( "access an unallocated bloc\n");
			 	return BLOC_NULL; }
		}
		
		int indirect[N_NBLOC_PER_BLOC];	
		read_bloc(current_volume, db_indirect[indirect_index], (unsigned char*)indirect);
		
		if (indirect[indirect_index] == 0)
		{
			if(do_allocate){
//				printf("allocate indirect[indirect_index]\n");
				indirect[indirect_index] = initialize_bloc(new_bloc());;
				write_bloc(current_volume, db_indirect[indirect_index], (unsigned char*)indirect);
			}
			else { 
//				printf( "access an unallocated bloc\n");
			 	return BLOC_NULL; }
		}
		
//		printf("double indirect vbloc: %d\n",indirect[indirect_index]);
		return indirect[indirect_index]; 
		
	}
	
	fprintf(stderr,"fbloc is too big.\n\tfbloc provided: %d\n\tfbloc max size: %d",fbloc, NDIRECT+N_NBLOC_PER_BLOC+N_NBLOC_PER_BLOC*N_NBLOC_PER_BLOC);
	return -1;
}


void init_super(unsigned int vol){
	struct superbloc_s super;
	super.super_first_free = 1;
	super.super_occupation = 0;
	super.super_magic = SPR_MAGIC;
	current_super=super;
	struct first_free_bloc_s first;
	first.fb_nbloc = mbr.mbr_vols[vol].vd_n_sector -1;
	first.fb_next = BLOC_NULL;
	write_bloc_n(vol, SUPER_INDEX, (unsigned char*)&super, sizeof(struct superbloc_s));
	write_bloc_n(vol, SUPER_INDEX+1, (unsigned char*)&first, sizeof(struct first_free_bloc_s));
}

void private_mkfs(unsigned vol){
	if(vol < 0 || vol >= mbr.mbr_nvol) {
		printf("Can't create a new file system because the current volume index is wrong\n");
		printf("or there is no partition on the disk.\n"); 
		printf("Have you used edit to choose the partition ? or new to create a new partition ?\n");
		return;
	}
	init_super(vol);
}

void mkfs(){
	unsigned vol = current_volume;
	private_mkfs(vol);
}

void interactive_mkfs(){
	unsigned vol = ask_for("Choose volume where to set a file system\n");
	private_mkfs(vol);
}

int load_super(){
	struct superbloc_s old = current_super;
	unsigned vol = current_volume;
	if(vol<0 || vol>= mbr.mbr_nvol){
		fprintf(stderr,"Can't load because the current volume index is wrong.\n");
		return 1;
	}
	read_bloc_n(vol, SUPER_INDEX,(char*)&current_super, sizeof(struct superbloc_s));
	if (current_super.super_magic != SPR_MAGIC) {
		fprintf(stderr,"Try to load a bloc that is not super\nMaybe you should create a file system (makefs)\n");
		current_super = old;
		return 1;
	}
	return 0;
}

int save_super(){
	unsigned vol = current_volume;
	if(vol<0 || vol>= mbr.mbr_nvol){
		return 1;
	}
	
	if (current_super.super_magic != SPR_MAGIC) {
		fprintf(stderr,"Try to save the bloc that is not SUUUUPPPPPPERRR\n");
		return 1;
	}
	
	write_bloc_n(vol, SUPER_INDEX,(char*)&current_super, sizeof(struct superbloc_s));
	return 0;
}

unsigned ask_for(char* phrase){
	unsigned int result;
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
	read_bloc_n(vol, n_bloc, (unsigned char*)&first, sizeof(struct first_free_bloc_s));
	if(first.fb_nbloc == 1){
		current_super.super_first_free = first.fb_next;
	}
	else {
		current_super.super_first_free++;
		first.fb_nbloc--;
		write_bloc_n(vol, current_super.super_first_free, (unsigned char*)&first, sizeof(struct first_free_bloc_s));
	}
	current_super.super_occupation++;
	return n_bloc;
}

unsigned int interactive_new_bloc(){
	unsigned vol = ask_for("Enter volume index:\n");
	return new_bloc_on(vol);
}

unsigned int new_bloc() {
	unsigned vol = current_volume;
	return new_bloc_on(vol);
}

void free_bloc(unsigned int bloc) {
	unsigned vol = current_volume;
	if(current_super.super_magic != SPR_MAGIC){ 
		fprintf(stderr, "Load a super bloc first\n");
		return;
	}
	
	struct first_free_bloc_s first;
	first.fb_nbloc = 1;
	first.fb_next = current_super.super_first_free;
	current_super.super_first_free = bloc;
	write_bloc_n(vol, bloc, (unsigned char*)&first, sizeof(struct first_free_bloc_s));
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
		fprintf(stderr, "Wrong cylinder index\n\tindex provided: %d\n\trange: 0 - %d\n",cyl, HDA_MAXCYLINDER);
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
		fprintf(stderr, "Wrong cylinder index\n\tindex provided: %d\n\trange: 0 - %d\n",cyl, HDA_MAXCYLINDER);
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
		fprintf(stderr, "Wrong cylinder index\n\tindex provided: %d\n\trange: 0 - %d\n",cyl, HDA_MAXCYLINDER);
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
	return "ERROR";
}

void printf_vol(unsigned int i){
	printf("Volume %d:\n",i);
		printf("\tsize: %d\n",mbr.mbr_vols[i].vd_n_sector);
		printf("\t(fc, fs):(%d,%d)\n",mbr.mbr_vols[i].vd_first_cylinder,mbr.mbr_vols[i].vd_first_sector);
		printf("\ttype: %s\n", printType(mbr.mbr_vols[i].vd_val_type));
		printf("\t(lc,ls); (%d,%d)\n\n",cylinder_of_bloc(i, mbr.mbr_vols[i].vd_n_sector), sector_of_bloc(i,mbr.mbr_vols[i].vd_n_sector-1));
}

void printf_current_vol(){
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
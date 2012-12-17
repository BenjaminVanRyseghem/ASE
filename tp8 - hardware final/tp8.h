#define ENABLE_HDA       1
#define HDA_FILENAME     "vdiskA.bin"
#define HDA_CMDREG       0x3F6
#define HDA_DATAREGS     0x110
#define HDA_IRQ          14
#define HDA_MAXCYLINDER  16
#define HDA_MAXSECTOR    16
#define HDA_SECTORSIZE   512
#define HDA_STPS         2
#define HDA_STPC         1
#define HDA_PON_DELAY    30
#define HDA_POFF_DELAY   30

#define ENABLE_HDB       1
#define HDB_FILENAME     "vdiskB.bin"
#define HDB_CMDREG       0x376
#define HDB_DATAREGS     0x170
#define HDB_IRQ          15
#define HDB_MAXCYLINDER  16
#define HDB_MAXSECTOR    16
#define HDB_SECTORSIZE   512
#define HDB_STPS         2
#define HDB_STPC         3
#define HDB_PON_DELAY    30
#define HDB_POFF_DELAY   30

void read_sector(unsigned int cylinder, unsigned int sector, unsigned char *buffer);
void write_sector(unsigned int cylinder, unsigned int sector, const unsigned char *buffer);
void format_sector(unsigned int cylinder, unsigned int sector, unsigned int nsector,unsigned int value);

int load_mbr();
void save_mbr();
void read_bloc(int vol, int n_bloc, unsigned char* buffer);
void read_bloc_n(int vol, int n_bloc, unsigned char* buffer, int size);
void write_bloc(unsigned int vol, unsigned int n_bloc, const unsigned char *buffer);
void write_bloc_n(unsigned int vol, unsigned int n_bloc, const unsigned char *buffer, int size);
void format_vol(unsigned int vol);

void printf_vol(unsigned int i);

void init_super(unsigned int vol);

unsigned current_volume;

unsigned int new_bloc();
void free_bloc(unsigned int bloc);

void mkfs();
void dfs();

enum file_type_e {FT_STD, FT_DIR, FT_SPEC};
unsigned int create_inode(enum file_type_e type);
int delete_inode(unsigned int inumber);
int load_super();
int save_super();


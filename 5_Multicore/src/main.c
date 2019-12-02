#include <stdbool.h>		// C standard needed for bool
#include <stdint.h>			// C standard for uint8_t, uint16_t, uint32_t etc
#include "system.h"
#include "hal.h"
#include "kprintf.h"
#include "fat.h"
#include "bmp.h"
#define ENTER   10
#define ESC		27
#define PREVIOUS 'q'
#define NEXT 'e'

int calculateFileIndex();
void populateFileIndex(int *fileArray, int length);
void display_root_dir(int *fileArray, int index);
void display_file(uint8_t * filename, uint8_t * ext);
void print_n_chars();


uint8_t buffer[240000]; //107KB (Recall there's 3 pixels per byte + headers)

FATFile file;
FATDirectory dir;

int kernel_main (void) {
//win32 disc imager
    system_init();  //This inits everything (should not be removed)
	int index = 0;
	int numberofRootFiles = calculateFileIndex();
	int fileArray[numberofRootFiles];
	populateFileIndex(fileArray, numberofRootFiles);
	display_root_dir(fileArray, index);
	uint8_t key;
	while (1) {
		 if( hal_io_serial_nonblocking_getc( SerialA, &key ) ){
			 switch(key) {
				 case ENTER:
				 	display_file(dir.files[fileArray[index]].name, dir.files[fileArray[index]].ext);
					 break;
				 case ESC:
				 	display_root_dir(fileArray, index);
					break;
				case PREVIOUS:
					if (index > 0)
					{
						index--;
						display_root_dir(fileArray, index);
					}
					else
					{
						display_root_dir(fileArray, index);
					}
					break;
				case NEXT:
					if (index < numberofRootFiles - 1)
					{
						index++;
						display_root_dir(fileArray, index);
					}
					else
					{
						display_root_dir(fileArray, index);
					}
					break;
				default:
					kprintf("Test: %d", key);
					break;
			 }
        }
	}
	return 0;	
}

void print_n_chars( uint8_t* str, uint32_t len ){
	while( len-- > 0 )
		kprintf( "%c", *str++ );
}

int calculateFileIndex()
{
	fat_read_files_in_dir(&dir, "/");
	int fileIndex = 0;
	for (uint32_t i = 0; i < dir.num_of_files; i++)
	{
		if (dir.files[i].is_read_only && dir.files[i].is_hidden && dir.files[i].is_volume && dir.files[i].is_system)
			continue; //Skip VFAT entries

		if (dir.files[i].is_volume || dir.files[i].is_hidden || dir.files[i].is_system)
			continue; //Skip Volume, Hidden, and System entries

		fileIndex++;
	}
	return fileIndex;
}

void populateFileIndex(int *fileArray, int arrayLength)
{	
	int index = 0;
	for (int i = 0; i < dir.num_of_files; i++)
	{
		if (dir.files[i].is_read_only && dir.files[i].is_hidden && dir.files[i].is_volume && dir.files[i].is_system)
			continue; //Skip VFAT entries

		if (dir.files[i].is_volume || dir.files[i].is_hidden || dir.files[i].is_system)
			continue; //Skip Volume, Hidden, and System entries

		fileArray[index] = i;
		index++;
	}
}
void display_root_dir(int *fileArray, int index)
{
	hal_video_clear(SYSTEM_SCREEN_BACKGROUND_COLOR);
	//display directory
	//
	//   VFAT Long File Names (LFNs) are stored on a FAT file system
	//	 using a trick: adding additional entries into the directory
	//	 before the normal file entry. The additional entries are marked
	//   with the VOLUME LABEL, SYSTEM, HIDDEN, and READ ONLY attributes
	//	 (yielding 0x0F), which is a combination that is not expected
	//	 in the MS-DOS environment, and therefore ignored by MS-DOS programs
	//	 and third-party utilities.
	//
	// See https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#VFAT_long_file_names
	//
	kprintf("\n");
	kprintf("ROOT DIRECTORY:\n\n");
	int fileIndex = 0;

	for (uint32_t i = 0; i < dir.num_of_files; i++)
	{

		if (dir.files[i].is_read_only && dir.files[i].is_hidden && dir.files[i].is_volume && dir.files[i].is_system)
			continue; //Skip VFAT entries

		if (dir.files[i].is_volume || dir.files[i].is_hidden || dir.files[i].is_system)
			continue; //Skip Volume, Hidden, and System entries

		print_n_chars(dir.files[i].name, FAT_MAX_FILENAME_LENGTH);
		kprintf(".");
		print_n_chars(dir.files[i].ext, FAT_MAX_EXT_LENGTH);
		kprintf("%d KB ", dir.files[i].size / 1024);
		// Compare the index, and display an arrow if this is the current index.
		if (fileArray[index] == i)
		{
			kprintf("  %s", "--CURRENT CURSOR--");
		};
		kprintf("\n");
		fileIndex++;
	}
	hal_video_puts("\n\nPress Q and E to navigate between directories", 2, VIDEO_COLOR_RED);
}


void display_file(uint8_t * filename, uint8_t * ext){
	FATFile file;
	hal_video_clear( SYSTEM_SCREEN_BACKGROUND_COLOR );
	if( fat_file_open( &file, filename, ext ) ==  FAT_SUCCESS ){
		//Read to buffer
		fat_file_read( &file, buffer );
		if(ext[0] == 'T' && ext[1] == 'X' && ext[2] == 'T'){
			print_n_chars(filename, FAT_MAX_FILENAME_LENGTH);
			kprintf( "." );
			print_n_chars(ext, FAT_MAX_EXT_LENGTH);
			kprintf( "(%d KB): \n\n", file.size/1024 );
			kprintf( "\n" );
			kprintf( "%s", buffer );
			kprintf( "\n\n" );
		} else if(ext[0] == 'B' && ext[1] == 'M' && ext[2] == 'P'){
			BMP_HEADER header;
			memcpy(&header, buffer, sizeof(BMP_HEADER));
			print_n_chars(filename, FAT_MAX_FILENAME_LENGTH);
			kprintf( "." );
			print_n_chars(ext, FAT_MAX_EXT_LENGTH);
			kprintf("(%d KB): \n", file.size/1024 );
			hal_io_video_draw_image( buffer, header.width_px, header.height_px );
			kprintf( "\n\n" );
		}
	}else{
		hal_video_puts( "\nFILE NOT FOUND\n", 2, VIDEO_COLOR_RED );
	}
}
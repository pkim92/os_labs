#include <stdbool.h>        // C standard needed for bool
#include <stdint.h>         // C standard for uint8_t, uint16_t, uint32_t etc
#include <string.h>
#include "system.h"
#include "hal.h"
#include "kprintf.h"
#include "fat.h"
#include "bmp.h"
#define ENTER   10
#define ESC     27
#define PREVIOUS 'w'
#define PREVIOUS_UPPER 'W'
#define NEXT 's'
#define NEXT_UPPER 'S'
#define NEXT_LINE 'l'
#define NEXT_LINE_UPPER 'L'
#define PREVIOUS_LINE 'm'
#define PREVIOUS_LINE_UPPER 'M'

int calculateFileIndex();
void populateFileIndex(int *fileArray, int length);
void display_root_dir(int *fileArray, int index);
void display_file(uint8_t * filename, uint8_t * ext);
void print_n_chars();


uint8_t buffer[240000]; //107KB (Recall there's 3 pixels per byte + headers)

FATFile file;
FATDirectory dir;

const uint32_t maxCharInLine = SYSTEM_SCREEN_WIDTH/((SYSTEM_SCREEN_TEXT_SIZE_FILE*VIDEO_CHARACTER_WIDTH)+VIDEO_CHARACTER_HORIZONTAL_SPACE);
const uint32_t maxLine = SYSTEM_SCREEN_HEIGHT/((SYSTEM_SCREEN_TEXT_SIZE_FILE*VIDEO_CHARACTER_HEIGHT)+VIDEO_CHARACTER_VERTICAL_SPACE) -10;
//const uint32_t maxLine = 10;
const uint32_t maxChars = maxCharInLine*maxLine;

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
                    if (index > 0) index--;
                    display_root_dir(fileArray, index);
                    break;
                case PREVIOUS_UPPER:
                    if (index > 0) index--;
                    display_root_dir(fileArray, index);
                    break;
                case NEXT:
                    if (index < numberofRootFiles - 1) index++;
                    display_root_dir(fileArray, index);
                    break;
                case NEXT_UPPER:
                    if (index < numberofRootFiles - 1) index++;
                    display_root_dir(fileArray, index);
                    break;
                default:
                    break;
            }
        }
    }
    return 0;
}

void print_n_chars( uint8_t* str, uint32_t len ){
    while( len-- > 0 ){
        kprintf( "%c", *str++ );
    }

}
void print_n_chars_file( uint8_t* str,int numberOfLine){
    int index = 1;
    while( *str ){
        if(*str == '\n'){
            if(numberOfLine == index){
                break;
            }else{
                index++;
                kprintf_file( "%c", *str++ );
            }
        }else{
            kprintf_file( "%c", *str++ );
        }
    }
}

int count_line_chars(uint8_t* buffer,int numberOfLine,int maxLine){
    int removeIndex = numberOfLine % maxLine;
    int block = numberOfLine/maxLine -1 ;
    if(block < 0){
        block = 0;
    }
    removeIndex = block*maxLine + removeIndex;
    int i=0;
    int numberOfC = 0;
    while(*buffer++){
        numberOfC++;
        if(*buffer == '\n' || *buffer == '\r'){
            if(i == removeIndex){
                return numberOfC;
            }else{
                i++;
            }
        }
    }
}
int getFileLineSize(uint8_t* buffer){
    int max = 0;
    while(*buffer++){
        if(*buffer == '\n'){
            max++;
        }
    }
    return max;
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
    //   using a trick: adding additional entries into the directory
    //   before the normal file entry. The additional entries are marked
    //   with the VOLUME LABEL, SYSTEM, HIDDEN, and READ ONLY attributes
    //   (yielding 0x0F), which is a combination that is not expected
    //   in the MS-DOS environment, and therefore ignored by MS-DOS programs
    //   and third-party utilities.
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
    hal_video_puts("\n\nPress W and S to navigate between directories", 2, VIDEO_COLOR_RED);
}


void display_file(uint8_t * filename, uint8_t * ext){
    //FATFile file;
    hal_video_clear( SYSTEM_SCREEN_BACKGROUND_COLOR );

    int numberOfLine = 0;
    
    if( fat_file_open( &file, filename, ext ) ==  FAT_SUCCESS ){
        //Read to buffer
        fat_file_read( &file, buffer );
        if(ext[0] == 'T' && ext[1] == 'X' && ext[2] == 'T'){
            int lineSizeInFile = getFileLineSize(buffer);
            uint8_t key;
            hal_video_puts("\n\nPress L and M to increase and decrease line", 1, VIDEO_COLOR_RED);
            hal_video_puts("\n\nPress ESC to return to dir", 1, VIDEO_COLOR_RED);
            while (1) {
                if (hal_io_serial_nonblocking_getc(SerialA, &key)) {
                    hal_video_clear( SYSTEM_SCREEN_BACKGROUND_COLOR );

                    switch (key) {
                        case NEXT_LINE:             //l
                            numberOfLine++;
                            if(numberOfLine == lineSizeInFile*2+maxLine){
                                numberOfLine--;
                            }else{
                                if(numberOfLine>maxLine){
                                    int test = count_line_chars(buffer,numberOfLine,maxLine-1);
                                    print_n_chars_file(buffer+test,numberOfLine);
                                }else{
                                    print_n_chars_file(buffer,numberOfLine);
                                }
                            }
                            break;
                        case NEXT_LINE_UPPER:
                            numberOfLine++;
                            if(numberOfLine == lineSizeInFile*2+maxLine){
                                numberOfLine--;
                            }else{
                                if(numberOfLine>maxLine){
                                    int test = count_line_chars(buffer,numberOfLine,maxLine-1);
                                    print_n_chars_file(buffer+test,numberOfLine);
                                }else{
                                    print_n_chars_file(buffer,numberOfLine);
                                }
                            }
                            break;
                        case PREVIOUS_LINE:
                            numberOfLine--;//m
                            if(numberOfLine>maxLine){
                                int test2 = count_line_chars(buffer,numberOfLine-1,maxLine-1);
                                print_n_chars_file(buffer+test2,numberOfLine);
                            }else{
                                if (numberOfLine > 1) {
                                    print_n_chars_file(buffer,numberOfLine);
                                    kprintf("\n");
                                } else {
                                    numberOfLine = 1;
                                    kprintf("No more line");
                                }
                            }
                            break;
                        case PREVIOUS_LINE_UPPER:
                            numberOfLine--;//m
                            if(numberOfLine>maxLine){
                                int test2 = count_line_chars(buffer,numberOfLine-1,maxLine-1);
                                print_n_chars_file(buffer+test2,numberOfLine);
                            }else{
                                if (numberOfLine > 1) {
                                    print_n_chars_file(buffer,numberOfLine);
                                    kprintf("\n");
                                } else {
                                    numberOfLine = 1;
                                    kprintf("No more line");
                                }
                            }
                            break;
                        default:
                            return;
                    }
                    hal_video_puts("\n\nPress L and M to increase and decrease line", 1, VIDEO_COLOR_RED);
                    hal_video_puts("\n\nPress ESC to return to dir", 1, VIDEO_COLOR_RED);
                }
            }
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
        hal_video_puts("\n\nPress ESC to return to dir", 1, VIDEO_COLOR_RED);
    }else{
        hal_video_puts( "\nFILE NOT FOUND\n", 2, VIDEO_COLOR_RED );
    }
}
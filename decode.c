#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <png.h>

int open_File(FILE **fp, const char * PNG_NAME, unsigned char * header);
int create_Read(FILE * fp, png_structp * read, png_infop * info, png_infop * end);
int decode(png_bytep * row_pointers, int y, int x);
int read_Message(int y, int x, int * ascii, png_bytep * row_pointers, png_uint_32 width, png_uint_32 height, int index);
void free_Memory(png_uint_32 height, png_bytep * row_pointers, int * ascii_mess, FILE * in);

/**
 * Decoder program that decodes an ASCII message encoded in the pixels of a given PNG image
 */
int main() {

    /**
     * Variables for the message itself
     */
    int index = 0; //doesnt include null char
    int * ascii_mess;  

    unsigned char header[9]; //vars for png stuff
    FILE *in_fp;
    png_uint_32 width, height;
    int bit_depth, color_type; //bitdepth: per channel, so 8 bits per channel (RGBA)
    int interlace_method, compression_method, filter_method;
    png_structp read;
    png_infop info, end;
    png_bytep *row_pointers;   
    const char * PNG_NAME = "altered.png";

    // open file, check for PNG sig
    if (open_File(&in_fp, PNG_NAME, header) == -1) {
        return -1;
    }

    // set up the structs to hold the png when reading
    if (create_Read(in_fp, &read, &info, &end) == -1) {
        return -1;
    }

    //retrieve image info section (NO SEPARATE METHOD: passing in all these params will get ugly)
    png_get_IHDR(read, info, &width, &height, &bit_depth, &color_type, &interlace_method, &compression_method, &filter_method);

    //allocate memory for each row, and read the image
    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(read, info));
    }
    png_set_rows(read, info, row_pointers);
    png_read_image(read, row_pointers);

    // DECODING MESSAGE
    index = 255 * decode(row_pointers, 0, 0);
    index += decode(row_pointers, 0, 1); //get length of the message
    ascii_mess = malloc((index) * sizeof(int));

    if (read_Message(0, 4, ascii_mess, row_pointers, width, height, index)) { //reading first encoding
        printf("1st encryption may have been corrupted. Now trying 2nd encryption......\n"); //try reading the second encoding to see if its fixed.
        read_Message((index + 9) / width, (index + 9) % width, ascii_mess, row_pointers, width, height, index);
    }

    // Print the decoded message
    printf("\n");
    for (int i = 0; i < index; i++) {
        printf("%c", ascii_mess[i]);
    }
  
    // CLEAN-UP SECTION 
    png_destroy_read_struct(&read, &info, &end);
    free_Memory(height, row_pointers, ascii_mess, in_fp);

    printf("\n>>-Program ran successfully-<<\n");

    return 0;
     
} //main

/** Opens the file, reads the first 8 bytes, checks for the PNG signature
 * 
 * @params:
 * fp: pointer to file descriptor for the opened png
 * png_name: name of the png file
 * header: array of 9 bytes that holds the first 8 bytes of the png
 * 
 * @returns: -1 -> function failed or file != PNG, 0 -> functions worked and file == PNG
 */
int open_File(FILE **fp, const char * png_name, unsigned char *header) {
    
    *(fp) = fopen(png_name, "rb");

    // opening file and checking if its a PNG
    if (!(*(fp))) {
        printf("File could not be opened.\n");
        return -1;
    }

    if (fread(header, sizeof(unsigned char), 8, *fp) < 8) {
        printf("Fread failed.\n");
        fclose(*fp);
        return -1;
    }

    if(!(png_sig_cmp(header, 0, 8) == 0)){
        printf("File is not a PNG\n");
        fclose(*fp);
        return -1;
    }

    return 0;
} //open_file

/**
 * Create the read, info, and end info structs to hold PNG data when reading
 * store the READING file pointer in the PNG struct
 * 
 * @params: 
 * fp: file descriptor to opened PNG file
 * png | info | end: structs to hold the PNG data
 * 
 * @returns: -1 -> function calls failed, 0 -> everything worked
 */
int create_Read(FILE * fp, png_structp * read, png_infop * info, png_infop * end) {
    // creating the structs to hold the png data
    *read = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); //png_structp holds all the info for the png for reading
    if (!(*read)) {
        printf("PNG struct could not be created\n");
        fclose(fp);
        return -1;
    }
    
    *info = png_create_info_struct(*read); //png_infop holds all the metadata
    if (!(*info)) {
        png_destroy_read_struct(read, NULL, NULL);
        printf("PNG info struct could not be created\n");
        fclose(fp);
        return -1;   
    }

    *end = png_create_info_struct(*read); //holds metadata after reading the png
    if (!(*end)) {
        png_destroy_read_struct(read, info, NULL);
        printf("PNG end info struct could not be created\n");
        fclose(fp);
        return -1; 
    }

    // Store file pointer in png struct 
    png_init_io(*read, fp); 

    // Indicates that we already checked signature
    png_set_sig_bytes(*read, 8);
    png_read_info(*read, *info);

    return 0;
} //create_Read

/**
 * Algorithm to decode the values from pixel channel data
 * 
 * @params:
 * row_pointers: pointer to rows of pixels
 * y: row of pixel
 * x: column of pixel
 * 
 * @returns: the decoded value
 */
int decode(png_bytep * row_pointers, int y, int x) {

    int mask = 0;
    int bits = 0;
    int num = 0;
    png_byte *row = row_pointers[y];
    png_byte *pixel = &(row[x * 4]);

    //masks out the 2 LSB from each channel (RGBA) of the pixel
    //combines them all to get the encoded value
    for (int i = 0; i < 4; i++) {
        mask = pixel[i] & 3;
        num |= mask;

        if (i != 3) {
           num <<= 2; 
        }
        
        mask = 0;
    }
    return num;
} //decode

/** Decode the encoded message and the extra error checking data, uses the extra data to check for errors
 * 
 * @params:
 * y: row
 * x: column
 * ascii: int array to hold encoded values
 * row_pointers: pointers to rows of the PNG
 * width: width of PNG
 * height: height of PNG
 * index: length of the JUST the message minus null terminator
 * 
 * @returns: 1 -> error in the array of ascii values was found, 0 -> everything matched up
*/
int read_Message(int y, int x, int * ascii, png_bytep * row_pointers, png_uint_32 width, png_uint_32 height, int index) {
    int count = 0;
    int sum = 0;
    int flag = 0;
    int temp_col = x;

    //check for start code
    if (decode(row_pointers, y, x - 1) != 128) {
        printf("Start code was not found, message may be corrupted.\n");
        flag = 1;
    }

    //decode the encoded values for ascii chars
    for (int i = 0; i < index; i++) {
        ascii[i] = decode(row_pointers, y, x);
        x++;
    
        if (x == width) {
            x = 0;
            y++;
        }

        if (y == height) {
            i = index; //cut off loop if picture is full already
        }

        count++;
        sum += ascii[i];
    }

    //check for stop code
    if (decode(row_pointers, y, x) != 128) { 
        printf("Stop code was not found, message may be corrupted.\n");
        flag = 1;
    }

    // check number of decoded chars == expected index
    if (count != index) {
        printf("Number of chars decoded does not match expected index, message may be corrupted.\n");
        flag = 1;
    }

    // check checksum == expected value
    sum %= 256;
    if (sum != decode(row_pointers, y, temp_col - 2)) {
        printf("Total numerical value of message does not match expected value, message may be corrupted.\n");
        flag = 1;
    }

    return flag;
} //read_Message

/**
 * Frees up memory used by ascii message representation, PNG structs, and opened FILES
 * 
 * @params:
 * height: height of PNG
 * row_pointers: pointers to rows of the PNG
 * ascii_mess: array holding the ascii representation of message
 * in: file descriptor for the opened reading PNG
 */
void free_Memory(png_uint_32 height, png_bytep * row_pointers, int * ascii_mess, FILE * in) {

    if (row_pointers != NULL) {
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
    }

    if (ascii_mess != NULL) {
        free(ascii_mess);

    }

    if (in != NULL) {
        fclose(in);
    }

} //free_Memory
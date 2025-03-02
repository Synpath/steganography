#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <png.h>

int * to_ASCII(const char * message, int index);
int open_File(FILE **fp, const char * PNG_NAME, unsigned char * header);
int create_Read(FILE * fp, png_structp * read, png_infop * info, png_infop * end);
void encode(unsigned int chunk, png_byte * pixel);
void mod_Image(png_bytep * row_pointers, png_uint_32 width, png_uint_32 height, int index, int * ascii, int row, int col);
void free_Memory(png_uint_32 height, png_bytep * row_pointers, int * ascii_mess, FILE * in, FILE * out);

/**
 * Encoder program that encodes an ASCII message in the pixels of a given PNG image
 */
int main() {

    /**
     * Variables for the message itself
     */
    int index = 0; //doesnt include null char
    int * ascii_mess;
    
    /** Message options */
    //const char message[] = "Hello World"; 
    //const char message[] = "No cost too great, no Song to Silk. Soon, we shall be drinking the milk\n";
    const char message[] = "On it, everyone you ever heard of...\nThe aggregate of all our joys and sufferings, thousands of confident religions, ideologies and economic doctrines, "
    "every hunter and forager, every hero and coward, every creator and destroyer of civilizations, every king and peasant, every young couple in love, every hopeful child, every "
    "mother and father, every inventor and explorer, every teacher of morals, every corrupt politician, every superstar, every supreme leader, every saint and sinner in the history "
    "of our species, lived there on a mote of dust, suspended in a sunbeam....\nThink of the rivers of blood spilled by all those generals and emperors so that in glory and triumph "
    "they could become the momentary masters of a fraction of a dot.\n";

    /**
     * Variables for PNG handling
     */
    unsigned char header[9];
    FILE *in_fp;
    FILE *out_fp;
    png_uint_32 width, height;
    int bit_depth, color_type; //bitdepth: per channel, so 8 bits per channel (RGBA)
    int interlace_method, compression_method, filter_method;
    png_structp read, write;
    png_infop info, end, out_info;
    png_bytep *row_pointers;   
    const char PNG_NAME[] = "original.png";

    /** Setting up PNG for reading, reading the PNG */
    /**/
    //find length of message
    while (message[index] != '\0') {
        index++;
    }

    ascii_mess = to_ASCII(message, index);
    index += 5; // +5: hold the 2 numbers that make up the actual message length, hold start and stop codeons + checksum

    // open file, check for PNG sig
    if (open_File(&in_fp, PNG_NAME, header) == -1) {
        return -1;
    }

    // set up the structs to hold the png when reading
    if (create_Read(in_fp, &read, &info, &end) == -1) {
        return -1;
    }

    //retrieve image info section
    png_get_IHDR(read, info, &width, &height, &bit_depth, &color_type, &interlace_method, &compression_method, &filter_method);

    //allocate memory for each row, and read the image
    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(read, info));
    }
    png_set_rows(read, info, row_pointers);
    png_read_image(read, row_pointers);

    // MODIFY THE IMAGE
    int row = 0;
    int col = 0;
    mod_Image(row_pointers, width, height, index, ascii_mess, row, col);
    row = index / width;
    col = index % width;
    mod_Image(row_pointers, width, height, index, ascii_mess, row, col); //encode again to provide data redundancy

    //Setting up PNG for Writing the modified image, writing the modified image to files
    //
    out_fp = fopen("altered.png", "wb");
    if (!out_fp) {
        printf("Unable to open output file.\n");
        png_destroy_read_struct(&read, &info, &end);
        free_Memory(height, row_pointers, ascii_mess, in_fp, NULL);
        return -1;
    }

    // initialize write struct
    write = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!write) {
        printf("Unable to create PNG write struct\n");
        png_destroy_read_struct(&read, &info, &end);
        free_Memory(height, row_pointers, ascii_mess, in_fp, out_fp);
        return -1;
    }

    out_info = png_create_info_struct(write);
    if (!out_info) {
        printf("PNG Output info struct couldn't be created.\n");
        png_destroy_read_struct(&read, &info, &end);
        png_destroy_write_struct(&write, NULL);
        free_Memory(height, row_pointers, ascii_mess, in_fp, out_fp);
        return -1;
    }

    png_init_io(write, out_fp);
    png_set_IHDR(write, out_info, width, height, 8, PNG_COLOR_TYPE_RGBA, interlace_method, compression_method, filter_method);
    png_set_rows(write, out_info, row_pointers);
    png_write_png(write, out_info, 0, PNG_COMPRESSION_TYPE_DEFAULT);

    ///* Memory clean up
    // 
    png_destroy_read_struct(&read, &info, &end);
    png_destroy_write_struct(&write, &out_info);
    free_Memory(height, row_pointers, ascii_mess, in_fp, out_fp);

    // statement to tell if program ran with no errors
    printf(">>-Program ran successfully-<<\n");

    return 0;
} //main

/**
 * Convert string to an array of ascii ints
 * 
 * @params:
 * message: string form of the message
 * index: message length minus null terminator
 * 
 * @returns: array of ints that represent the ascii vals of the message + error checking data
 */
int * to_ASCII(const char * message, int index) {

    int * ascii_rep = malloc((index + 5) * sizeof(int)); //ascii representation of the message
    int sum = 0;

    ascii_rep[0] = index / 255; // for message lengths longer than 255 chars
    ascii_rep[1] = index % 255; // adds on the remainder 
    // using this method, maximum of 255^2 + 255 chars in a message, without having to use another pixel
    // to help store message length
    ascii_rep[3] = 128; //start codon

    for (int i = 0; i < index; i++) {
        ascii_rep[i + 4] = message[i];
        sum += message[i];
    }
    ascii_rep[2] = sum % 256; //checksum
    ascii_rep[index + 4] = 128; //stop codon

    return ascii_rep;
} //toASCII

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

    // read the first 8 bytes
    if (fread(header, sizeof(unsigned char), 8, *fp) < 8) {
        printf("Fread failed.\n");
        fclose(*fp);
        return -1;
    }

    // compare 8 bytes to standardized PNG signature
    if(!(png_sig_cmp(header, 0, 8) == 0)){
        printf("File is not a PNG\n");
        fclose(*fp);
        return -1;
    }

    return 0;
}

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

/** Modify PNG pixel data, encode the message into the pixels, one pixel at a time
 * 
 * @params:
 * row_pointers: pointers to the rows of pixels
 * width: width of the PNG
 * height: height of the PNG
 * index: length of message + error checking data
 * ascii: int array holding the ascii vals of the message + extra error checking data
 * y: row to start at
 * x: column to start at
*/
void mod_Image(png_bytep * row_pointers, png_uint_32 width, png_uint_32 height, int index, int * ascii, int y, int x) {

    for (int i = 0; i < index; i++) {
        png_byte *row = row_pointers[y];
        png_byte *pixel = &(row[x * 4]);

        //encode a number regularly
        encode(ascii[i], pixel);

        x++;
    
        if (x == width) {
            x = 0;
            y++;
        }

        if (y == height) {
            i = index; //cut off loop if picture is full already
        }
        
    }
} //mod_Image

/**
 * Algorithm to encode the values into the pixels
 * 
 * @params:
 * chunk: value to be encoded
 * pixel: pixel that value is going to be encoded in
 */
void encode(unsigned int chunk, png_byte * pixel) {
    
    //extract 2 bits at a time, replace the 2 LSBs of the pixel channel (RGBA)
    //one pixel holds 1 value

    unsigned int mask = 3; //7-6
    unsigned int extract = (chunk >> 6) & mask;

    pixel[0] &= ~3; 
    pixel[0] |= extract;

    mask = (1U << (2)) - 1; // 5-4
    extract = (chunk >> 4) & mask;

    pixel[1] &= ~3;
    pixel[1] |= extract;

    mask = (1U << (2)) - 1; // 3-2
    extract = (chunk >> 2) & mask;

    pixel[2] &= ~3;
    pixel[2] |= extract;

    mask = (1U << (2)) - 1; // 1-0
    extract = (chunk >> 0) & mask;
    
    pixel[3] &= ~3;
    pixel[3] |= extract;
} //encode

/**
 * Frees up memory used by ascii message representation, PNG structs, and opened FILES
 * 
 * @params:
 * height: height of PNG
 * row_pointers: pointers to rows of the PNG
 * ascii_mess: array holding the ascii representation of message + extra error checking data
 * in: file descriptor for the opened reading PNG
 * out: file descriptor for the opened writing PNG
 */
void free_Memory(png_uint_32 height, png_bytep * row_pointers, int * ascii_mess, FILE * in, FILE * out) {

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

    if (out != NULL) {
        fclose(out);
    }
} //free_Memory
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <png.h>

int * to_ASCII(const char * message, int index);
int open_File(FILE **fp, const char * PNG_NAME, unsigned char * header);
int create_Read(FILE * fp, png_structp * read, png_infop * info, png_infop * end);
unsigned int count_Ones(int num, const int * pbits);
unsigned int hamming(int num);
unsigned int expand_Num(int ascii);

// given image is 1124 x 1192: 1,339,808 pixels: 42,873,856 bits in total
// 5,359,232 can be used to store bits using LSB way, max 669,904 chars
int main() {

    /**
     * Variables for the message itself
     */
    int index = 0; //doesnt include null char
    int * ascii_mess;
    
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
    unsigned char header[9]; //vars for png stuff
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
    index += 4; // +4: hold the 2 numbers that make up the actual message length, hold start and stop codeons
    
    printf("index after: %d\n", index);

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

    /** Modifying the PNG pixel data 
    
    int y = 0;
    int x = 0;
    int count = 0;
    while ((y < height) && (count < index)) {
        png_byte *row = row_pointers[y];

        while ((x < width) && (count < index)) {
            png_byte *pixel = &(row[x * 4]);

            pixel[0] = ascii_mess[count]; //CHANGE THIS

            x++;
            count++;
        }
        x = 0;
        y++;
    }
    */


    
    //Setting up PNG for Writing the modified image, writing the modified image to files
    //
    out_fp = fopen("altered.png", "wb");
    if (!out_fp) {
        printf("Unable to open output file.\n");
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        free(ascii_mess);
        png_destroy_read_struct(&read, &info, &end);
        fclose(in_fp);
        return -1;
    }

    // initialize write struct
    write = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!write) {
        printf("Unable to create PNG write struct\n");
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        free(ascii_mess);
        png_destroy_read_struct(&read, &info, &end);
        fclose(in_fp);
        fclose(out_fp);
        return -1;
    }

    out_info = png_create_info_struct(write);
    if (!out_info) {
        printf("PNG Output info struct couldn't be created.\n");
        for (int y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        free(ascii_mess);
        png_destroy_read_struct(&read, &info, &end);
        png_destroy_write_struct(&write, NULL);
        fclose(in_fp);
        fclose(out_fp);
        return -1;
    }

    png_init_io(write, out_fp);
    png_set_IHDR(write, out_info, width, height, 8, PNG_COLOR_TYPE_RGBA, interlace_method, compression_method, filter_method);
    png_set_rows(write, out_info, row_pointers);
    png_write_png(write, out_info, 0, PNG_COMPRESSION_TYPE_DEFAULT);

    ///* Memory clean up section 
    //
    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    free(ascii_mess);
    png_destroy_read_struct(&read, &info, &end);
    png_destroy_write_struct(&write, &out_info); // DO I NEED ANOTHER END INFO STRUCT FOR THE WRITE?
    fclose(in_fp);
    fclose(out_fp);
    

    // statement to tell if program ran with no errors
    printf("\nProgram ran successfully.\n");

    return 0;
     
} //main

// convert string to an array of ascii ints
int * to_ASCII(const char * message, int index) {

    int * ascii_rep = malloc((index + 4) * sizeof(int)); //ascii representation of the message

    ascii_rep[0] = index / 255; // for message lengths longer than 255 chars
    ascii_rep[1] = index % 255; // adds on the remainder 
    // using this method, maximum of 255^2 + 255 chars in a message, without having to use another pixel
    // to help store message length
    ascii_rep[2] = 128; //start codon

    for (int i = 0; i < index; i++) {
        ascii_rep[i + 3] = message[i];
    }
    
    ascii_rep[index + 3] = 128; //stop codon

    return ascii_rep;
} //toASCII

/** opens the file, reads the first 8 bytes, checks for the PNG signature
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
}

/**
 * Create the read, info, and end info structs to hold PNG data when reading
 * and store the READING file pointer in the PNG struct
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

/** Modify PNG pixel data, encode the message into the pixels using Hamming Code 
 * 
*/
void mod_Image(png_bytep * row_pointers, png_uint_32 width, png_uint_32 height, int index, int * ascii) {
    int y = 0; //row
    int x = 0; //column
    int count = 0; //keeps track of how many chars have been encoded so far
    int ham_flag = 0;

    for (int i = 3; i < index - 1; i++) {
        ascii[i] = hamming(ascii[i]); //convert ascii encoding to hamming
    }

    //add the first 2 numbers to the first 2 pixels separately, from there, every 3 pixels encodes one char
    // at the end, stop at index - 1, encode the stop code seperately in 1 pixel

    while ((y < height) && (count < index)) {
        png_byte *row = *(row_pointers[y]);

        while ((x < width)) {
            png_byte *pixel = &(row[x * 4]);

            encode(ascii[count], &pixel);

            x++;
            count++;
        }

        x = 0;
        y++;
    }

    //y = 0;
    // x = 0;

    for (int i = 0; i < index; i++) {

        png_byte *row = *(row_pointers[y]);
        png_byte *pixel = &(row[x * 4]);

        if ((i == index - 1) || (index <= 2 && index >= 0)) {
            //encode a number regularly
            encode(ascii[i], &pixel);
        } else {

            for (int j = 0; j < 3; j++) { //runs 3 times to encode 12 bits into 3 pixels

            }

        }

        if (x < width) {
            x++;
        } else {
            x = 0;
        }

        if (y < height) {
            y++;
        } else {
            i = index; //cuts off the loop if message is longer than 1142 * 1192? chars long
        }

    }


} //mod_Image

/** count the number of 1's in the binary representation of the char */
unsigned int count_Ones(int num, const int * pbits) {
    unsigned int count = 0;
    unsigned int position = 0;
    unsigned int mask;

    int i = 0;
    while (pbits[i] != 13) {
        position = pbits[i]; //which position at to check the bit

        mask = 1 << position;

        if ((num & mask) >> position) { //if the extracted bit is 1
            count++;
        }

        i++;
    }
     
    // USING ODD PARITY SCHEME:
    // 0: number of 1's is odd
    // 1: number of 1's is even
    if ((count % 2) == 0) { //if number of 1's is even
        return 1;
    } 

    return 0; //# of 1's was odd
} //count_ones

/** Find Hamming encoding using Odd Parity */
unsigned int hamming(int num) {

    unsigned int num_ones;
    unsigned int expanded;

    //postitions to check for parity bits
    const int R0[] = {2, 4, 6, 8, 10, 13};
    const int R1[] = {2, 5, 6, 9, 10, 13};
    const int R3[] = {4, 5, 6, 11, 13};
    const int R7[] = {8, 9, 10, 11, 13};

    expanded = expand_Num(num); //expands the num to hold the new bits
    
    //SET PARITY BITS
    num_ones = count_Ones(expanded, R0); //returns the val that the parity bit at 0 should be
    expanded = expanded | (num_ones << 0);
    
    num_ones = count_Ones(expanded, R1); //returns the val that the parity bit at 1 should be
    expanded = expanded | (num_ones << 1);

    num_ones = count_Ones(expanded, R3); //returns the val that the parity bit at 3 should be
    expanded = expanded | (num_ones << 3);

    num_ones = count_Ones(expanded, R7); //returns the val that the parity bit at 7 should be
    expanded = expanded | (num_ones << 7);

    return expanded;
} //hamming

/** Expands the number to hold 4 extra 0's to hold parity bits */
unsigned int expand_Num(int ascii) {

    unsigned int mask1 = 0;
    unsigned int mask2 = 0;
    unsigned int num = ascii << 2;
    
    // create space for parity bit at 3
    for (int i = 9; i >= 3; i--) {
        mask1 = mask1 | (1 << i);
    }
    for (int i = 2; i >= 0; i--) {
        mask2 = mask2 | (1 << i);
    }

    mask1 = num & mask1; //holding bits from 3-9 now
    mask2 = num & mask2; //hold bits from 0-2
    mask1 <<= 1; //make space for parity bit
    num = mask1 | mask2;

    //create space for parity bit at 7
    mask1 = 0;
    mask2 = 0;
    for (int i = 10; i >= 7; i--) {
        mask1 = mask1 | (1 << i);
    }

    for (int i = 6; i >= 0; i--) {
        mask2 = mask2 | (1 << i);
    }

    mask1 = num & mask1; //holding bits from 7-10 now
    mask2 = num & mask2; //hold bits from 0-6
    mask1 <<= 1; //make space for parity bit
    num = mask1 | mask2; //reassign num to the expanded version
    
    return num;
} //assign_Bits

void encode(unsigned int chunk, png_byte * pixel) {

    unsigned int mask = (1U << (2)) - 1; //7-6
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
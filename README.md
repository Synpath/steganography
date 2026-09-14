# Steganography
---
This was a take-home project administered for entry into UGA's Small Satellite Research Laboratory. The given project period was about a week or so. 

---
## Technical Specs
---
###### Libraries: Zlib, libpng   
###### Platform: Windows 11    
###### IDE/Code Editor: Visual Studio Code    
###### Compiler: gcc from MinGW    
###### Other: vcpkg    
-------------------
Design Specs:
-------------------
##### Encode:        
1. Find the length of the message minus the null terminating character         
2. Convert the message into an array of ASCII values. 
3. Add on extra data for error checking: length of message, checksum, start codon, message, stop codon
       
       Length of the message is stored as two values.
       - The two values are calculated by finding length / 255 and length % 255. 255 was chosen as to make sure the calculated
       values fit within 8 bits. Using this method, the maximum length can be 255^255 + 255 ASCII characters, however in actuality,
       the maximum length is shorter than that as 5 extra values need to be added on for data redundancy purposes. 

       Checksum is calculated by finding the sum of all the ASCII values of the message characters and modding it by 256.
       - By using (% 256), the checksum should stay as an 8-bit value for most if not all message lengths. This checksum is
       stored as the third value in the array. It will be used by the decoding program to determine if all the received ASCII
       values for the message itself were correct or not. While there may be the possibility that the checksum is bigger than a 8-bit number,
       this method seems to cover a very large range of message value sums.

       Start/Stop codon is 128 and is stored as the fourth value and as the last value in the array.
       - 128 was chosen because ASCII is a 7-bit code, and 128 is 8-bits. If the message does not contain characters from the expanded
       ASCII set, 128 can be used to denote the start and end of the message as the original ASCII set only goes up to 127.

       Then, for each char in the message string, its ASCII value is added into the array after the start codon.

4. The PNG File for reading is then opened using fopen() and is parsed using the libpng library.

       I chose to use libpng as it is widely-used and is simple to use as well.

       Because I only need to open up two PNGS, one for reading and one for writing,
       parse them, modify the pixel data, and write the modified data to a new PNG file,
       I did not need a library with more complicated functionality.

       However, setting up the dependencies for Zlib and libpng for Visual Studio Code
       cost me three days of work, so encode and decode were written in about four days.
       (this will be relevant later)

6. Using the created array that holds the message and the extra data, encode one value into one
       pixel.

        Since each pixel has 4 channels (RGBA), and each value in the array is 8 bits, I chose
       to encode 2 bits into 4 channels.

       The 2 most significant bits of the value replace the 2 least significant bits into the first
       channel (Red), then the value is bit-shifted to the left, then rinse and repeat with the next
       channel (from Red to Green to Blue to Alpha).

       As a result, this lets me use one pixel to hold one value. This increases the maximum possible
       length of the message, which depending on the length, also allows me to encode the message more
       times into the image for redundancy.

       However, it also has a greater effect on the difference in color value. The color difference is
       very slight though, even when zoomed in on the affected pixels all the way.

       In comparison, using something like the Hamming Code would need 3 pixels for 1 value if you want
       to keep the algorithm as simple as possible. The Hamming Code would affect the pixel color less though.
   
       (I tried to implement the Hamming Code and got as far as finding the 12-bit Hamming representation
       for each 8-bit value. I realized I wouldn't have time to implement the appropriate encoding, decoding
       and error correction algorithms in two days, so I abandoned that idea. If you would like to see my work
       on that, I have left a snapshot of my encode program with some of the Hamming encoding functionality I did
       manage to implement)

8. For further data redundancy, I encoded the message a second time into the pixels, starting from the 
       starting from the next pixel after the first stop codon.

       This hinges on the message being short enough to fit twice into the image/the image being large enough,
       so to further improve this, I would calculate the max length that can fit twice into the total number
       of pixels.

10. Open up a file to hold the newly modified PNG, create structs to parse it, write that new PNG 
       to hard drive, then free all dynamically allocated memory

##### Decode:    
1. Open up the altered PNG file, perform the PNG signature check, and create the structs to parse the PNG. 
2. The index of the message is found by decoding the first two pixels of the image.

       The first value decoded is multiplied by 255, and then the second value is added
       onto the product in order to get the final length of just the message itself.

3. The program then decodes the first instance of the message.

       This algorithm is the reverse of the encoding algorithm.

       For a given pixel, I loop through its 4 channels. For each channel, I extract
       the 2 least significant bits, and then place it in another number. Then, the number
       is shifted left to make room for subsequent groups of 2 bits.

       In total, I extract 8 bits, 2 from each pixel channel value.

       At the end, the full 8-bit number is returned to give the ASCII value of the char that was encoded.

    4. Decode the pixel where the start code should be.
   
           If the decoded value != 128, then I set an error flag to true.
           Afterwards, I decode the pixels holding the message chars, summing up the
           values as I go and how many values I have decoded.

           I then check for the stop code. If the stop code != 128, then the error flag is set to true.

           I then check if the number of values decoded matches the index I calculated earlier.

           After that, I check whether the sum of the char ASCII values matches the encoded checksum.
           If any of these checks fail, then the error flag is set to true. 
       
    6. After the first instance of the message is decoded, check the value of the 
       error flag.

           If this flag is true, I output an error message stating that the second instance will be checked.
           I then decode the second instance of the message.

           Afterwards, I print out the decoded values, whether they are correct or not, and free any memory.
           For any of the error checks above, I print out a statement stating which check failed.

###### Data Redundancy:
I decided to include various ways of checking for errors: checking the final length of the message, adding start and stop markers, and a checksum to detect if there are any wrong decoded ASCII character values. While these methods can help determine if there are any errors in the transmitted data, they do not really specify where the error occured. 
For example, the checksum can tell you that there are wrong characters, but if everything else matches up, you cannot determine which characters are wrong based on the checksum itself. Checking the final length and for the start/stop codes can help determine if data was lost during transmission, but again, nothing to determine exactly where chars were lost. 

In addition, I also encoded the message a second time into the pixels. This could help provide data redundancy by increasing the chance that one of the encodings was not corrupted by transmission. 
However to fully improve this chance, it might be better to repeat it multiple times until it fills the whole image. This has drawbacks however as this will change the color values of the entire image, making differences easier to spot. This will also cause the program to run slower. Message length will also determine how many repetitions can be encoded. 

For simplicity, I chose to encode it just two times, but a full algorithm would be something along the lines of finding the total amount of pixels, dividing it by the message length + the extra data, and seeing how many full encodings you can perform. 

I chose to keep the extra error data in for each encoded instance versus just encoding the error data once and repeating just the message. This has the added benefit of increasing the odds that the error check data is transmitted correctly as all the checks are performed for each decoding, but also can reduce the number of times you can repeatedly encode the message.  
    
       

#include <sys/mman.h>  // For mmap(2)
#include <sys/stat.h>  // For stat(2)
#include <fcntl.h>     // O_RDONLY
#include <stdio.h>     // printf!
#include <string.h>	   // str*, mem*
#include <stdlib.h>	   // exit..


/**
Original inspiration by imagine by Jonathan Levin
http://www.newosxbook.com/src.jl?tree=listings&file=6-bonus.c

Thanks to theiphonewiki for img3 structs
https://www.theiphonewiki.com/wiki/IMG3_File_Format
*/


typedef struct {
    uint32_t magic;            // see below
    uint32_t totalLength;      // length of tag including "magic" and these two length values
    uint32_t dataLength;       // length of tag data
    uint8_t data[];
    // uint8_t  data[dataLength];
    // uint8_t  pad[totalLength - dataLength - 12]; // Typically padded to 4 byte multiple
} img3Tag;

typedef struct {
    uint32_t magic;       // ASCII_LE("Img3")
    uint32_t fullSize;    // full size of fw image
    uint32_t sizeNoPack;  // size of fw image without header
    uint32_t sigCheckArea;// although that is just my name for it, this is the
                          // size of the start of the data section (the code) up to
                          // the start of the RSA signature (SHSH section)
    uint32_t ident;       // identifier of image, used when bootrom is parsing images
                          // list to find LLB (illb), LLB parsing it to find iBoot (ibot),
                          // etc.
    // img3Tag  tags[];      // continues until end of file
} img3File;

typedef enum {
	tagVERS = 0x56455253, // iBoot version of the image
	tagSEPO = 0x5345504F, // Security Epoch
	tagSDOM = 0x53444F4D, // Security Domain
	tagPROD = 0x50524F44, // Production Mode
	tagCHIP = 0x43484950, // Chip to be used with. example: 0x8900 for S5L8900.
	tagBORD = 0x424F5244, // Board to be used with
	tagKBAG = 0x4B424147, // Contains the IV and key required to decrypt; encrypted with the GID Key
	tagSHSH = 0x53485348, // RSA encrypted SHA1 hash of the file
	tagCERT = 0x43455254, // Certificate
	tagECID = 0x45434944, // Exclusive Chip ID unique to every device
	tagTYPE = 0x54595045, // Type of image, should contain the same string as the header's ident
	tagDATA = 0x44415441, // Real content of the file
} img3TagType;

union converter {
    char    c[4];
    uint32_t i;
} tmp;


void printTagData(unsigned char *data, uint32_t dataLength)
{
 //   int i;
 //   for (i = 0 ; i < dataLength; i++)
	// {
	//   printf ("%02x", data[i]);
	// }
	printf("%04x\n", *((int*) data));
}


int main(int argc, char **argv)
{
	struct stat stbuf;
	char *filename;
	int rc;
	int fd; 
	int filesize;
	char *mmapped;
	img3File *img3Header;
	img3Tag  *tag;


	if (argc < 2) {
   		fprintf (stderr,"Usage: %s img3_file\n", argv[0]); exit(0);
   	}

  
   filename = argv[argc -1];

   rc = stat(filename, &stbuf);

   if (rc == -1) { perror (filename); exit (1); }

   filesize = stbuf.st_size;

   fd = open (filename, O_RDONLY);
   if (fd < 0) { perror (filename); exit(2);}

   mmapped = mmap(NULL,
             filesize,  // size_t len,
             PROT_READ, // int prot,
             MAP_SHARED | MAP_FILE,  // int flags,
             fd,        // int fd,
             0);        // off_t offset);

   if (!mmapped) { perror ("mmap"); exit(3);}

   img3Header = (img3File *) mmapped;


   // printf("%04X\n", img3Header->magic);
	tmp.i = htonl(img3Header->ident);
	printf("Image Ident: %s\n", tmp.c);
	printf("Full Size of FW Image: %d\n", img3Header->fullSize);
	printf("Size of FW Image w/o Header: %d\n", img3Header->sizeNoPack);
	printf("Sig Check Area: %d\n", img3Header->sigCheckArea);
	printf("\n");


	// mmapped is a pointer currently pointing to the start of the file
	// Need to move the pointer to the start of the tags element in the struct
	tag = (img3Tag *) (mmapped + sizeof(img3File));
	// printf("%08x\n", *(mmapped + sizeof(img3File)));


	while ( (int)tag - (int)mmapped < filesize ) {

		char *tagData;
		tagData = (char*)malloc(tag->dataLength);
		// printf("%d\n", sizeof(tag->totalLength));

		switch (tag->magic) {
			case tagVERS:
				printf("Tag: Version\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("Data Length: %d\n", tag->dataLength);

				printf("%s\n", tag->data+4);
				printf("\n");
				break;

			case tagSEPO:
				printf("Tag: Security Epoch\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("Data Length: %d\n", tag->dataLength);

				printTagData(tag->data, tag->dataLength);
				printf("\n");
				break;

			case tagSDOM:
				printf("Tag: Security Domain\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagPROD:
				printf("Tag: Production Mode\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagCHIP:
				printf("Tag: Chip - Chip to be used with\n");
				printf("Total Length: %d\n", tag->totalLength);

				printTagData(tag->data, tag->dataLength);
				printf("\n");
				break;

			case tagBORD:
				printf("Tag: Board - Board to be used with\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("Data Length: %d\n", tag->dataLength);

				printTagData(tag->data, tag->dataLength);
				printf("\n");
				break;

			case tagKBAG:
				printf("Tag: KBAG - Contains the IV and key required to decrypt; encrypted with the GID Key\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagSHSH:
				printf("Tag: SHSH - RSA encrypted SHA1 hash of the file\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagCERT:
				printf("Tag: Certificate\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagECID:
				printf("Tag: ECID - Exclusive Chip ID unique to every device\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("\n");
				break;

			case tagTYPE:
				printf("Tag: Type - Type of image, should contain the same string as the header's ident\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("Data Length: %d\n", tag->dataLength);

				char type[5];
				for (int i = 0; i < 4; i++)
				{
				   type[i] = * (((char *)&(tag->data)) + 3-i);
				}
				printf ("%s\n", type);

				// printTagData(tag->data, tag->dataLength);
				// memcpy(tagData, tag->data, tag->dataLength);
				// printf("%s\n", tagData);

				printf("\n");
				break;

			case tagDATA:
				printf("Tag: Data - Real content of the file\n");
				printf("Total Length: %d\n", tag->totalLength);
				printf("Data Length: %d\n", tag->dataLength);
				
				memcpy(tagData, tag->data, tag->dataLength);

				char buf[256];
				snprintf(buf, sizeof(buf), "%s%s", filename, ".DATA");
				FILE *outFile = fopen(buf,"wb");

				if (outFile){
				    fwrite(tagData, tag->dataLength, 1, outFile);
				    printf("Wrote .DATA file\n");
				}
				else{
				    printf("Error opening .DATA file\n");
				}

				fclose(outFile);

				printf("\n");

				break;
		}

		free(tagData);
		tag = (char*)tag + tag->totalLength;
	}
	

  return 0;  

}



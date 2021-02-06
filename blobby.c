// blobby.c
// blob file archiver
// COMP1521 20T3 Assignment 2
// Written by Bowen Sze

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// the first byte of every blobette has this value
#define BLOBETTE_MAGIC_NUMBER          0x42

// number of bytes in fixed-length blobette fields
#define BLOBETTE_MAGIC_NUMBER_BYTES    1
#define BLOBETTE_MODE_LENGTH_BYTES     3
#define BLOBETTE_PATHNAME_LENGTH_BYTES 2
#define BLOBETTE_CONTENT_LENGTH_BYTES  6
#define BLOBETTE_HASH_BYTES            1

// maximum number of bytes in variable-length blobette fields
#define BLOBETTE_MAX_PATHNAME_LENGTH   65535
#define BLOBETTE_MAX_CONTENT_LENGTH    281474976710655


// ADD YOUR #defines HERE


typedef enum action {
    a_invalid,
    a_list,
    a_extract,
    a_create
} action_t;


void usage(char *myname);
action_t process_arguments(int argc, char *argv[], char **blob_pathname,
                           char ***pathnames, int *compress_blob);

void list_blob(char *blob_pathname);
void extract_blob(char *blob_pathname);
void create_blob(char *blob_pathname, char *pathnames[], int compress_blob);

uint8_t blobby_hash(uint8_t hash, uint8_t byte);


// ADD YOUR FUNCTION PROTOTYPES HERE
void create_file_in_blob(char *pathname, FILE *output_stream);
void create_dir_in_blob(char *pathname, FILE *output_stream);

// YOU SHOULD NOT NEED TO CHANGE main, usage or process_arguments

int main(int argc, char *argv[]) {
    char *blob_pathname = NULL;
    char **pathnames = NULL;
    int compress_blob = 0;
    action_t action = process_arguments(argc, argv, &blob_pathname, &pathnames,
                                        &compress_blob);

    switch (action) {
    case a_list:
        list_blob(blob_pathname);
        break;

    case a_extract:
        extract_blob(blob_pathname);
        break;

    case a_create:
        create_blob(blob_pathname, pathnames, compress_blob);
        break;

    default:
        usage(argv[0]);
    }

    return 0;
}

// print a usage message and exit

void usage(char *myname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t%s -l <blob-file>\n", myname);
    fprintf(stderr, "\t%s -x <blob-file>\n", myname);
    fprintf(stderr, "\t%s [-z] -c <blob-file> pathnames [...]\n", myname);
    exit(1);
}

// process command-line arguments
// check we have a valid set of arguments
// and return appropriate action
// **blob_pathname set to pathname for blobfile
// ***pathname set to a list of pathnames for the create action
// *compress_blob set to an integer for create action

action_t process_arguments(int argc, char *argv[], char **blob_pathname,
                           char ***pathnames, int *compress_blob) {
    extern char *optarg;
    extern int optind, optopt;
    int create_blob_flag = 0;
    int extract_blob_flag = 0;
    int list_blob_flag = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":l:c:x:z")) != -1) {
        switch (opt) {
        case 'c':
            create_blob_flag++;
            *blob_pathname = optarg;
            break;

        case 'x':
            extract_blob_flag++;
            *blob_pathname = optarg;
            break;

        case 'l':
            list_blob_flag++;
            *blob_pathname = optarg;
            break;

        case 'z':
            (*compress_blob)++;
            break;

        default:
            return a_invalid;
        }
    }

    if (create_blob_flag + extract_blob_flag + list_blob_flag != 1) {
        return a_invalid;
    }

    if (list_blob_flag && argv[optind] == NULL) {
        return a_list;
    } else if (extract_blob_flag && argv[optind] == NULL) {
        return a_extract;
    } else if (create_blob_flag && argv[optind] != NULL) {
        *pathnames = &argv[optind];
        return a_create;
    }

    return a_invalid;
}

// list the contents of blob_pathname

void list_blob(char *blob_pathname) {
    FILE *input_stream = fopen(blob_pathname, "rb");

    int ch = fgetc(input_stream);

    while (ch != EOF) {
        //obtain magic number
        if (ch != BLOBETTE_MAGIC_NUMBER) {
            fprintf(stderr,"ERROR: Magic byte of blobette incorrect\n");
            exit(-1);
        }  
        //obtain mode
        uint64_t mode = 0;
        for (int i=1;i<=3;i++) {
            mode = mode << 8;
            int c = fgetc(input_stream);
            if (c != EOF) 
                mode = mode | c;
        }  
        //obtain pathname length
        uint32_t name_len = 0;
        for (int i=1;i<=2;i++) {
            name_len = name_len << 8;
            int c = fgetc(input_stream);
            if (c != EOF)
                name_len = name_len | c;
        }
        //obtain content length
        uint64_t cont_len = 0;
        for (int i=1;i<=6;i++) {
            cont_len = cont_len << 8;
            int c = fgetc(input_stream);
            if (c != EOF)
                cont_len = cont_len | c;
        }
        //obtain pathname
        char pathname[name_len+1];
        /*for (int i=0;i<name_len;i++) {
            pathname[i] = fgetc(input_stream);
        }*/
        fread(pathname, 1, name_len, input_stream);
        pathname[name_len] = '\0';

        //print 
        printf("%06lo %5lu %s\n", mode, cont_len, pathname);
        fseek(input_stream, cont_len+1, SEEK_CUR);
        ch = fgetc(input_stream); 
        // HINT: you'll need a printf like:
        // printf("%06lo %5lu %s\n", mode, content_length, pathname);
    }
    fclose(input_stream);
}


// extract the contents of blob_pathname

void extract_blob(char *blob_pathname) {

    // REPLACE WITH YOUR CODE FOR -x
    FILE *input_stream = fopen(blob_pathname, "rb");

    int ch = fgetc(input_stream);

    while (ch != EOF) {
        int h = 0;
        h = blobby_hash(h,ch);

        if (ch != BLOBETTE_MAGIC_NUMBER) {
            fprintf(stderr,"ERROR: Magic byte of blobette incorrect\n");
            exit(-1);
        }  
        //obtain mode
        uint64_t mode = 0;
        for (int i=1;i<=3;i++) {
            mode = mode << 8;
            int c = fgetc(input_stream);
            h = blobby_hash(h,c);
            if (c != EOF) 
                mode = mode | c;
        }  
        //obtain pathname length
        uint32_t name_len = 0;
        for (int i=1;i<=2;i++) {
            name_len = name_len << 8;
            int c = fgetc(input_stream);
            h = blobby_hash(h,c);
            if (c != EOF)
                name_len = name_len | c;
        }
        //obtain content length
        uint64_t cont_len = 0;
        for (int i=1;i<=6;i++) {
            cont_len = cont_len << 8;
            int c = fgetc(input_stream);
            h = blobby_hash(h,c);
            if (c != EOF)
                cont_len = cont_len | c;
        }
        //obtain pathname
        char pathname[name_len+1];
        fread(pathname, 1, name_len, input_stream);
        pathname[name_len] = '\0';
        for (int i=0; i<name_len; i++) {
            h = blobby_hash(h, pathname[i]);
        }

        char bytes[cont_len > 0 ? cont_len : 1];
        if (cont_len > 0) {
            fread(bytes, 1, cont_len, input_stream);
            for (int i=0; i<cont_len; i++) {
                h = blobby_hash(h, bytes[i]);
            }
        }

        int hash = fgetc(input_stream);
        if (h != hash) {
            fprintf( stderr, "ERROR: blob hash incorrect\n");
        } else {
            if ((mode & S_IFDIR) == S_IFDIR) {
                // make the directory
                mkdir(pathname, mode);
                printf("Creating directory: %s\n", pathname); 
            } else {
                //copy file into output
                FILE *output_stream = fopen(pathname, "wb");
                fwrite(bytes, 1, cont_len, output_stream);
                fclose(output_stream);
                //change mode
                if (chmod(pathname, mode) != 0) {
                    perror(pathname); // prints why the chmod failed
                    exit(-1);
                }
                printf("Extracting: %s\n", pathname); 
            }
        }
                      
        
        ch = fgetc(input_stream); 
    }
    fclose(input_stream);
}
        
void create_dir_in_blob(char *pathname, FILE *output_stream) {
    int h = 0;
    printf("Adding: %s\n", pathname);
    fputc(BLOBETTE_MAGIC_NUMBER, output_stream);
    h = blobby_hash(h, BLOBETTE_MAGIC_NUMBER);
    struct stat s;
    if (stat(pathname, &s) != 0) {
        perror(pathname);
        exit(1);
    }
    // write mode.
    for (int i=1; i<=3; i++) {
        uint8_t stmode = (s.st_mode >> ((3-i) * 8)) & 255;
        fputc(stmode, output_stream);
        h = blobby_hash(h, stmode);
    }
    // write pathname length - 2 bytes.
    int length = strlen(pathname);
    for (int i=1; i<=2; i++) {
        uint8_t namelen = (length >> ((2-i) * 8)) & 255;
        fputc(namelen, output_stream);
        h = blobby_hash(h, namelen);
    }
    // write content length - 6 bytes.
    for (int i=1; i<=6; i++) {
        fputc(0, output_stream);
        h = blobby_hash(h, 0);
    }

    // write pathname
    fwrite(pathname, 1, length, output_stream);
    for (int i=0; i<length; i++) {
        h = blobby_hash(h, pathname[i]);
    }
    fputc(h, output_stream);
}

void create_file_in_blob(char *pathname, FILE *output_stream) {
    int h = 0;
    printf("Adding: %s\n", pathname);
    fputc(BLOBETTE_MAGIC_NUMBER, output_stream);
    h = blobby_hash(h, BLOBETTE_MAGIC_NUMBER);
    struct stat s;
    if (stat(pathname, &s) != 0) {
        perror(pathname);
        exit(1);
    }
    // write mode.
    for (int i=1; i<=3; i++) {
        uint8_t stmode = (s.st_mode >> ((3-i) * 8)) & 255;
        fputc(stmode, output_stream);
        h = blobby_hash(h, stmode);
    }
    // write pathname length - 2 bytes.
    int length = strlen(pathname);
    for (int i=1; i<=2; i++) {
        uint8_t namelen = (length >> ((2-i) * 8)) & 255;
        fputc(namelen, output_stream);
        h = blobby_hash(h, namelen);
    }
    // write content length - 6 bytes.
    for (int i=1; i<=6; i++) {
        uint8_t size = (s.st_size >> ((6-i) * 8)) & 255;
        fputc(size, output_stream);
        h = blobby_hash(h, size);
    }

    // write pathname
    fwrite(pathname, 1, length, output_stream);
    for (int i=0; i<length; i++) {
        h = blobby_hash(h, pathname[i]);
    }
    // write out contents
    if (s.st_size > 0) {
        FILE *input_stream = fopen(pathname, "rb");
        char read[s.st_size];
        fread(read, 1, s.st_size, input_stream);
        fclose(input_stream);
        fwrite(read, 1, s.st_size, output_stream);
        for (int i=0; i<s.st_size; i++) {
            h = blobby_hash(h, read[i]);
        }
    }
    fputc(h, output_stream);
}

// create blob_pathname from NULL-terminated array pathnames
// compress with xz if compress_blob non-zero (subset 4)

void create_blob(char *blob_pathname, char *pathnames[], int compress_blob) {
    // REPLACE WITH YOUR CODE FOR -c

    FILE *output_stream = fopen(blob_pathname, "wb");
    // take out pathnames 1 by 1 and write info to blob_pathname.
    int x = 0;
    for (x=0; pathnames[x]!= NULL; x++);
    for (int p=0; p<x; p++) {
        char *position_ptr = strchr(pathnames[p], '/');
        if (position_ptr == NULL) {
            //for (int p = 0; pathnames[p]; p++) {
            create_file_in_blob(pathnames[p], output_stream);
            //}
        } else {
            char paths[strlen(pathnames[p])];
            int count = 0;
            // find number of /.
            for (int i=0; i<strlen(pathnames[p]); i++) {
                if (pathnames[p][i] == '/') {
                    count++;
                }
            }
            // add each name into the array.
            int i = 0;
            while (count > 1) {
                for (i=0; pathnames[p][i] != '/'; i++) {
                    paths[i] = pathnames[p][i];
                    paths[i+1] = '\0';
                    count--;
                    create_dir_in_blob(paths, output_stream);
                    //arbitrary way to remove null terminator:
                    paths[i+1] = pathnames[p][i+1];
                }
            }
            // add the final part into the char array.
            while (pathnames[p][i] != '\0') {
                paths[i] = pathnames[p][i];
            }
            //paths[i+1] = '\0';

            struct stat s;
            if (stat(paths, &s) != 0) {
                perror(paths);
                exit(1);
            }
            uint32_t mode = s.st_mode;
            if ((mode & S_IFDIR) == S_IFDIR) {
                struct dirent *dp;
                DIR *dir = opendir(paths); 
                dp = readdir(dir);
                while (dp != NULL) {
                    char filename[256];
                    strcpy(filename, dp->d_name);
                    struct stat s1;
                    if (stat(filename, &s1) != 0) {
                        perror(filename);
                        exit(1);
                    }
                    uint32_t modes = s.st_mode;
                    if ((modes & S_IFDIR) == S_IFDIR) {
                        create_dir_in_blob(filename, output_stream);
                    } else {
                        create_file_in_blob(filename, output_stream);
                    }
                    dp = readdir(dir);
                }
                closedir(dir);
            } else {
                create_file_in_blob(paths, output_stream);
            }
        }
    }
    fclose(output_stream);
    //printf("create_blob called to create %s blob '%s' containing:\n",
    //       compress_blob ? "compressed" : "non-compressed", blob_pathname);
}


// ADD YOUR FUNCTIONS HERE


// YOU SHOULD NOT CHANGE CODE BELOW HERE

// Lookup table for a simple Pearson hash
// https://en.wikipedia.org/wiki/Pearson_hashing
// This table contains an arbitrary permutation of integers 0..255

const uint8_t blobby_hash_table[256] = {
    241, 18,  181, 164, 92,  237, 100, 216, 183, 107, 2,   12,  43,  246, 90,
    143, 251, 49,  228, 134, 215, 20,  193, 172, 140, 227, 148, 118, 57,  72,
    119, 174, 78,  14,  97,  3,   208, 252, 11,  195, 31,  28,  121, 206, 149,
    23,  83,  154, 223, 109, 89,  10,  178, 243, 42,  194, 221, 131, 212, 94,
    205, 240, 161, 7,   62,  214, 222, 219, 1,   84,  95,  58,  103, 60,  33,
    111, 188, 218, 186, 166, 146, 189, 201, 155, 68,  145, 44,  163, 69,  196,
    115, 231, 61,  157, 165, 213, 139, 112, 173, 191, 142, 88,  106, 250, 8,
    127, 26,  126, 0,   96,  52,  182, 113, 38,  242, 48,  204, 160, 15,  54,
    158, 192, 81,  125, 245, 239, 101, 17,  136, 110, 24,  53,  132, 117, 102,
    153, 226, 4,   203, 199, 16,  249, 211, 167, 55,  255, 254, 116, 122, 13,
    236, 93,  144, 86,  59,  76,  150, 162, 207, 77,  176, 32,  124, 171, 29,
    45,  30,  67,  184, 51,  22,  105, 170, 253, 180, 187, 130, 156, 98,  159,
    220, 40,  133, 135, 114, 147, 75,  73,  210, 21,  129, 39,  138, 91,  41,
    235, 47,  185, 9,   82,  64,  87,  244, 50,  74,  233, 175, 247, 120, 6,
    169, 85,  66,  104, 80,  71,  230, 152, 225, 34,  248, 198, 63,  168, 179,
    141, 137, 5,   19,  79,  232, 128, 202, 46,  70,  37,  209, 217, 123, 27,
    177, 25,  56,  65,  229, 36,  197, 234, 108, 35,  151, 238, 200, 224, 99,
    190
};

// Given the current hash value and a byte
// blobby_hash returns the new hash value
//
// Call repeatedly to hash a sequence of bytes, e.g.:
// uint8_t hash = 0;
// hash = blobby_hash(hash, byte0);
// hash = blobby_hash(hash, byte1);
// hash = blobby_hash(hash, byte2);
// ...

uint8_t blobby_hash(uint8_t hash, uint8_t byte) {
    return blobby_hash_table[hash ^ byte];
}

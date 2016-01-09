/**
 * qzip, a gzip-like command-line tool for compressing and decompressing
 * quicklz files (.qz).
 *
 * stream_compress/stream_decompress are borrowed from the quicklz demos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "quicklz.h"

#if QLZ_STREAMING_BUFFER == 0
    #error Define QLZ_STREAMING_BUFFER to a non-zero value for this application
#endif

// 1 MB Buffer
#define MAX_BUF_SIZE 1024*1024
#define BUF_BUFFER 400

#define bool int
#define true 1
#define false 0

static char doc[] =
       "qzip/qunzip - A gzip-like program which uses quicklz compression.\n"
       "version 0.2\n"
       "\n"
       "Examples:\n"
       "  cat <file> | qzip > outfile.qz \n"
       "  qunzip file.qz\n"
       "  qzip file\n"
       "  qcat file.qz\n";

static char* progname;

int stream_compress(FILE *ifile, FILE *ofile)
{
    char *file_data, *compressed;
    size_t d, c, fd_size, compressed_size;
    qlz_state_compress *state_compress = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));

    fd_size = MAX_BUF_SIZE;
    file_data = (char*) malloc(fd_size);

    // allocate MAX_BUF_SIZE + BUF_BUFFER bytes for the destination buffer
    compressed_size = MAX_BUF_SIZE + BUF_BUFFER;
    compressed = (char*) malloc(compressed_size);

    // allocate and initially zero out the states. After this, make sure it is
    // preserved across calls and never modified manually
    memset(state_compress, 0, sizeof(qlz_state_compress));

    // compress the file using MAX_BUF_SIZE packets.
    while((d = fread(file_data, 1, MAX_BUF_SIZE, ifile)) != 0)
    {
        c = qlz_compress(file_data, compressed, d, state_compress);

        // the buffer "compressed" now contains c bytes which we could have sent directly to a
        // decompressing site for decompression
        fwrite(compressed, c, 1, ofile);
    }

    free(state_compress);
    free(compressed);
    free(file_data);
    return 0;
}

int stream_decompress(FILE *ifile, FILE *ofile)
{
    char *file_data, *decompressed;
    size_t d, c, dc, fd_size, d_size;
    qlz_state_decompress *state_decompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));

    // a compressed packet can be at most MAX_BUF_SIZE + BUF_BUFFER bytes if it
    // was compressed with this program.
    fd_size = MAX_BUF_SIZE + BUF_BUFFER;
    file_data = (char*) malloc(fd_size);

    // allocate decompression buffer
    d_size = fd_size - BUF_BUFFER;
    decompressed = (char*) malloc(d_size);

    // allocate and initially zero out the scratch buffer. After this, make sure it is
    // preserved across calls and never modified manually
    memset(state_decompress, 0, sizeof(qlz_state_decompress));

    // read 9-byte header to find the size of the entire compressed packet, and
    // then read remaining packet
    while((c = fread(file_data, 1, 9, ifile)) != 0)
    {
        // Do we need a bigger decompressed buffer? If the file was compressed
        // with segments larger than the default in this program.
        dc = qlz_size_decompressed(file_data);
        if (dc > (fd_size - BUF_BUFFER)) {
            free(file_data);
            fd_size = dc + BUF_BUFFER;
            file_data = (char*)malloc(fd_size);
        }

        // Do we need a bigger compressed buffer?
        c = qlz_size_compressed(file_data);
        if (c > d_size) {
            free (decompressed);
            d_size = c;
            decompressed = (char*)malloc(d_size);
        }

        fread(file_data + 9, 1, c - 9, ifile);
        d = qlz_decompress(file_data, decompressed, state_decompress);
        fwrite(decompressed, d, 1, ofile);
    }

    free(decompressed);
    free(state_decompress);
    free(file_data);
    return 0;
}

void usage(int error) {
  fprintf(error ? stderr : stdout, doc);
  exit(error);
}

void abort_if_exists(char *fn) {
    FILE *f;

    // Check if the file already exists. If it does, abort.
    if ((f = fopen(fn, "rb")) != NULL) {
        fprintf(stderr,
                "%s: File already exists. Will not overwrite '%s'\n",
                progname, fn);
        fclose(f);
        exit(1);
    }
}

void move_to_final(char *tmp, char *dst) {
    abort_if_exists(dst);
    if (rename(tmp, dst) < 0) {
        fprintf(stderr, "%s:Unable to rename output file '%s'",
                progname, dst);
        perror(progname);
        exit(2);
    }
}

int main(int argc, char* argv[])
{
    bool do_compress = false;
    bool to_stdout   = false;
    bool use_literal = false;
    char fn_buffer[1024] = {'\0'};
    char tmp_fn_buffer[1024] = {'\0'};
    FILE* ifile;
    FILE* ofile;
    char *progname_iter;
    int file_index;

    int c;
    char *cvalue = NULL;

    progname = strtok(argv[0], "/");
    while ((progname_iter = strtok(NULL, "/")) != NULL) {
        progname = progname_iter;
    }

    if (strcmp(progname, "qzip") == 0) {
        do_compress = true;
    }
    else if (strcmp(progname, "qunzip") == 0) {
        do_compress = false;
    }
    else if (strcmp(progname, "qcat") == 0) {
        do_compress = false;
        to_stdout = true;
    }
    else {
        fprintf(stderr, "Unknown executable invocation: '%s'\n", progname);
        usage(1);
    }

    while ((c = getopt (argc, argv, "dh?")) != -1) {
      switch (c) {
      case 'd':
        do_compress = false;
        break;
      case '-':
        use_literal = true;
      case 'h':
      case 'v':
        usage(0);
        break;
      case '?':
      default:
        usage(1);
      }
    }

    if (optind > 0 &&  strcmp(argv[optind-1], "--") == 0) {
      use_literal = true;
    }


    if (optind < argc && strcmp(argv[optind], "-") == 0 && !use_literal) {
      to_stdout = true;
    }

    for (file_index = optind; file_index <= argc; file_index++)
    {
        if (argc > optind && file_index == argc) {
            // We ensure that we go through the loop at least once.
            // If there are files listed as argv, then ignore the
            // last iteration.
            break;
        }

        if (do_compress) {
            // Compress
            if (argc > 1 && !to_stdout) {
                sprintf(fn_buffer, "%s.qz", argv[file_index]);
                sprintf(tmp_fn_buffer, "%s.qz.%d", argv[file_index], getpid());

                abort_if_exists(fn_buffer);
                abort_if_exists(tmp_fn_buffer);

                ifile = fopen(argv[file_index], "rb");
                if (!ifile) {
                    fprintf(stderr,
                            "Unable to open input file '%s':",
                            argv[file_index]);
                    perror(NULL);
                    exit(2);
                }
                ofile = fopen(tmp_fn_buffer, "wb");
            }
            else {
                strcpy(fn_buffer, "stdout");
                ifile = stdin;
                ofile = stdout;
            }
        }
        else {
            // Decompress
            if (argc > 1 && !to_stdout) {
                bool err = false;
                if (strlen(argv[file_index]) < 4) err = true;
                if (!err) {
                    strcpy(tmp_fn_buffer, argv[file_index] + strlen(argv[file_index]) - 3);
                    if (strcmp(tmp_fn_buffer, ".qz") != 0) {
                        err = true;
                    }
                }
                if (err) {
                    fprintf(stderr, "%s: File does not end in '.qz': '%s'\n",
                            progname, argv[file_index]);
                    exit(1);
                }

                if (! to_stdout) {
                    strncpy(fn_buffer, argv[file_index], strlen(argv[file_index]) - 3);
                    sprintf(tmp_fn_buffer, "%s.%d", fn_buffer, getpid());
                    abort_if_exists(fn_buffer);
                }

                ifile = fopen(argv[file_index], "rb");
                if (!ifile) {
                    fprintf(stderr,
                            "Unable to open compressed file '%s':",
                            argv[file_index]);
                    perror(NULL);
                    exit(2);
                }

                if (to_stdout) {
                    strcpy(fn_buffer, "stdin");
                    ofile = stdout;
                }
                else {
                    ofile = fopen(tmp_fn_buffer, "wb");
                }
            }
            else {
                strcpy(fn_buffer, "stdin");
                ifile = stdin;
                ofile = stdout;
            }
        }

        if (!ofile) {
            perror("Unable to open output file");
            exit(2);
        }

        if (do_compress) {
            stream_compress(ifile, ofile);
        }
        else {
            stream_decompress(ifile, ofile);
        }
        fclose(ifile);

        if (argc > 1 && !to_stdout) {
            fclose(ofile);
            move_to_final(tmp_fn_buffer, fn_buffer);
            if (unlink(argv[file_index]) < 0) {
                fprintf(stderr, "%s: Unable to unlink original file '%s'\n",
                        progname, argv[1]);
                perror(progname);
                exit(3);
            }
        }
    }
    exit(0);
}


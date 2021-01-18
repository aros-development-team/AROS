/*
 * unadf 1.1
 *
 * tested under Linux and Windows
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "adflib.h"

#ifdef WIN32
typedef uint32_t mode_t;
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# include <sys/utime.h>
# define DIRSEP '\\'
#else
# include <sys/time.h>
# include <unistd.h>
# define DIRSEP '/'
#endif

#define UNADF_VERSION "1.1"
#define EXTRACT_BUFFER_SIZE 8192

/* command-line arguments */
BOOL list_mode = FALSE, list_all = FALSE, use_dircache = FALSE,
     show_sectors = FALSE, show_comments = FALSE, pipe_mode = FALSE;
char *adf_file = NULL, *extract_dir = NULL;
int vol_number = 0;
struct List *file_list = NULL;

/* prototypes */
void parse_args(int argc, char *argv[]);
void help();
void print_device(struct Device *dev);
void print_volume(struct Volume *vol);
void print_tree(struct List *node, char *path);
void print_entry(struct Entry *e, char *path);
void extract_tree(struct Volume *vol, struct List *node, char *path);
void extract_filepath(struct Volume *vol, char *filepath);
void extract_file(struct Volume *vol, char *filename, char *out, mode_t perms);
char *output_name(char *path, char *name);
char *join_path(char *path, char *name);
void set_file_date(char *out, struct Entry *e);
void mkdir_if_needed(char *path, mode_t perms);
mode_t permissions(struct Entry *e);


int main(int argc, char *argv[]) {
    struct Device *dev = NULL;
    struct Volume *vol = NULL;
    struct List *list, *node;

    fprintf(stderr, "unADF v%s : a unzip like for .ADF files, powered by ADFlib (v%s - %s)\n\n",
        UNADF_VERSION, adfGetVersionNumber(), adfGetVersionDate());

    adfEnvInitDefault();
    parse_args(argc, argv);

    /* mount device */
    if (!(dev = adfMountDev(adf_file, TRUE))) {
        fprintf(stderr, "%s: can't mount as device\n", adf_file);
        goto error_handler;
    }
    if (!pipe_mode) {
        print_device(dev);
    }

    /* mount volume */
    if (vol_number < 0 || vol_number >= dev->nVol) {
        fprintf(stderr, "%s: volume %d is invalid (device has %d volume(s))\n",
            adf_file, vol_number, dev->nVol);
        goto error_handler;
    }
    if (!(vol = adfMount(dev, vol_number, TRUE))) {
        fprintf(stderr, "%s: can't mount volume %d\n",
            adf_file, vol_number);
        goto error_handler;
    }
    if (!pipe_mode) {
        print_volume(vol);
    }

    if (pipe_mode && !file_list) {
        fprintf(stderr, "can't use pipe mode without giving a file name to extract\n");
        goto error_handler;
    }

    if (list_mode || list_all) {
        /* list files */
        if (use_dircache && isDIRCACHE(vol->dosType)) {
            BOOL true = TRUE;
            adfChgEnvProp(PR_USEDIRC, &true);
            puts("Using dir cache blocks.");
        }
        if (list_all) {
            /* list all files recursively */
            list = adfGetRDirEnt(vol, vol->curDirPtr, TRUE);
            print_tree(list, "");
        }
        else {
            /* list contents of root directory */
            list = adfGetDirEnt(vol, vol->curDirPtr);
            for (node = list; node; node = node->next) {
                print_entry(node->content, "");
            }
        }
        adfFreeDirList(list);
    }
    else {
        /* extract files */
        if (file_list) {
            /* extract list of files given on command line */
            for (node = file_list; node; node = node->next) {
                extract_filepath(vol, node->content);

                if (pipe_mode) {
                    /* only extract one file in pipe mode */
                    break;
                }
            }
        }
        else {
            /* extract all files */
            list = adfGetRDirEnt(vol, vol->curDirPtr, TRUE);
            extract_tree(vol, list, "");
            adfFreeDirList(list);
        }
    }

error_handler:
    if (vol) adfUnMount(vol);
    if (dev) adfUnMountDev(dev);
    if (file_list) freeList(file_list);
    adfEnvCleanUp();
    return 0;
}

/* parses the command line arguments into global variables */
void parse_args(int argc, char *argv[]) {
    struct List *list = NULL;
    int i, j;

    /* parse flags */
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        for (j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'l': list_mode     = TRUE; break;
            case 'r': list_all      = TRUE; break;
            case 'c': use_dircache  = TRUE; break;
            case 's': show_sectors  = TRUE; break;
            case 'm': show_comments = TRUE; break;
            case 'p':
                pipe_mode = TRUE;
                fprintf(stderr, list_mode
                    ? "-p option must be used with extraction, ignored\n"
                    : "sending files to pipe\n");
                break;
            case 'v':
                vol_number = atoi((i + 1) < argc ? argv[i + 1] : "0");
                i++, j = strlen(argv[i]) - 1; /* skip to next arg */
                break;
            case 'd':
                extract_dir = (i + 1) < argc ? argv[i + 1] : NULL;
                i++, j = strlen(argv[i]) - 1; /* skip to next arg */
                break;
            default: /* unknown option - show help and exit */
                help();
                exit(1);
            }
        }
    }

    /* ADF filename must come after options */
    if (i < argc) {
        adf_file = argv[i++];
    }
    else {
        help();
        exit(1);
    }

    /* list of files to extract can follow ADF filename */
    for (; i < argc; i++) {
        list = newCell(list, argv[i]);
        if (!file_list) {
            file_list = list;
        }
    }
}

void help() {
    fprintf(stderr,
        "unadf [-lrcsmp] [-v n] [-d extractdir] dumpname.adf [files-with-path]\n"
        "    -l : lists root directory contents\n"
        "    -r : lists directory tree contents\n"
        "    -c : use dircache data (must be used with -l)\n"
        "    -s : display entries logical block pointer (must be used with -l)\n"
        "    -m : display file comments, if exists (must be used with -l)\n"
        "    -p : send extracted files to pipe (unadf -p dump.adf Pics/pic1.gif | xv -)\n\n"
        "    -v n : mount volume #n instead of default #0 volume\n\n"
        "    -d dir : extract to 'dir' directory\n");
}

/* prints one line of information about a device */
void print_device(struct Device *dev) {
    printf("Device : %s. Cylinders = %d, Heads = %d, Sectors = %d. Volumes = %d\n",
        dev->devType == DEVTYPE_FLOPDD   ? "Floppy DD" :
        dev->devType == DEVTYPE_FLOPHD   ? "Floppy HD" :
        dev->devType == DEVTYPE_HARDDISK ? "Harddisk"  :
        dev->devType == DEVTYPE_HARDFILE ? "Hardfile"  : "???",
        dev->cylinders, dev->heads, dev->sectors, dev->nVol);
}

/* prints one line of information about a volume */
void print_volume(struct Volume *vol) {
    SECTNUM num_blocks = vol->lastBlock - vol->firstBlock + 1;

    switch (vol->dev->devType) {
    case DEVTYPE_FLOPDD:
        printf("Volume : Floppy 880 KBytes,");
        break;
    case DEVTYPE_FLOPHD:
        printf("Volume : Floppy 1760 KBytes,");
        break;
    case DEVTYPE_HARDDISK:
        printf("Volume : HD partition #%d %3.1f KBytes,", vol_number, num_blocks / 2.0);
        break;
    case DEVTYPE_HARDFILE:
        printf("Volume : HardFile %3.1f KBytes,", num_blocks / 2.0);
        break;
    default:
        printf("???,");
    }

    if (vol->volName) {
        printf(" \"%s\"", vol->volName);
    }

    printf(" between sectors [%d-%d]. %s%s%s. Filled at %2.1f%%.\n",
        vol->firstBlock,
        vol->lastBlock,
        isFFS(vol->dosType) ? "FFS" : "OFS",
        isINTL(vol->dosType) ? " INTL" : "",
        isDIRCACHE(vol->dosType) ? " DIRCACHE" : "",
        100.0 - ((adfCountFreeBlocks(vol) * 100.0) / num_blocks));
}

/* recurses through all directories/files and calls print_entry() on them */
void print_tree(struct List *node, char *path) {
    for (; node; node = node->next) {
        print_entry(node->content, path);
        if (node->subdir) {
            struct Entry *e = (struct Entry *) node->content;
            char *newpath = join_path(path, e->name);
            print_tree(node->subdir, newpath);
            free(newpath);
        }
    }
}

/* prints one line of information about a directory/file entry */
void print_entry(struct Entry *e, char *path) {
    BOOL is_dir = e->type == ST_DIR;
    BOOL print_comment = show_comments && e->comment && *e->comment;

    /* do not print the links entries, ADFlib do not support them yet properly */
    if (e->type == ST_LFILE || e->type == ST_LDIR || e->type == ST_LSOFT) {
        return;
    }

    /* print "size date time [ sector ] path/filename [, comment]" */
    printf(is_dir ? "       " : "%7d", e->size);
    printf("  %4d/%02d/%02d  %2d:%02d:%02d ",
        e->year, e->month, e->days, e->hour, e->mins, e->secs);
    if (show_sectors) {
        printf(" %06d ", e->sector);
    }
    printf(" %s%s%s%s%s%s\n",
        path,
        *path ? "/" : "",
        e->name,
        is_dir ? "/" : "",
        print_comment ? ", " : "",
        print_comment ? e->comment : "");
}

/* extracts all files, recursing into directories */
void extract_tree(struct Volume *vol, struct List *node, char *path) {
    for (; node; node = node->next) {
        struct Entry *e = node->content;

        /* extract file or create directory */
        char *out = output_name(path, e->name);
        if (e->type == ST_DIR) {
            if (!pipe_mode) {
                printf("x - %s/\n", out);
                mkdir_if_needed(out, permissions(e));
            }
        }
        else if (e->type == ST_FILE) {
            extract_file(vol, e->name, out, permissions(e));
        }
        if (!pipe_mode) {
            set_file_date(out, e);
        }
        free(out);

        /* recurse into subdirectories */
        if (node->subdir) {
            char *newpath = join_path(path, e->name);
            if (adfChangeDir(vol, e->name) == RC_OK) {
                extract_tree(vol, node->subdir, newpath);
                adfParentDir(vol);
            }
            else {
                fprintf(stderr, "%s: can't find directory %s in volume\n",
                    adf_file, newpath);
            }
            free(newpath);
        }
    }
}

/* follows a path to a file on disk and extracts just that file */
void extract_filepath(struct Volume *vol, char *filepath) {
    char *p, *element, *out;

    /* skip any leading slashes */
    for (p = filepath; *p == '/';) p++;

    /* switch to root directory */
    adfToRootDir(vol);

    /* find each path element and adfChangeDir() to each element */
    for (element = p; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (*element) {
                if (adfChangeDir(vol, element) != RC_OK) {
                    fprintf(stderr, "%s: can't find directory %s in volume\n",
                        adf_file, filepath);
                    return;
                }
            }
            *p = '/';
            element = p + 1;
        }
    }

    /* extract the file using the final path element */
    if (*element) {
        if (element == filepath) {
            /* no directory parts, just a filename */
            out = output_name("", element);
        }
        else {
            element[-1] = 0; /* split path and file */
            out = output_name(filepath, element);
        }
        extract_file(vol, element, out, 0666); /* assume rw permissions */
        free(out);
    }
}

/* copies a file from the volume to a given output filename */
void extract_file(struct Volume *vol, char *filename, char *out, mode_t perms) {
    struct File *f = NULL;
    uint8_t buf[EXTRACT_BUFFER_SIZE];
    int fd = 0;

    if (!(f = adfOpenFile(vol, filename, "r"))) {
        fprintf(stderr, "%s: can't find file %s in volume\n", adf_file, filename);
        goto error_handler;
    }

    /* open local file for writing */
    if (pipe_mode) {
        fd = 1; /* stdout */
    }
    else {
        printf("x - %s\n", out);
        fd = open(out, O_CREAT | O_WRONLY, perms);
        if (fd < 0) {
            perror(out);
            goto error_handler;
        }
    }

    /* copy from volume to local file until EOF */
    while (!adfEndOfFile(f)) {
        int32_t n = adfReadFile(f, sizeof(buf), buf);
        if (write(fd, buf, n) != n) {
            perror(out);
            goto error_handler;
        }
    }

error_handler:
    if (fd && !pipe_mode) close(fd);
    if (f) adfCloseFile(f);
}

/* combines path and file to "path/file", or just "file" if path is blank */
char *join_path(char *path, char *file) {
    char *newpath = malloc(strlen(path) + strlen(file) + 2);
    if (!newpath) {
        perror(adf_file);
        exit(1);
    }
    sprintf(newpath, "%s%s%s", path, *path ? "/" : "", file);
    return newpath;
}

/* creates a suitable output filename from the amiga filename */
char *output_name(char *path, char *name) {
    int dirlen = extract_dir ? strlen(extract_dir) + 1 : 0;
    char *out = malloc(dirlen + strlen(path) + strlen(name) + 2), *s, *o;
    if (!out) {
        perror(adf_file);
        exit(1);
    }

    /* copy user's extract directory if needed */
    if (dirlen) {
        strcpy(out, extract_dir);
        out[dirlen - 1] = DIRSEP;
    }

    /* copy path and name, translating '/' to platform's separator */
    for (s = path, o = &out[dirlen]; *s; s++) {
        *o++ = (*s == '/') ? DIRSEP : *s;
    }
    if (*path) {
        *o++ = DIRSEP;
    }
    strcpy(o, name);

    /* search for "../" and change to "xx" to stop directory traversal */
    for (o = &out[dirlen]; *o; o++) {
        if (o[0] == '.' && o[1] == '.' && (o[2] == '/' || o[2] == '\\')) {
            o[0] = o[1] = 'x';
            o += 2;
        }
    }

#ifdef WIN32
    /* TODO: remove characters : * " ? < > | from names (not allowed in Win32)
     * and remove any trailing dot or space (breaks Windows File Explorer)
     */
#endif

    if (!pipe_mode) {
        /* create directories leading up to name, if needed */
        for (o = out; *o; o++) {
            if (*o == DIRSEP) {
                *o = 0;
                mkdir_if_needed(out, 0777);
                *o = DIRSEP;
            }
        }
    }

    return out;
}

/* set amiga file date on output file */
void set_file_date(char *out, struct Entry *e) {
    time_t time;
#ifdef WIN32
    struct utimbuf times;
#else
    struct timeval times[2];
#endif

    struct tm tm;
    tm.tm_sec = e->secs;
    tm.tm_min = e->mins;
    tm.tm_hour = e->hour;
    tm.tm_mday = e->days;
    tm.tm_mon = e->month - 1;
    tm.tm_year = e->year - 1900;
    tm.tm_isdst = -1;
    time = mktime(&tm);

#ifdef WIN32
    /* on Windows, don't allow dates earlier than 1 Jan 1980 */
    if (time < 315532800) {
        time = 315532800;
    }
    times.actime = times.modtime = time;
    if (utime(out, &times) != 0) {
        perror(out);
    }
#else
    times[0].tv_sec = times[1].tv_sec = time;
    times[0].tv_usec = times[1].tv_usec = 0;
    if (utimes(out, times) != 0) {
        perror(out);
    }
#endif
}

/* creates directory, if it does not already exist */
void mkdir_if_needed(char *path, mode_t perms) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        if (mkdir(path, perms) != 0) {
            perror(path);
        }
    }
}

/* convert amiga permissions to unix permissions */
mode_t permissions(struct Entry *e) {
    return (!(e->access & 4) ? 0600 : 0400) | /* rw for user */
        (e->type == ST_DIR || !(e->access & 2) ? 0100 : 0) | /* x for user */
        ((e->access >> 13) & 0007) | /* rwx for others */
        ((e->access >> 5) & 0070); /* rwx for group */
}

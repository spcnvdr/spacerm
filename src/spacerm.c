/*****************************************************************************
 * Copyright 2019 Bryan Hawkins <spcnvdrr@protonmail.com>                    *
 *                                                                           *
 * Redistribution and use in source and binary forms, with or without        *
 * modification, are permitted provided that the following conditions        *
 * are met:                                                                  *
 *                                                                           *
 * 1. Redistributions of source code must retain the above copyright notice, *
 * this list of conditions and the following disclaimer.                     *
 *                                                                           *
 * 2. Redistributions in binary form must reproduce the above copyright      *
 * notice, this list of conditions and the following disclaimer in the       *
 * documentation and/or other materials provided with the distribution.      *
 *                                                                           *
 * 3. Neither the name of the copyright holder nor the names of its          *
 * contributors may be used to endorse or promote products derived from      *
 * this software without specific prior written permission.                  *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     *
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      *
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  *
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 *                                                                           *
 *  The stlcpy and strlcat functions are copyrighted by Todd C. Miller,      *
 *  <Todd.Miller@courtesan.com>. See the LICENSE file for the full copyright *
 *  notice regarding their use.                                              *
 *                                                                           *
 *  A simple C program to remove spaces and other annoying                   *
 *  characters from one or more filenames. Windows allows spaces in          *
 *  file names which makes it annoying to specify those files on             *
 *  the command line. This program will remove the spaces from the           *
 *  file name by simply renaming the file. It would probably be              *
 *  easier to make this program as a BASH script, but eh. When               *
 *  using this program, you can specify one or more files on the             *
 *  command line. The backup/-b option will copy the file to a new           *
 *  file whose name does not include spaces. This is called backup           *
 *  because the original file is not modified. The default mode is           *
 *  to rename the original file. The interactive option will prompt          *
 *  the user for a Yes/No answer before each operation is carried            *
 *  out. The other options are self explanatory.                             *
 *****************************************************************************/
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <argp.h>

/* Use PATH_MAX if we have it. Max length of a path + 1 for NUL termination */
#ifdef PATH_MAX
#define MAXLEN (PATH_MAX + 1)
#else
#define MAXLEN (4096+1)
#endif

/* Size of buffer to use for I/O operations */
#define IO_BUFSIZ    (128*1024)

const char *argp_program_version = "spacerm 1.3.1";
const char *argp_program_bug_address = "<spcnvdrr@protonmail.com>";

/* Program documentation */
static char doc[] = "Remove spaces from FILE name(s)\
	\vThis program removes spaces from file names by renaming the file.\
	Optionally, the original file can be left unchanged with the --backup\
	option. The --backup option copies the file contents to a new file with\
	spaces removed from the file name. The --interactive option will prompt\
	the user for approval before each operation is carried out.\n\
	e.g. \tspacerm 'first file' 'second file' 'third file'\n\
	\tspacerm -iu 'first file'\n\
	\tspacerm -b -v '/home/user/file name'";

/* A description of the arguments we accept */
static char args_doc[] = "FILE...";

/* The options we understand */
static struct argp_option options[] = {
	{"backup",      'b',  0,    0,  "Leave the original file unchanged", 0},
	{"dash",        'd',  0,    0,  "Replace spaces with dashes/hyphens", 0},
	{"interactive", 'i',  0,    0,  "Prompt before renaming or copying file", 0},
	{"just-print",  'n',  0,    0,  "Print what would be done, but do nothing", 0},
	{"dry-run",      0,   0,    OPTION_ALIAS, "Alias for just-print", 0},
	{"strip",       's',  "CHARS", 0, "Remove the given characters from filename", 0},
	{"underscore",  'u',  0,    0,  "Replace spaces with underscores", 0},
	{"verbose",     'v',  0,    0,  "Verbosely list files processed", 0},
	{0}
};

/* Used by main to communicate with parse_opt */
struct arguments {
	char *arg1;                   /* First mandatory filename */
	char **strings;               /* Any other filenames to fix */
	char *strip;                  /* A string of chars to remove from filename */
	int verbose;                  /* Set if verbose mode */
	int backup;                   /* Copy to a file with modified file name */
	int dashes;                   /* Replace spaces with dashes */
	int interactive;              /* Prompt before action */
	int underscore;               /* Replace spaces with underscores */
	int dryrun;                   /* Print what would happen, but do nothing */
};

/* Global variables to signal various modes */
struct arguments cmdargs = {0};


/** Find the file name in a path
 * @param path The path to the file
 * @returns A pointer to the start of the filename, or NUL
 * if there is no filename component or an error occured
 * NOTE: path MUST be a NUL terminated string!
 * Could also replace this with strrchr()
 *
 */
static char *getfname(char *path){
	char *ptr = path;
	int i;

	if(path == NULL || *path == '\0')
		return("");

	for(i = 0; *path != '\0'; i++, path++){
		if(*path == '/'){
			ptr = ++path;
		}
	}

	return(ptr);
}


/* NOTE: The strlcpy() and strlcat functions are copyrighted by:
 * Todd C. Miller <Todd.Miller@courtesan.com>. See the LICENSE file
 * for the complete copyright notice.
 */

/** Copy string src to buffer dst of size dsize. At most, dsize-1
 * chars will be copied. Always NUL terminate (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
static size_t strlcpy(char* dst, const char *src, size_t dsize){
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit */
	if(nleft != 0){
		while(--nleft != 0){
			if((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if(nleft == 0){
		if(dsize != 0)
			*dst = '\0';            /* NUL-terminate dst */
		while(*src++)
			;
	}

	/* Count does not include NUL */
	return(src - osrc - 1);
}


/** Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left). At most, dsize-1 characters
 * will be copied. Always NUL terminate (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occured.
 */
static size_t strlcat(char *dst, const char *src, size_t dsize){
	const char *odst = dst;
	const char *osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left, but don't go past end */
	while(n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if(n-- == 0)
		return(dlen + strlen(src));
	while(*src != '\0'){
		if(n != 0){
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	/* Count does not include NUL */
	return(dlen + (src - osrc));
}


/** Check if c occurs in the string str
 * @param str The string to search
 * @param c The character to search for
 * @returns 1 (True) if c occurs in str, else 0 (False)
 *
 */
static int instr(char *str, int c){
	char ch = (char) c;
	/* Continue until we hit NUL or find a match */
	while(*str != '\0' && *str != ch)
		str++;

	/* Did we find a match? */
	return(*str == ch);
}


/** Remove all occurrences of the characters in rm from a string
 *  @param str The string to remove characters from
 *  @param rm A NUL terminated string of characters to remove from str
 *
 */
static void rmchr(char *str, char *rm){
	char *ptr = str;
	char *ptrw = str;

	if(str == NULL || rm == NULL)
		return;

	/* While we haven't reached the end of the string */
	while(*ptr != '\0'){
		/* Copy the current char into ptrw */
		*ptrw = *ptr++;
		/* Only advance the ptrw pointer if the char we copied was not in rm */
		ptrw += (!instr(rm, *ptrw));
	}

	/* NUL terminate the string in case it was shortened */
	*ptrw = '\0';
}


/** Replace all occurrences of x with y in the string str
 * @param str The string to substitute characters in
 * @parma len The length of the string
 * @param x The character to be replaced
 * @param y The character to replace x with
 *
 */
static void subchr(char *str, size_t len, char x, char y){
	size_t i;

	for(i = 0; str[i] != '\0' && i < len; i++){
		if(str[i] == x){
			str[i] = y;
		}
	}
}


/** Removes spaces from a file name and places the result in newpath
 * @param oldpath The relative or absolute path of the file to fix.
 * @param newpath A char array to store the new file path in.
 * @param len The maximum number of characters that newpath can hold
 * @returns 0 if successful, -1 upon error.
 * The return value is pointless for now, but may be used in the future
 *
 */
static int fixfilename(const char *oldpath, char *newpath, size_t len){
	size_t ret;
	char *ext, *extcpy, *ptr;

	/* Copy oldpath into newpath so we don't modify the original path */
	if((ret = strlcpy(newpath, oldpath, len)) >= len){
		fprintf(stderr, "spacerm: strlcpy() error: truncation occurred\n");
		exit(EXIT_FAILURE);
	}

	/* Find the filename in the path, then call rmchars b/c we
	 * don't want to remove spaces from directory names in the path.
	 * If we do, we end up trying to rename files or move files to a
	 * nonexistent path. We want to avoid the following:
	 * e.g. '/home/user/dir 1/filename 1' -> '/home/user/dir1/filename1'
	 * If we don't find any slashes, ptr == &newpath[0]
	 */
	ptr = getfname(newpath);
	if(*ptr == '\0'){
		fprintf(stderr, "spacerm: path error: bad path\n");
		return(-1);
	}

	if(cmdargs.strip){
		/* Find the extension */
		ext = strrchr(ptr, '.');
		if(ext != NULL){
			/* make a copy of just the extension */
			extcpy = strndup(ext, 100);
			if(extcpy == NULL){
				perror("spacerm: strndup() error");
				exit(EXIT_FAILURE);
			}

			/* split the original filename, remove the given characters,
			 * and append the extension back on the end. */
			*ext = '\0';
			rmchr(ptr, cmdargs.strip);
			if((strlcat(ptr, extcpy, len)) >= len){
				fprintf(stderr, "spacerm: path error: truncation occurred\n");
				free(extcpy);
				return(-1);
			}

			free(extcpy);

		} else {
			/* There is no extension to worry about, strip away! */
			rmchr(ptr, cmdargs.strip);
		}
	}

	if(cmdargs.underscore){
		subchr(ptr, strlen(ptr), ' ', '_');
	} else if(cmdargs.dashes){
		subchr(ptr, strlen(ptr), ' ', '-');
	} else {
		rmchr(ptr, " ");
	}

	return(0);
}


/** Parse a single option with argp
 * @param key The option, or key, to parse
 * @param arg The argument to the current option, if present
 * @param state The current argp state
 * @returns 0 on success, or ARGP_ERR_UNKNOWN if an unknown option is passed
 *
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state){
	/* Get the input argument from argp_parse, which we
	 * know is a pointer to our arguments structure */
	struct arguments *arguments = state->input;

	switch(key){
		case 'b':
		/* Do not modify original file */
		cmdargs.backup = 1;
		break;

		case 'd':
		/* Replace spaces with dashes */
		cmdargs.dashes = 1;
		break;

		case 'i':
		/* Interactive mode, prompt before action */
		cmdargs.interactive = 1;
		break;

		case 'n':
		/* Just print what we would do */
		cmdargs.dryrun = 1;
		break;

		case 's':
		cmdargs.strip = arg;
		break;

		case 'u':
		/* Replace spaces with underscores */
		cmdargs.underscore = 1;
		break;

		case 'v':
		/* Print extra information */
		cmdargs.verbose = 1;
		break;

		case ARGP_KEY_NO_ARGS:
		/* We got no arguments, show help */
		argp_usage(state);
		/* Never get here */
		break;

		case ARGP_KEY_ARG:
		/* Here we know that state->arg_num == 0, since we
		 * force argument parsing to end before any more arguments can
		 * get here */
		arguments->arg1 = arg;

		/* Now we consume all the rest of the arguments.
		 * state->next is the index in state->argv of the
		 * next argument to be parsed, which is the first string
		 * we're interested in, so we can just use
		 * &state->argv[state->next] as the value for
		 * arguments->strings
		 * in addition, by setting state->next to the end
		 * of the arguments we can force argp to stop parsing here and
		 * return */
		arguments->strings = &state->argv[state->next];
		state->next = state->argc;
		break;

		default:
			return(ARGP_ERR_UNKNOWN);
	}
	return(0);
}

/** Get the permissions of a file
 * @param file The file to get the permissions of
 * @returns The permissions of the file
 *
 */
static mode_t getfmode(const char *file){
	struct stat sbuf;

	if(stat(file, &sbuf) < 0){
		perror("spacerm: stat() error");
		exit(EXIT_FAILURE);
	}

	return(sbuf.st_mode);

}


/** Copy a file from infile to outfile
 * @param infile The file to read data from
 * @param outfile The file to write data to
 * @returns 0 on success, else -1 upon error
 *
 */
static int cpfile(const char *infile, const char *outfile){
	int infd, outfd;
	ssize_t n;
	mode_t oldmask, fmode;

	/* Allocate our data buffer */
	char *buf = malloc(IO_BUFSIZ * sizeof(char));
	if(!buf){
		perror("spacerm: malloc() error");
		return(-1);
	}

	/* Open the input and output files, creating the output file if needed */
	if((infd = open(infile, O_RDONLY)) < 0){
		perror("spacerm: open() error");
		free(buf);
		return(-1);
	}

	/* Set the umask to 0 and get the permissions of infile */
	oldmask = umask(0);
	fmode = getfmode(infile);

	/* Create the new file with the same permissions as infile */
	if((outfd = open(outfile, O_CREAT | O_WRONLY, fmode)) < 0){
		perror("spacerm: open() error");
		free(buf);
		close(infd);
		return(-1);
	}

	/* restore the processes's umask */
	umask(oldmask);

	/* Announce our intention to read the file sequentially */
	posix_fadvise(infd, 0, 0, POSIX_FADV_SEQUENTIAL);

	/* Copy the file one buffer at a time */
	while((n = read(infd, buf, IO_BUFSIZ)) > 0){
		if(write(outfd, buf, (size_t)n) != n){
			perror("spacerm: write() error");
			goto fail;
		}
	}
	/* If read() hit an error, print the error and return -1 */
	if(n < 0){
		perror("spacerm: read() error");
		goto fail;
	}

	/* Clean up our resources */
	close(infd);
	close(outfd);
	free(buf);

	return(0);

	/* We jump here if something went wrong */
fail:
	close(infd);
	close(outfd);
	free(buf);
	return(-1);
}


/** Get an affirmative or negative answer from the user
 * @returns 0 (False) if negative, 1 (True) if affirmative
 *
 */
static int yesno(void){
	char *response = NULL;    /* Tell getline to malloc space for the response */
	size_t response_size = 0;
	ssize_t response_len = getline(&response, &response_size, stdin);
	int yes;

	if(response_len <= 0)
		yes = 0;
	else {
	/* Only return True if user entered affirmative,
	 * and rpmatch was successful */
	response[response_len - 1] = '\0';
	yes = (0 < rpmatch(response));
	}

	free(response);
	return(yes);
}


/** Check the permissions on the file and the directory it resides in. Also
 * ensure that we only operate on regular files.
 * @param path the path of the file
 * @returns 0 on success, -1 if an error occurred
 * or insufficient permissions
 *
 */
static int checkperm(const char *path){
	char *dir, *backup;
	int ret;
	struct stat st;

	if(stat(path, &st) < 0){
		perror("spacerm: stat() error");
		return(-1);
	}

	/* If it is not a regular file, exit */
	if(!S_ISREG(st.st_mode)){
		fprintf(stderr, "spacerm: file error: %s is not a regular file\n", path);
		return(-1);
	}

	/* Make sure file exists and we have adequate permissions,
	 * otherwise */
	if((ret = access(path, R_OK)) < 0){
		perror("spacerm: access() error");
		return(ret);
	}

	if((backup = strndup(path, MAXLEN)) == NULL){
		perror("spacerm: strndup() error");
		exit(EXIT_FAILURE);
	}

	dir = dirname(backup);

	/* Test for write access to the directory that the file resides in. */
	if((ret = access(dir, R_OK|W_OK)) < 0){
		perror("spacerm: access() error");
		free(backup);
		return(ret);
	}

	free(backup);
	return(ret);

}


/** Fix the file name of the file at the given path
 * @param file The path of the file name to remove spaces from
 * @returns 0 on success, -1 on failure.
 */
static int spacerm(char *file){
	char newpath[MAXLEN] = {0};
	int resp, ret = 0;

	/* Check permissions on the file and directory. */
	if((ret = checkperm(file)) < 0){
		exit(EXIT_FAILURE);
	}


	/* Generate a corrected file name and act according to the given
	 * command line options */
	if(fixfilename(file, newpath, MAXLEN) < 0)
		return(-1);

	if(cmdargs.backup){
		if(cmdargs.interactive){
			printf("spacerm: copy '%s' to '%s'? (Yes/No): ", file, newpath);
			/* If user said no, abort operation */
			if((resp = yesno()) == 0)
				return(-1);
		}

		if(cmdargs.dryrun){
			/* Print out what actions we would take and return */
			printf("copy file: '%s' -> '%s'\n", file, newpath);
			return(0);
		}

		/* Do the actual copy */
		if((ret = cpfile(file, newpath)) < 0){
			exit(EXIT_FAILURE);
		}

		/* Print some useful information if verbose mode is enabled */
		if(cmdargs.verbose){
			printf("copied file: '%s' -> '%s'\n", file, newpath);
		}
	} else {
		if(cmdargs.interactive){
			printf("spacerm: rename '%s' to '%s'? (Yes/No): ", file, newpath);
			/* If user said no, abort operation */
			if((resp = yesno()) == 0)
				return(-1);
		}

		if(cmdargs.dryrun){
			printf("rename file: '%s' -> '%s'\n", file, newpath);
			return(0);
		}

		/* Attempt to rename the file */
		if((ret = rename(file, newpath)) < 0){
			perror("spacerm: rename() error");
			exit(EXIT_FAILURE);
		}

		if(cmdargs.verbose){
			printf("renamed file: '%s' -> '%s'\n", file, newpath);
		}
	}

	return(ret);
}


/* Our argp parser */
static struct argp argp = {options, parse_opt, args_doc, doc};


/**
 *  Usage: spacerm [-bdinuv?V] [-s CHARS] [--backup] [--dash] [--interactive]
 *           [--just-print] [--dry-run] [--strip=CHARS] [--underscore]
 *           [--verbose] [--help] [--usage] [--version] FILE...
 *
 */
int main(int argc, char *argv[]){
	int i;

	/* Parse command-line arguments */
	argp_parse(&argp, argc, argv, 0, 0, &cmdargs);

	/* Iterate through and process each file named on the command line */
	spacerm(cmdargs.arg1);
	for(i = 0; cmdargs.strings[i]; i++)
		spacerm(cmdargs.strings[i]);

	return(EXIT_SUCCESS);
}

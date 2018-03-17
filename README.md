# spacerm - The Space Remover
Spacerm is a simple program to remove spaces and other undesirable
characters from file names. By default, only spaces are removed from
the specified files and each file is renamed in place. The --backup 
or -b option will create a copy of the original file with the new
filename not containing spaces. The --strip or -s option allows the
user to specify additional characters to be removed from the file
names. Spaces are still removed from filenames when the --strip
option is used. The file extension is not changed when using the
--strip/-s option. Optionally, spaces can be replaced with 
underscores instead of being removed by using the --underscore/-u 
option. The --interactive/-i option will prompt the user for 
approval before renaming each file. The --just-print/--dry-run/-n 
option will just print what actions would be carried out, but no 
changes are made to any file names. Finally, the --verbose/-v option
prints out information each time a file is renamed. Also, note that
multiple files can be specified on the command line at once.

*** Examples ***
spacerm 'file 1' 'file 2' 'file 3'
spacerm -vui /home/user/bad\ file\ name.pdf
spacerm --strip=. bad.file.name.pdf 


*** Building ***
To compile with GCC C99 add the following defines before all the include 
statements:
#define _POSIX_C_SOURCE 200809L
#define _SVID_SOURCE

Then run the following command:
gcc -std=c99 -Wall -pedantic ./spacerm.c -o spacerm

The defines are not needed if compiled with the following command:
gcc -std=gnu99 -Wall -pedantic ./spacerm.c -o spacerm 

To run splint:
splint +showsummary -warnposix ./spacerm.c > splint.out
splint +showsummary -warnposix +usestderr ./spacerm.c  2> splint.out



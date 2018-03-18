# spacerm - The Space Remover
Spacerm is a simple program to remove spaces and other undesirable
characters from one or more file names. Often times there are
file names that are a pain to specify on the command line or are
just plain annoying. This program makes it easy to transform these
undesirable file names in to something more palatable. While there may
be better programs that handle this mundane task, this program is
extremely simple to use. This project was started as just something fun
and useful to make. I find it useful for renaming large batches of files
and maybe someone else will too.


**Available Options**

This section describes just some of the available options. For all the
supported options, run the program with the --help option. By default,
only spaces are removed from the specified files and each file is
renamed in place. The --backup or -b option will create a copy of the
original file with the new file name not containing spaces.
The --strip or -s option allows the user to specify additional characters
to be removed from the file names. Spaces are still removed from file
names when the --strip option is used. The file extension is not
changed when using the --strip/-s option. Optionally, spaces can be
replaced with underscores instead of being removed by using the
--underscore/-u option. The --interactive/-i option will prompt the user for
approval before renaming each file. The --just-print/--dry-run/-n
option will just print what actions would be carried out, but no
changes are made to any file names. Finally, the --verbose/-v option
prints out information each time a file is renamed. Also, note that
multiple files can be specified on the command line at once.


**Getting Started**

This program requires a C compiler and make to compile. By default the
Makefile relies on GCC, but this can easily be changed to use Clang
instead.

To build this program, first get a copy of the source code and change
into the spacerm directory.

    cd ./spacerm

Then simply type 'make' to build

    make

For help and to see the available options

    ./spacerm --help

**Examples**

Remove spaces from 'file 1' changing it to 'file1'. Repeated for the
other files specified.

    spacerm 'file 1' 'file 2' 'file 3'

Rename '/home/user/bad file name.pdf' to '/home/user/bad-file-name.pdf'
and prompt the user to confirm this before renaming.

    spacerm -di /home/user/bad\ file\ name.pdf

Rename the file 'bad.file.name.pdf' to 'badfilename.pdf'

    spacerm --strip=. bad.file.name.pdf


**Contributing**

Pull requests, new feature suggestions, and bug reports/issues are
welcome.


**License**

This project is licensed under the 3-Clause BSD License also known as the
*"New BSD License"* or the *"Modified BSD License"*. A copy of the license
can be found in the LICENSE file. A copy can also be found at the
[Open Source Institute](https://opensource.org/licenses/BSD-3-Clause)

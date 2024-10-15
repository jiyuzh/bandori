#!/usr/bin/env false

# To use this module without install:
# use FindBin;
# use lib "$FindBin::Bin/relative/path/to/this/dir";
# use bandori;

package bandori;

use strict;
use warnings;

use Cwd qw(getcwd cwd abs_path);
use File::Basename;
use File::Spec;

require Exporter;

our @ISA = qw(Exporter);

# Exported to module namespace
our @EXPORT_OK = qw();

# Exported to global and module namespace
our @EXPORT = qw(
	readAllText
	writeAllText
	appendAllText

	enumerateFiles
	enumerateDirectories
	enumerateFileSystemEntries
	getRealPath

	streamReadToEnd
);

our $VERSION = '0.01';

#
# File operations
#

# Opens a text file, reads all the text in the file into a string, and then closes the file.
# If the target file cannot be read and a default value is provided, the default value is returned instead.
sub readAllText
{
	my ($path, $default) = @_;

	open(my $fd, '<', $path) or do {
		return $default if (defined $default);
		die "Could not open $path: $!\n";
	};

	my $text = do { local $/; <$fd> };
	close($fd);

	return $text;
}

# Creates a new file, write the contents to the file, and then closes the file.
# If the target file already exists, it is truncated and overwritten.
sub writeAllText
{
	my ($path, $text) = @_;

	open(my $fd, '>', $path) or die "Could not open $path: $!\n";
	$fd->print($text);
	close($fd);
}

# Appends the specified string to the file, creating the file if it does not already exist.
sub appendAllText
{
	my ($path, $text) = @_;

	open(my $fd, '>>', $path) or die "Could not open $path: $!\n";
	$fd->print($text);
	close($fd);
}

#
# Directory Operations
#

# Enumerate the given path and return the path to the containing files
sub enumerateFiles
{
	my ($path) = @_;

	opendir(my $dir, $path) or die "Could not enumerate $path: $!\n";
	my @files = grep{ -f $_ } map{ abs_path("$path/$_") } readdir($dir);
	closedir($dir);

	return \@files;
}

# Enumerate the given path and return the path to the containing directories
sub enumerateDirectories
{
	my ($path) = @_;

	opendir(my $dir, $path) or die "Could not enumerate $path: $!\n";
	my @files = grep{ -d $_ } map{ abs_path("$path/$_") } grep{ ($_ ne ".") && ($_ ne "..") } readdir($dir);
	closedir($dir);

	return \@files;
}

# Enumerate the given path and return the path to the containing file system entries
sub enumerateFileSystemEntries
{
	my ($path) = @_;

	opendir(my $dir, $path) or die "Could not enumerate $path: $!\n";
	my @files = map{ abs_path("$path/$_") } grep{ ($_ ne ".") && ($_ ne "..") } readdir($dir);
	closedir($dir);

	return \@files;
}

# Get the real directory and file path of the given file.
sub getRealPath
{
	my ($script) = @_;
	my $bin;
	my $dir;

	# Ensure that we are dealing with a file
	unless (-f $script) {
		return "";
	}

	# Expand to absolute path
	unless (File::Spec->file_name_is_absolute($script)) {
		my $cwd = getcwd();
		$cwd = cwd() unless(defined $cwd);
		$script = File::Spec->catfile(getcwd(), $script)
	}

	# Resolve $script if it is a link
	while (1) {
		my $linktext = readlink($script);

		($bin, $dir) = fileparse($script);
		last unless defined $linktext;

		$script = (File::Spec->file_name_is_absolute($linktext))
					? $linktext
					: File::Spec->catfile($dir, $linktext);
	}

	$dir = abs_path($dir) if ($dir);

	return ($dir, $bin);
}

#
# Stream Operations
#

# Reads all characters from the current position to the end of the stream.
# To use with STDIN, pass \*STDIN as parameter.
sub streamReadToEnd
{
	my ($fd) = @_;

	my $text = do { local $/; <$fd> };
	return $text;
}

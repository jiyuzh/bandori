#!/usr/bin/env false

# To use this module without install:
# use File::Basename;
# use lib dirname (__FILE__) . "relative/path/to/this/dir";
# use bandori;

package bandori;

use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);

# Exported to module namespace
our @EXPORT_OK = qw();

# Exported to global and module namespace
our @EXPORT = qw(
	readAllText
	writeAllText
	appendAllText
	streamReadToEnd
);

our $VERSION = '0.01';

# Opens a text file, reads all the text in the file into a string, and then closes the file.
sub readAllText
{
	my ($path) = @_;

	open(my $fd, '<', $path) or die "Could not open $path: $!\n";
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

# Reads all characters from the current position to the end of the stream.
# To use with STDIN, pass \*STDIN as parameter.
sub streamReadToEnd
{
	my ($fd) = @_;

	my $text = do { local $/; <$fd> };
	return $text;
}


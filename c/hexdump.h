#pragma once

#include "headers.h"
#include "common.h"

#define HD_BINARY BIT(0)
#define HD_CHARTX BIT(1)
#define HD_OFFSET BIT(2)

/*
 *	hexdump - output a hex dump of a buffer
 *
 *	fd is file descriptor to write to
 *	data is pointer to the buffer
 *	length is length of buffer to write
 *	linelen is number of chars to output per line
 *	split is number of chars in each chunk on a line
 */
fn int hexdump_core(FILE *fd, void const *data, size_t length, int linelen, int split, int mode)
{
	char *buffer;
	char *ptr;
	const void *inptr;
	int pos;
	int remaining = length;

	inptr = data;

	/*
	 *	Assert that the buffer is large enough. This should pretty much
	 *	always be the case...
	 *
	 *	hex/ascii gap (2 chars) + closing \0 (1 char)
	 *	split = 4 chars (2 each for hex/ascii) * number of splits
	 *
	 *	(hex = 3 chars, ascii = 1 char) * linelen number of chars
	 */
	buffer = malloc(32 + (4 * (linelen / split)) + (linelen * 4));
	if (!buffer)
		return -1;

	/* Nothing to do */
	if (!(mode & (HD_BINARY | HD_CHARTX)))
		return -1;

	/*
	 *	Loop through each line remaining
	 */
	while (remaining > 0) {
		int lrem;
		int splitcount;
		ptr = buffer;

		if (mode & HD_OFFSET) {
			lrem = sprintf(ptr, "0x%08lx  |  ", inptr - data);
			if (lrem >= 0)
				ptr += lrem;
		}

		if (!(mode & HD_BINARY))
			goto print_chartx;

		/*
		 *	Loop through the hex chars of this line
		 */
		lrem = remaining;
		splitcount = 0;
		for (pos = 0; pos < linelen; pos++) {

			/* Split hex section if required */
			if (split == splitcount++) {
				sprintf(ptr, "  ");
				ptr += 2;
				splitcount = 1;
			}

			/* If still remaining chars, output, else leave a space */
			if (lrem) {
				sprintf(ptr, "%02x ", *((unsigned char *) inptr + pos));
				lrem--;
			} else {
				sprintf(ptr, "   ");
			}
			ptr += 3;
		}

		if (!(mode & HD_CHARTX))
			goto done;

		*ptr++ = ' ';
		*ptr++ = '|';
		*ptr++ = ' ';
		*ptr++ = ' ';

print_chartx:
		/*
		 *	Loop through the ASCII chars of this line
		 */
		lrem = remaining;
		splitcount = 0;
		for (pos = 0; pos < linelen; pos++) {
			unsigned char c;

			/* Split ASCII section if required */
			if (split == splitcount++) {
				sprintf(ptr, "  ");
				ptr += 2;
				splitcount = 1;
			}

			if (lrem) {
				c = *((unsigned char *) inptr + pos);
				if (c > 31 && c < 127) {
					sprintf(ptr, "%c", c);
				} else {
					sprintf(ptr, ".");
				}
				lrem--;
			} else {
				sprintf(ptr, " ");
			}
			ptr++;
		}

done:
		*ptr = '\0';
		fprintf(fd, "%s\n", buffer);

		inptr += linelen;
		remaining -= linelen;
	}

	free(buffer);

	return 0;
}

fn int hexdump(void const *data, size_t length)
{
	return hexdump_core(stdout, data, length, 32, 8, HD_OFFSET | HD_BINARY | HD_CHARTX);
}

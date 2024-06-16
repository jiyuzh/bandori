#pragma once

#include "headers.h"
#include "common.h"
#include "kernel.h"

fn void wait_enter_key(char *desc)
{
	int c = 0;

	printf("Press ENTER to %s ...\n", desc ? desc : "continue");
	fflush (stdout);
	while (c != '\n') {
		c = getchar();
	}
}

typedef struct formatted_size {
	char str[16];
} formatted_size;

// Format a size into human readable form.
// The buf should be able to hold at least 12 visible characters.
fn formatted_size format_size_impl(u64 bytes)
{
	formatted_size ret = { 0 };
	char *unit = NULL;
	u64 base = 0;

	if (bytes < KB(1)) {
		// We use short here to make static buffer size checker happy
		sprintf(ret.str, "%hu bytes", (short) bytes);
		return ret;
	}
	else if (bytes < MB(1)) {
		unit = "KiB";
		base = KB(1);
	}
	else if (bytes < GB(1)) {
		unit = "MiB";
		base = MB(1);
	}
	else if (bytes < TB(1)) {
		unit = "GiB";
		base = GB(1);
	}
	else if (bytes < PB(1)) {
		unit = "TiB";
		base = TB(1);
	}
	else {
		unit = "PiB";
		base = PB(1);
	}

	sprintf(ret.str, "%.2Lf %s", bytes * 1.0L / base, unit);
	return ret;
}

#define format_size(bytes) (format_size_impl(bytes).str)

// Parse a human readable size into its byte count.
// This implementation is aware of 'B' (byte) and 'b' (bit), preferring byte when ambiguous.
// This implementation will handle hex and float numbers, round up when necessary.
fn u64 parse_size(char *input)
{
	char *endp = input;
	char size = '\0';
	long double value, bak;

	errno = 0;
	value = strtold(input, &endp);
	bak = value;

	if(errno || endp == input || value < 0)
		return -1ULL;

	// Eat possible spaces
	while (*endp == ' ')
		endp++;

	size = *endp;

	switch (size) {
	case 'B':
	case 'b':
		break;

	case 'k':
	case 'K':
		value *= KB(1);
		break;

	case 'm':
	case 'M':
		value *= MB(1);
		break;

	case 'g':
	case 'G':
		value *= GB(1);
		break;

	case 't':
	case 'T':
		value *= TB(1);
		break;

	case 'p':
	case 'P':
		value *= PB(1);
		break;

	case '\0':
		return ceil(value);

	default:
		printf("Unrecognized size unit in input '%s', default to Byte\n", input);
		return ceil(value);
	}

	// Eat the size prefix
	endp++;

	// 127 [bBkKmMgGtTpP]$
	if (!strcmp(endp, "")) {
		// 127 b, consider intentional
		if (size == 'b')
			value /= 8;
		goto result;
	}

	// 127 [bB].+$
	if (size == 'b' || size == 'B') {
		// 127 bytes?
		if (!strcasecmp(endp, "YTE") || !strcasecmp(endp, "YTES"))
			goto result;
		// 127 bits?
		if (!strcasecmp(endp, "IT") || !strcasecmp(endp, "ITS")) {
			value /= 8;
			goto result;
		}
		// Other 127 [bB].+ are illegal
		goto error;
	}

	// 127 [kKmMgGtTpP].+$

	// 127 [kKmMgGtTpP](B|iB|bytes?)$
	if (!strcmp(endp, "B") || !strcmp(endp, "iB") || !strcasecmp(endp, "BYTE") || !strcasecmp(endp, "BYTES"))
		goto result;

	// 127 [kKmMgGtTpP](b|ib)$
	if (!strcmp(endp, "b") || !strcmp(endp, "ib")) {
		// 127 [KMGTP](b|ib), consider intentional
		if ('A' <= size && size <= 'Z')
			value /= 8;

		goto result;
	}

	// 127 [kKmMgGtTpP](bits?)$
	if (!strcasecmp(endp, "BIT") || !strcasecmp(endp, "BITS")) {
		value /= 8;
		goto result;
	}

error:
	printf("Unrecognized size unit in input '%s', default to Byte\n", input);
	value = bak;

result:
	return ceil(value);
}


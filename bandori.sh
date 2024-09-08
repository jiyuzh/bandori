#!/usr/bin/env false

set -Eeuo pipefail

#
# The Spell
#

: <<'END'
#!/usr/bin/env bash

set -Eeuo pipefail

bandori="bandori.sh" # relative path
source "$(dirname "$(realpath -e "${BASH_SOURCE[0]:-$0}")")/$bandori"
END



#
# Knobs
#

: <<'END'
BANDORI_QUIET (default: unset)
	When set and not empty, suppress unnecessary report messages.

BANDORI_REINIT (default: unset)
	When set and not empty, treat the current script as top level (user-invoked) script.

BANDORI_CLEAN_PR (default: unset)
	When set and not empty, do not print caller location for pr_* macros.
END



#
# Standard Setup
#

# Init routine, called only at the top level
# Arguments:
#   $1: The $0 of the environment
function init_once_bash
{
	local cmdline="$1"

	# Run time accounting
	BANDORI_SCRIPT_START=$(date +%s.%N)
	export BANDORI_SCRIPT_START

	# Create file for recursive trace
	BANDORI_SCRIPT_TRACE=$(mktemp)
	export BANDORI_SCRIPT_TRACE

	# Backup real $0 for later use
	export BANDORI_SCRIPT_IDENTITY="$cmdline"
}

# Fini routine, called only at the top level
function fini_once_bash
{
	# Cleanup recursive trace
	rm "$BANDORI_SCRIPT_TRACE"

	# Hook up BANDORI notifier for long running script (>= 1 min)
	if [ -n "${BURUAKA:-}" ] && [ "$(script_elapsed_sec)" -ge 60 ]; then
		"$BURUAKA/bin/bell" "Execution done: $BANDORI_SCRIPT_IDENTITY"
	fi
}

# Init routine, called every time this file get sourced
# Arguments:
#   $1: The $0 of the environment
function init_bash
{
	local cmdline="$1"

	if [ -n "${BANDORI_REINIT:-}" ]; then
		unset BANDORI_SCRIPT_NESTING
		unset BANDORI_REINIT
	fi

	# Script nesting handling, primarily used in signal handlers
	if [ -z "${BANDORI_SCRIPT_NESTING:-}" ]; then
		export BANDORI_SCRIPT_NESTING=1
	else
		export BANDORI_SCRIPT_NESTING=$((BANDORI_SCRIPT_NESTING + 1))
	fi

	
	if [ "$BANDORI_SCRIPT_NESTING" -eq 1 ]; then
		init_once_bash "$cmdline"
	fi
}

# Fini routine, called every time script is exiting
function fini_bash
{
	if [ "$BANDORI_SCRIPT_NESTING" -eq 1 ]; then
		fini_once_bash
	fi
}



#
# Who Am I
#

# Get the canonicalized path of the directory that contains the caller script
# This function will return the file location of the sourcee file, if sourced
function script_dir
{
	# Here we use [1] as [0] is always the file that defines `script_dir`
	dirname "$(realpath -e "${BASH_SOURCE[1]:-$0}")"
}

# Get the canonicalized path of the caller script file
# This function will return the file location of the sourcee file, if sourced
function script_file
{
	# See comment for `script_dir`
	realpath -e "${BASH_SOURCE[1]:-$0}"
}

# Get the time elapsed since execution start in seconds, with highest precision to nanoseconds
function script_elapsed_ns
{
	math "$(date +%s.%N) - $BANDORI_SCRIPT_START"
}

# Get the time elapsed since execution start in seconds, rounded down to an integer
function script_elapsed_sec
{
	printf "%.0f" "$(script_elapsed_ns)"
}



#
# Color and Print
#

# Translate color name to 256 color code
# Arguments:
#   $1: name or code of the color
# Note:
#   See https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit for color table
function color_name_to_code
{
	local cname="${1^^}"

	case "$cname" in

	BLACK) echo "0" ;;
	RED) echo "1" ;;
	GREEN) echo "2" ;;
	YELLOW) echo "3" ;;
	BLUE) echo "4" ;;
	MAGENTA) echo "5" ;;
	CYAN) echo "6" ;;
	WHITE) echo "7" ;;

	LIGHTBLACK|LBLACK|GRAY) echo "8" ;;
	LIGHTRED|LRED|ERR) echo "9" ;;
	LIGHTGREEN|LGREEN|SUCC) echo "10" ;;
	LIGHTYELLOW|LYELLOW|WARN) echo "11" ;;
	LIGHTBLUE|LBLUE|INFO) echo "12" ;;
	LIGHTMAGENTA|LMAGENTA) echo "13" ;;
	LIGHTCYAN|LCYAN) echo "14" ;;
	LIGHTWHITE|LWHITE) echo "15" ;;

	*) echo "$1" ;;

	esac
}

# Function that output the given text with given color code
# This function uses tput and printf as the backend
# Arguments:
#   $1: name or code of the color, leave empty to use default color
#   $2...: printf format string and parameters
function clrs
{
	local cname="$1"
	shift

	local fmt="$1"
	shift

	if [ -n "$cname" ]; then
		printf "$(tput setaf "$(color_name_to_code "$cname")")$fmt$(tput sgr0)" "$@"
	else
		printf "$fmt" "$@"
	fi
}

# Internal use only. Print a formatted timestamp
function pr_timestamp
{
	printf "[%12.6f]" "$(script_elapsed_ns)"
}

# Internal use only. Print a formatted message
function pr_core
{
	local color="$1"
	shift

	local fmt="$1"
	shift

	if [ -n "${BANDORI_CLEAN_PR:-}" ]; then
		clrs "$color" "%s $fmt\n" "$(pr_timestamp)" "$@"
	else
		clrs "$color" "%s $fmt (%s)\n" "$(pr_timestamp)" "$@" "$(get_caller 1)"
	fi
}

# Print an error message
# Arguments:
#   $1...: printf format string and parameters
function pr_err
{
	pr_core err "$@"
}

# Print a warning message
# Arguments:
#   $1...: printf format string and parameters
function pr_warn
{
	pr_core warn "$@"
}

# Print an info message
# Arguments:
#   $1...: printf format string and parameters
function pr_info
{
	pr_core info "$@"
}

# Print a success message
# Arguments:
#   $1...: printf format string and parameters
function pr_succ
{
	pr_core succ "$@"
}

# Print a message
# Arguments:
#   $1...: printf format string and parameters
function pr
{
	pr_core "" "$@"
}

# Internal use only. Print a formatted message and ask for user input
function ask_core
{
	local color="$1"
	shift

	local regex="$1"
	shift

	local -n outvar=$1
	shift

	local fmt="$1"
	shift

	local input=""
	local output=""
	local prog=""
	local msg="Please give an input with the format: /^$regex$$/"

	while true; do
		if [ -n "${BANDORI_CLEAN_PR:-}" ]; then
			clrs "$color" "%s $fmt:" "$(pr_timestamp)" "$@"
		else
			clrs "$color" "%s $fmt (%s):" "$(pr_timestamp)" "$@" "$(get_caller 1)"
		fi

		# The whitespace is to prevent empty line assumption that may kill the prompt
		read -rep ' ' input

		case "$regex" in

		# Handling for yes/no/cancel selection
		yn|Yn|yN|ync|Ync|yNc|ynC)
			if [[ "$regex" == *"c"* ]] || [[ "$regex" == *"C"* ]]; then
				prog='print uc("$+{a}") if /^(?:(?<a>[yY])(?:es)?|(?<a>[nN])(?:o)?|(?<a>[cC])(?:ancel|ancle)?)$/'
				msg="Please choose between Yes/No/Cancel"
			else
				prog='print uc("$+{a}") if /^(?:(?<a>[yY])(?:es)?|(?<a>[nN])(?:o)?)$/'
				msg="Please choose between Yes/No"
			fi

			output=$(echo "$input" | perl -ne "$prog")

			# Found input
			if [ -n "$output" ]; then
				outvar="$output"
				return 0
			fi

			# Handle default case
			if [ -z "$input" ]; then
				if [[ "$regex" = *"Y"* ]]; then
					outvar="Y"
					return 0
				elif [[ "$regex" = *"N"* ]]; then
					outvar="N"
					return 0
				elif [[ "$regex" = *"C"* ]]; then
					outvar="C"
					return 0
				fi
			fi
			;;

		# Handling for decimal input
		num|dec)
			output=$(echo "$input" | perl -ne 'print "$1" if /^(\d+)$/')
			msg="Please input a valid decimal number"

			# Found input
			if [ -n "$output" ]; then
				outvar="$output"
				return 0
			fi
			;;

		# Handling for hexadecimal input
		hex)
			output=$(echo "$input" | perl -ne 'print uc("$+{a}") if /^(?:(?:0x|0X)?(?<a>[0-9a-fA-F]+)|(?<a>[0-9a-fA-F]+)[hH]?)$/')
			msg="Please input a valid hexadecimal number"

			# Found input
			if [ -n "$output" ]; then
				outvar="$output"
				return 0
			fi
			;;

		# Handling for path input
		path|epath|fpath|dpath|npath)
			msg="Please input a valid path"

			# Found input
			if [ "$regex" = "epath" ]; then
				msg="Please input a valid existing path"
				if [ -e "$input" ]; then
					outvar="$output"
					return 0
				fi
			elif [ "$regex" = "fpath" ]; then
				msg="Please input a valid file path"
				if [ -f "$input" ]; then
					outvar="$output"
					return 0
				fi
			elif [ "$regex" = "dpath" ]; then
				msg="Please input a valid directory path"
				if [ -d "$input" ]; then
					outvar="$output"
					return 0
				fi
			elif [ "$regex" = "npath" ]; then
				msg="Please input a valid non-existing path"
				if [ ! -e "$input" ]; then
					outvar="$output"
					return 0
				fi
			else
				outvar="$output"
				return 0
			fi
			;;

		# Handling for regex input
		*)
			output=$(echo "$input" | perl -ne 'print ((defined $+{"out"}) ? $+{"out"} : $_) if /^'"$regex"'$/')
			msg="Please give an input with the format: /^$regex$$/"

			# Found input
			if [ -n "$output" ]; then
				outvar="$output"
				return 0
			fi
			;;

		esac

		# Handle wild input
		clrs err "%s     %s\n" "$(pr_timestamp)" "$msg"
	done
}

# Print an error message and ask for user input
# Arguments:
#   $1: user input validation regex or one of the predefined patterns
#   $2: the variable holding the output value
#   $3...: printf format string and parameters
function ask_err
{
	ask_core err "$@"
}

# Print a warning message and ask for user input
# Arguments:
#   $1: user input validation regex or one of the predefined patterns
#   $2: the variable holding the output value
#   $3...: printf format string and parameters
function ask_warn
{
	ask_core warn "$@"
}

# Print an info message and ask for user input
# Arguments:
#   $1: user input validation regex or one of the predefined patterns
#   $2: the variable holding the output value
#   $3...: printf format string and parameters
function ask_info
{
	ask_core info "$@"
}

# Print a success message and ask for user input
# Arguments:
#   $1: user input validation regex or one of the predefined patterns
#   $2: the variable holding the output value
#   $3...: printf format string and parameters
function ask_succ
{
	ask_core succ "$@"
}

# Print a message and ask for user input
# Arguments:
#   $1: user input validation regex or one of the predefined patterns
#   $2: the variable holding the output value
#   $3...: printf format string and parameters
function ask
{
	ask_core "" "$@"
}



#
# Utility
#

# Run the given float point math calculation
# Arguments:
#   $1...: The expression
function math
{
	echo "$@" | bc -l
}

# Throw an exception
# Arguments:
#   $1: The return code
#   $2...: [optional] The error message, in printf format
function throw
{
	local retval="$1"
	shift

	if [ "$#" -ge 1 ]; then
		local fmt="$1"
		shift

		clrs err "%s Unhandled exception was thrown.\n\nError code: %d\nMessage: $fmt\n" "$(pr_timestamp)" "$retval" "$@"
	else
		clrs err "%s Unhandled exception was thrown.\n\nError code: %d\n" "$(pr_timestamp)" "$retval"
	fi

	return "$retval"
}



#
# Debugging
#

# Print the full call stack at the point of execution
# This function can optionally skip n frames at the top of stack
# Arguments:
#   $1: [optional] frames to skip (default 0)
#   $2: [optional, internal] alternative line number, used to workaround the signal handler bug (default 1)
function get_stacktrace
{
	local i=0
	local line
	local file
	local func
	local altln=""

	if [ "$#" -ge 1 ]; then
		i=$((i+"$1"))
	fi

	if [ "$#" -ge 2 ]; then
		altln="$2"
	fi

	while read -r line func file < <(caller "$i"); do
		# The interrupt handler is always the first caller
		# So we should replace the line number of the second frame
		if [ -n "$altln" ] && [ "$i" -eq 1 ]; then
			line="$altln"
		fi

		# Not sure why but caller sometimes behaves funky that we need to manually break
		if [ -z "$file" ] && [ -z "$func" ]; then
			break
		fi

		echo "  at $func() in $file:$line"

		# When $i == 0, double parentheses will return 1
		((i++)) || true
	done
}

# Print the location at the point of execution
# Arguments:
#   $1: [optional] frames to skip (default 0)
function get_caller
{
	local i=0
	local line
	local file
	local func

	if [ "$#" -ge 1 ]; then
		i=$((i+"$1"))
	fi

	read -r line func file < <(caller "$i")

	echo "at $func() in $file:$line"
}



#
# Standard Signal Handler
#

# Standard EXIT signal handler
# It does nothing other than display the return code and exit
function sig_exit
{
	local retval="$1"

	if [ "$BANDORI_SCRIPT_NESTING" -eq 1 ] && [ -z "${BANDORI_QUIET:-}" ]; then
		if [ "$retval" -eq 0 ]; then
			clrs succ "\n%s Execution Succeed\n" "$(pr_timestamp)"
		else
			clrs err "\n%s Execution Failed\n" "$(pr_timestamp)"
		fi
	fi

	fini_bash
	exit "$retval"
}

# Standard ERR signal handler
# It displays the error code and stacktrace, and then exit
function sig_err
{
	local retval="$1"

	if [ "$retval" -ne 0 ]; then
		# We use a file to build a monolithic stack trace when scripts nests
		get_stacktrace 1 >> "$BANDORI_SCRIPT_TRACE"

		if [ "$BANDORI_SCRIPT_NESTING" -eq 1 ]; then
			clrs err "\n%s Unhandled error (code: $retval) was thrown.\n\nStack Trace:\n%s\n" "$(pr_timestamp)" "$(cat "$BANDORI_SCRIPT_TRACE")"
		fi
	fi

	# No need to call clean_bash here, we will still go through the `sig_exit`
	exit "$retval"
}



#
# Main Context Injection
#

# This is the standard setting for all our scripts
trap 'sig_err "$?"' ERR
trap 'sig_exit "$?"' EXIT
init_bash "$0"

#ifndef UTILS_H
#define UTILS_H

// ANSI Escape code
// reference: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
// ESC: "\033"
#define ANSI_RESET_COLOR				"\033[0m"
// Foreground colors
#define ANSI_BOLD_BLACK_FG_COLOR		"\033[1;30m"
#define ANSI_BLACK_FG_COLOR				"\033[0;30m"
#define ANSI_BRIGHT_BLACK_FG_COLOR		"\033[0;90m"
#define ANSI_BOLD_RED_FG_COLOR			"\033[1;31m"
#define ANSI_RED_FG_COLOR				"\033[0;31m"
#define ANSI_BRIGHT_RED_FG_COLOR		"\033[0;91m"
#define ANSI_BOLD_GREEN_FG_COLOR		"\033[1;32m"
#define ANSI_GREEN_FG_COLOR				"\033[0;32m"
#define ANSI_BRIGHT_GREEN_FG_COLOR		"\033[0;92m"
#define ANSI_BOLD_YELLOW_FG_COLOR		"\033[1;33m"
#define ANSI_YELLOW_FG_COLOR			"\033[0;33m"
#define ANSI_BRIGHT_YELLOW_FG_COLOR		"\033[0;93m"
#define ANSI_BOLD_BLUE_FG_COLOR			"\033[1;34m"
#define ANSI_BLUE_FG_COLOR				"\033[0;34m"
#define ANSI_BRIGHT_BLUE_FG_COLOR		"\033[0;94m"
#define ANSI_BOLD_MAGENTA_FG_COLOR		"\033[1;35m"
#define ANSI_MAGENTA_FG_COLOR			"\033[0;35m"
#define ANSI_BRIGHT_MAGENTA_FG_COLOR	"\033[0;95m"
#define ANSI_BOLD_CYAN_FG_COLOR			"\033[1;36m"
#define ANSI_CYAN_FG_COLOR				"\033[0;36m"
#define ANSI_BRIGHT_CYAN_FG_COLOR		"\033[0;96m"
#define ANSI_BOLD_WHITE_FG_COLOR		"\033[1;37m"
#define ANSI_WHITE_FG_COLOR				"\033[0;37m"
#define ANSI_BRIGHT_WHITE_FG_COLOR		"\033[0;97m"

// Background colors
#define ANSI_BLACK_BG_COLOR				"\033[40m"
#define ANSI_RED_BG_COLOR				"\033[41m"
#define ANSI_GREEN_BG_COLOR				"\033[42m"
#define ANSI_YELLOW_BG_COLOR			"\033[43m"
#define ANSI_BLUE_BG_COLOR				"\033[44m"
#define ANSI_MAGENTA_BG_COLOR			"\033[45m"
#define ANSI_CYAN_BG_COLOR				"\033[46m"
#define ANSI_WHITE_BG_COLOR				"\033[47m"

#define ANSI_BRIGHT_BLACK_BG_COLOR		"\033[100m"
#define ANSI_BRIGHT_RED_BG_COLOR		"\033[101m"
#define ANSI_BRIGHT_GREEN_BG_COLOR		"\033[102m"
#define ANSI_BRIGHT_YELLOW_BG_COLOR		"\033[103m"
#define ANSI_BRIGHT_BLUE_BG_COLOR		"\033[104m"
#define ANSI_BRIGHT_MAGENTA_BG_COLOR	"\033[105m"
#define ANSI_BRIGHT_CYAN_BG_COLOR		"\033[106m"
#define ANSI_BRIGHT_WHITE_BG_COLOR		"\033[107m"


// Usage e.g.:
/*
int main (int argc, char const *argv[]) {

	printf(ANSI_RED_FG_COLOR     "This text is RED!"     ANSI_RESET_COLOR "\n");
	printf(ANSI_YELLOW_FG_COLOR ANSI_BRIGHT_CYAN_BG_COLOR   "This text is YELLOW with CYAN background color"   ANSI_RESET_COLOR "\n");

*/

#define BOOL_TO_ONOFF(b) (b ? "on" : "off")
#define NIBBLE_TO_HEX_CHAR(i) ((i <= 9) ? ('0' + i) : ('A' - 10 + i))
#define HIGH_NIBBLE(i) ((i >> 4) & 0x0F)
#define LOW_NIBBLE(i) (i & 0x0F)
#define HEX_CHAR_TO_NIBBLE(c) ((c >= 'A') ? (c - 'A' + 0x0A) : (c - '0'))
#define HEX_PAIR_TO_BYTE(h, l) ((HEX_CHAR_TO_NIBBLE(h) << 4) + HEX_CHAR_TO_NIBBLE(l))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define NIBBLE2ASCII(nibble) (((nibble < 0x0A) ? (nibble + '0') : (nibble + 0x57)))

void charArrayToHexArray(const char* in, char* out) ;
uint8_t charToInt(uint8_t in) ;
uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt) ;
bool isHexFormat(uint8_t val) ;
bool isHexLowerCase(uint8_t val) ;
bool isHexUpperCase(uint8_t val) ;
uint8_t convertHexToUpperCase(uint8_t in) ;
uint8_t convertHexToLowerCase(uint8_t in) ;

void print_array (uint8_t *array, uint8_t length) ;

#endif // UTILS_H
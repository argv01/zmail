#include "bitmaps/lpr.xbm"
#include "print.h"

ZcIcon lpr_icon = {
    "printer_icon", 0, lpr_width, lpr_height, lpr_bits
};

char *print_header_choices[] = {
    "standard",
    "all_headers",
    "body_only"
};

const unsigned int print_header_count = XtNumber(print_header_choices);

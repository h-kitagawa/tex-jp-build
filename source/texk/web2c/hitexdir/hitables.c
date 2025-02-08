#include "hibasetypes.h"
#include "hiformat.h"

const char *content_name[32]={"list", "param", "range", "xdimen", "adjust", "glyph", "kern", "glue", "ligature", "disc", "language", "rule", "image", "leaders", "baseline", "hbox", "vbox", "par", "math", "table", "item", "hset", "vset", "hpack", "vpack", "stream", "page", "link", "color", "undefined1", "undefined2", "penalty"};

const char *definition_name[32]={"list", "param", "range", "xdimen", "adjust", "font", "dimen", "glue", "ligature", "disc", "language", "rule", "image", "leaders", "baseline", "hbox", "vbox", "par", "math", "table", "item", "hset", "vset", "hpack", "vpack", "stream", "page", "label", "color", "undefined1", "undefined2", "int"};

int max_outline=-1;

int32_t int_defaults[MAX_INT_DEFAULT+1]={0, 100, 200, 10, 50, 50, 150, 150, 50, 100, 10000, 0, 0, 10000, 5000, 10000, 0, 720, 4, 7, 1776, 1, 20000};

Dimen dimen_defaults[MAX_DIMEN_DEFAULT+1]={0x0, 0x1d5c147, 0x28333f7, 0x0, 0x0, 0x38000, 0x0, 0x0, 0xa0000, 0xa0000};

Glue glue_defaults[MAX_GLUE_DEFAULT+1]={
{{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0x0, 0.000000, 0.000000},{1.000000, 1},{0.000000, 0}},
{{0x0, 0.000000, 0.000000},{1.000000, 2},{0.000000, 0}},
{{0x10000, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0xc0000, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0xc0000, 0.000000, 0.000000},{3.000000, 0},{9.000000, 0}},
{{0xc0000, 0.000000, 0.000000},{3.000000, 0},{9.000000, 0}},
{{0x0, 0.000000, 0.000000},{3.000000, 0},{0.000000, 0}},
{{0x70000, 0.000000, 0.000000},{3.000000, 0},{4.000000, 0}},
{{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0xa0000, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0x80000, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}},
{{0x0, 0.000000, 0.000000},{1.000000, 1},{0.000000, 0}},
};

Xdimen xdimen_defaults[MAX_XDIMEN_DEFAULT+1]={{0x0, 0.0, 0.0}, {0x0, 1.0, 0.0}, {0x0, 0.0, 1.0}};

Baseline baseline_defaults[MAX_BASELINE_DEFAULT+1]={{{{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}}, {{0x0, 0.000000, 0.000000},{0.000000, 0},{0.000000, 0}}, 0x0}};

Label label_defaults[MAX_LABEL_DEFAULT+1]={{0,0,LABEL_TOP,true,0,0}};

ColorSet  color_defaults[MAX_COLOR_DEFAULT+1]=
{{0x000000FF, 0xFFFFFF00,
  0xEE0000FF, 0xFFFFFF00,
  0x00EE00FF, 0xFFFFFF00,
  0xFFFFFFFF, 0x00000000,  0xFF1111FF, 0x00000000,
  0x11FF11FF, 0x00000000},
 {0x0000EEFF, 0xFFFFFF00,
  0xEE0000FF, 0xFFFFFF00,
  0x00EE00FF, 0xFFFFFF00,
  0x1111FFFF, 0x00000000,
  0xFF1111FF, 0x00000000,
  0x11FF11FF, 0x00000000
}};

int max_fixed[32]= {0, 0, 0, 2, 65536, -1, 0, 2, -1, -1, -1, -1, -1, -1, 0, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 0, 0, -1, -1, 65536, 65536, 0};

int max_default[32]= {0, 0, 0, 2, -1, -1, 9, 14, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 1, -1, -1, 22};

int max_ref[32]= {0, 0, 0, 2, -1, -1, 9, 14, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 1, -1, -1, 22};

signed char hnode_size[0x100]= {
0,0,0,0,0,0,0,0, /* list */
0,0,0,0,0,0,0,0, /* param */
0,0,0,0,0,0,0,0, /* range */
0,6,6,10,6,10,10,14, /* xdimen */
0,-4*1+0,0,0,0,0,0,0, /* adjust */
0,4,5,6,7,0,0,0, /* glyph */
3,3,6,-4*1+0,3,3,6,-4*1+0, /* kern */
3,6,6,10,6,10,10,-4*9+0, /* glue */
3,4,5,6,7,8,9,-4*2+0, /* ligature */
3,0,-4*1+0,-4*1+1,3,0,-4*2+0,-4*2+1, /* disc */
3,2,2,2,2,2,2,2, /* language */
3,6,6,10,6,10,10,14, /* rule */
3,-4*11+0,-4*11+0,-4*11+0,-4*9+0,-4*8+1,-4*8+1,-4*7+2, /* image */
3,-4*1+0,-4*1+0,-4*1+0,0,-4*1+1,-4*1+1,-4*1+1, /* leaders */
3,6,-4*1+0,-4*5+0,-4*1+0,-4*5+0,-4*1+1,-4*5+1, /* baseline */
-4*9+0,-4*13+0,-4*13+0,-4*17+0,-4*14+0,-4*18+0,-4*18+0,-4*22+0, /* hbox */
-4*9+0,-4*13+0,-4*13+0,-4*17+0,-4*14+0,-4*18+0,-4*18+0,-4*22+0, /* vbox */
-4*3+0,0,-4*2+1,0,-4*2+1,0,-4*1+2,0, /* par */
-4*2+0,-4*2+1,-4*2+1,2,-4*1+1,-4*1+2,-4*1+2,2, /* math */
-4*2+1,-4*2+1,-4*2+1,-4*2+1,-4*1+2,-4*1+2,-4*1+2,-4*1+2, /* table */
-4*1+0,-4*1+0,-4*1+0,-4*1+0,-4*1+0,-4*1+0,-4*1+0,-4*2+0, /* item */
-4*18+0,-4*22+0,-4*22+0,-4*26+0,-4*17+1,-4*21+1,-4*21+1,-4*25+1, /* hset */
-4*18+0,-4*22+0,-4*22+0,-4*26+0,-4*17+1,-4*21+1,-4*21+1,-4*25+1, /* vset */
-4*2+0,-4*2+0,-4*6+0,-4*6+0,-4*1+1,-4*1+1,-4*5+1,-4*5+1, /* hpack */
-4*6+0,-4*6+0,-4*10+0,-4*10+0,-4*5+1,-4*5+1,-4*9+1,-4*9+1, /* vpack */
-4*3+0,0,-4*2+1,0,3,0,0,0, /* stream */
0,0,0,0,0,0,0,0, /* page */
3,4,3,4,4,5,4,5, /* link */
3,0,0,0,0,0,0,0, /* color */
0,0,0,0,0,0,0,0, /* undefined1 */
0,0,0,0,0,0,0,0, /* undefined2 */
3,3,4,6,0,0,0,0}; /* penalty */


uint8_t content_known[32]= {
0x00, /* list */
0x00, /* param */
0x00, /* range */
0xFE, /* xdimen */
0x02, /* adjust */
0x1E, /* glyph */
0xFF, /* kern */
0xFF, /* glue */
0xFF, /* ligature */
0xDD, /* disc */
0xFF, /* language */
0xFF, /* rule */
0xFF, /* image */
0xEF, /* leaders */
0xFF, /* baseline */
0xFF, /* hbox */
0xFF, /* vbox */
0x55, /* par */
0xFF, /* math */
0xFF, /* table */
0xFF, /* item */
0xFF, /* hset */
0xFF, /* vset */
0xFF, /* hpack */
0xFF, /* vpack */
0x15, /* stream */
0x00, /* page */
0xFF, /* link */
0x01, /* color */
0x00, /* undefined1 */
0x00, /* undefined2 */
0x0F}; /* penalty */


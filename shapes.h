#pragma once

struct glyph{
    char code,
    std::vector<uint8_t> draw;
};

using VectorFont = std::vector<glyph>;

void drawText()

const VectorFont incitti={
	'a':[5,0,2,4,2,4,2,4,6,4,6,0,6,0,6,0,3,0,3,4,3],
	'b':[4,0,0,0,6,0,6,4,6,4,6,4,2,4,2,0,2],
	'c':[3,0,2,0,6,0,6,4,6,0,2,4,2],
	'd':[4,4,0,4,6,4,6,0,6,0,6,0,2,0,2,4,2],
	'e':[5,4,6,0,6,0,6,0,2,0,2,4,2,4,2,4,3,4,3,1,3],
	'f':[3,4,0,2,0,2,0,2,6,1,2,3,2],
	'g':[5,0,7,4,7,4,7,4,2,4,2,0,2,0,2,0,6,0,6,4,6],
	'h':[3,0,0,0,6,0,2,4,2,4,2,4,6],
	'i':[2,2,2,2,6,2,1,2,0],
	'j':[4,3,1,3,1,3,2,3,7,3,7,0,7,0,7,0,5],
	'k':[3,0,0,0,6,0,4,3,2,0,4,3,6],
	'l':[1,2,0,2,6],
	'm':[4,0,6,0,2,0,2,4,2,2,2,2,4,4,2,4,6],
	'n':[4,0,2,0,6,0,3,1,2,1,2,4,2,4,2,4,6],
	'o':[4,0,2,0,6,0,6,4,6,4,6,4,2,4,2,0,2],
	'p':[4,0,2,0,7,0,2,4,2,0,6,4,6,4,2,4,6],
	'q':[4,0,2,0,6,0,6,4,6,4,2,4,7,0,2,4,2],
	'r':[3,0,2,0,6,0,3,1,2,1,2,4,2],
	's':[5,4,2,0,2,0,2,0,4,0,4,4,4,4,4,4,6,4,6,0,6],
	't':[2,0,2,4,2,2,0,2,6],
	'u':[3,0,2,0,6,0,6,4,6,4,6,4,2],
	'v':[2,0,2,2,6,2,6,4,2],
	'w':[4,0,2,1,6,1,6,2,4,2,4,3,6,3,6,4,2],
	'x':[2,0,2,4,6,0,6,4,2],
	'y':[2,0,2,2,5,4,2,1,7],
	'z':[3,0,2,4,2,4,2,0,6,0,6,4,6],
	'0':[4,0,0,0,6,0,6,4,6,4,6,4,0,4,0,0,0],
	'1':[2,1,1,2,0,2,0,2,6],
	'2':[5,0,0,4,0,4,0,4,3,4,3,0,3,0,3,0,6,0,6,4,6],
	'3':[4,0,0,4,0,4,0,4,6,4,6,0,6,2,3,4,3],
	'4':[3,0,0,0,3,0,3,4,3,4,0,4,6],
	'5':[5,0,0,4,0,0,0,0,3,0,3,4,3,4,3,4,6,0,6,4,6],
	'6':[4,0,0,0,6,0,3,4,3,4,3,4,6,0,6,4,6],
	'7':[2,0,0,4,0,4,0,4,6],
	'8':[5,0,0,0,6,0,6,4,6,4,6,4,0,4,0,0,0,0,3,4,3],
	'9':[4,0,0,4,0,4,0,4,6,0,0,0,3,0,3,4,3],
	'A':[5,2,0,0,2,2,0,4,2,0,2,0,6,4,2,4,6,0,3,4,3],
	'B':[6,0,0,0,6,0,6,4,6,4,6,4,3,4,3,0,3,0,0,3,0,3,0,3,3],
	'C':[3,0,0,0,6,0,6,4,6,0,0,4,0],
	'D':[6,0,0,0,6,0,6,2,6,2,6,4,4,4,4,4,2,4,2,2,0,2,0,0,0],
	'E':[4,0,0,0,6,0,6,4,6,0,0,4,0,0,3,2,3],
	'F':[3,0,0,0,6,0,0,4,0,0,3,2,3],
	'G':[5,0,0,0,6,0,6,4,6,0,0,4,0,4,6,4,3,4,3,2,3],
	'H':[3,0,0,0,6,0,3,4,3,4,0,4,6],
	'I':[3,0,0,4,0,0,6,4,6,2,0,2,6],
	'J':[4,3,0,4,0,4,0,4,6,4,6,2,6,2,6,1,4],
	'K':[4,0,0,0,6,0,3,2,3,2,3,4,0,2,3,4,6],
	'L':[2,0,0,0,6,0,6,4,6],
	'M':[4,0,0,0,6,0,0,2,3,2,3,4,0,4,0,4,6],
	'N':[3,0,0,0,6,0,0,4,6,4,6,4,0],
	'O':[4,0,0,0,6,0,6,4,6,4,6,4,0,4,0,0,0],
	'P':[4,0,0,0,6,0,0,4,0,0,3,4,3,4,3,4,0],
	'Q':[6,0,0,0,6,0,6,2,6,2,6,4,4,4,4,4,0,4,0,0,0,4,6,2,4],
	'R':[5,0,0,0,6,0,0,4,0,0,3,4,3,4,0,4,3,2,3,4,6],
	'S':[5,0,0,0,3,0,3,4,3,4,3,4,6,4,6,0,6,0,0,4,0],
	'T':[2,0,0,4,0,2,0,2,6],
	'U':[3,0,0,0,6,0,6,4,6,4,6,4,0],
	'V':[4,0,0,0,3,0,3,2,6,2,6,4,3,4,3,4,0],
	'W':[4,0,0,1,6,1,6,2,4,2,4,3,6,3,6,4,0],
	'X':[2,0,0,4,6,0,6,4,0],
	'Y':[3,0,0,2,3,2,3,4,0,2,3,2,6],
	'Z':[3,0,0,4,0,4,0,0,6,0,6,4,6],
	'/':[1,4,0,0,6],
	'"':[2,1,0,1,2,3,0,3,2],
	'\\':[1,0,0,4,6],
	'\'':[1,2,1,2,3],
	'`':[1,1,1,2,2],
	'!':[2,2,0,2,4,2,5,2,6],
	';':[2,2,1,2,3,2,5,1,6],
	':':[2,2,1,2,2,2,4,2,5],
	'#':[4,1,0,1,6,3,0,3,6,0,2,4,2,0,4,4,4],
	'$':[6,0,1,0,3,0,3,4,3,4,3,4,5,4,5,0,5,0,1,4,1,2,0,2,6],
	'%':[3,3,0,1,6,1,1,1,2,3,4,3,5],
	'&':[6,0,1,4,5,0,1,1,0,1,0,2,1,2,1,0,4,0,4,2,6,2,6,4,4],
	'*':[3,1,1,3,5,1,5,3,1,0,3,4,3],
	'+':[2,2,2,2,4,1,3,3,3],
	',':[1,2,5,1,6],
	'-':[1,1,3,3,3],
	'.':[1,2,5,2,6],
	'^':[2,1,2,2,0,2,0,3,2],
	'_':[1,0,7,4,7],
	'=':[2,1,2,3,2,1,4,3,4],
	'?':[6,1,1,1,0,1,0,3,0,3,0,3,2,3,2,2,3,2,3,2,4,2,5,2,6],
	'@':[6,2,2,2,4,2,4,4,4,4,4,4,0,4,0,0,0,0,0,0,6,0,6,4,6],
	'(':[3,3,0,1,2,1,2,1,4,1,4,3,6],
	')':[3,1,0,3,2,3,2,3,4,3,4,1,6],
	'[':[3,3,0,1,0,1,0,1,6,1,6,3,6],
	',':[3,1,0,3,0,3,0,3,6,3,6,1,6],
	'':[4,3,0,2,0,2,0,2,6,2,6,3,6,1,3,2,3],
	'|':[1,2,0,2,6],
	'}':[4,1,0,2,0,2,0,2,6,2,6,1,6,2,3,3,3],
	'~':[5,0,3,0,1,0,1,2,1,2,1,2,3,2,3,4,3,4,3,4,1],
	'<':[2,4,0,1,3,1,3,4,6],
	'>':[2,1,0,4,3,4,3,1,6],
	//escape
	'27':[2,1,1,4,3,4,1,1,3],
	'10':[5,0,0,0,24,0,24,8,24,8,24,8,11,8,11,5,11,5,11,5,0],
	'11':[3,1,0,1,10,1,10,7,10,7,10,7,0],
	'12':[7,3,0,3,11,3,11,0,11,0,11,0,24,0,24,8,24,8,24,8,11,8,11,5,11,5,11,5,0],
	'13':[3,1,0,1,10,1,10,7,10,7,10,7,0],
	'14':[5,3,0,3,11,3,11,0,11,0,11,0,24,0,24,8,24,8,24,8,0],
	'15':[5,0,0,0,24,0,24,8,24,8,24,8,11,8,11,5,11,5,11,5,0],
	'16':[3,1,0,1,10,1,10,7,10,7,10,7,0],
	'17':[7,3,0,3,11,3,11,0,11,0,11,0,24,0,24,8,24,8,24,8,11,8,11,5,11,5,11,5,0],
	'18':[3,1,0,1,10,1,10,7,10,7,10,7,0],
	'19':[7,3,0,3,11,3,11,0,11,0,11,0,24,0,24,8,24,8,24,8,11,8,11,5,11,5,11,5,0],
	'20':[3,1,0,1,10,1,10,7,10,7,10,7,0],
	'21':[5,3,0,3,11,3,11,0,11,0,11,0,24,0,24,8,24,8,24,8,0]
};

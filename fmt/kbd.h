7100 // PC keyboard interface constants
7101 
7102 #define KBSTATP         0x64    // kbd controller status port(I)
7103 #define KBS_DIB         0x01    // kbd data in buffer
7104 #define KBDATAP         0x60    // kbd data port(I)
7105 
7106 #define NO              0
7107 
7108 #define SHIFT           (1<<0)
7109 #define CTL             (1<<1)
7110 #define ALT             (1<<2)
7111 
7112 #define CAPSLOCK        (1<<3)
7113 #define NUMLOCK         (1<<4)
7114 #define SCROLLLOCK      (1<<5)
7115 
7116 #define E0ESC           (1<<6)
7117 
7118 // Special keycodes
7119 #define KEY_HOME        0xE0
7120 #define KEY_END         0xE1
7121 #define KEY_UP          0xE2
7122 #define KEY_DN          0xE3
7123 #define KEY_LF          0xE4
7124 #define KEY_RT          0xE5
7125 #define KEY_PGUP        0xE6
7126 #define KEY_PGDN        0xE7
7127 #define KEY_INS         0xE8
7128 #define KEY_DEL         0xE9
7129 
7130 // C('A') == Control-A
7131 #define C(x) (x - '@')
7132 
7133 static uchar shiftcode[256] =
7134 {
7135   [0x1D] CTL,
7136   [0x2A] SHIFT,
7137   [0x36] SHIFT,
7138   [0x38] ALT,
7139   [0x9D] CTL,
7140   [0xB8] ALT
7141 };
7142 
7143 static uchar togglecode[256] =
7144 {
7145   [0x3A] CAPSLOCK,
7146   [0x45] NUMLOCK,
7147   [0x46] SCROLLLOCK
7148 };
7149 
7150 static uchar normalmap[256] =
7151 {
7152   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7153   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7154   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7155   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7156   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7157   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7158   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7159   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7160   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7161   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7162   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7163   [0x9C] '\n',      // KP_Enter
7164   [0xB5] '/',       // KP_Div
7165   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7166   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7167   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7168   [0x97] KEY_HOME,  [0xCF] KEY_END,
7169   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7170 };
7171 
7172 static uchar shiftmap[256] =
7173 {
7174   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7175   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7176   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7177   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7178   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7179   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7180   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7181   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7182   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7183   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7184   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7185   [0x9C] '\n',      // KP_Enter
7186   [0xB5] '/',       // KP_Div
7187   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7188   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7189   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7190   [0x97] KEY_HOME,  [0xCF] KEY_END,
7191   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7192 };
7193 
7194 
7195 
7196 
7197 
7198 
7199 
7200 static uchar ctlmap[256] =
7201 {
7202   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7203   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7204   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7205   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7206   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7207   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7208   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7209   [0x9C] '\r',      // KP_Enter
7210   [0xB5] C('/'),    // KP_Div
7211   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7212   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7213   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7214   [0x97] KEY_HOME,  [0xCF] KEY_END,
7215   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7216 };
7217 
7218 
7219 
7220 
7221 
7222 
7223 
7224 
7225 
7226 
7227 
7228 
7229 
7230 
7231 
7232 
7233 
7234 
7235 
7236 
7237 
7238 
7239 
7240 
7241 
7242 
7243 
7244 
7245 
7246 
7247 
7248 
7249 

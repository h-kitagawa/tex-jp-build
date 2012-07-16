% This is a change file for PLtoTF
%
% (07/18/2006) ST PLtoTF p1.8 (3.5, Web2c 7.2)
% (11/13/2000) KN PLtoTF p1.4 (3.5, Web2c 7.2)
% (03/27/1998) KN PLtoTF p1.3 (3.5, Web2c 7.2)
%
@x [0] l.52 - pTeX:
\def\title{PL$\,$\lowercase{to}$\,$TF changes for C}
@y
\def\title{PL$\,$\lowercase{to}$\,$TF changes for C, and for KANJI}
@z

@x [2] l.69 - pTeX:
@d my_name=='pltotf'
@d banner=='This is PLtoTF, Version 3.5' {printed when the program starts}
@y
@d my_name=='ppltotf'
@d banner=='This is pPLtoTF, Version 3.5-p1.8'
  {printed when the program starts}
@z

@x
  parse_arguments;
@y
  init_kanji;
  parse_arguments;
@z

@x [6] l.140 - pTeX:
  print_ln (version_string);
@y
  print_ln (version_string);
  print_ln ('process kanji code is ', conststringcast(get_enc_string), '.');
@z

@x [18] l.495 - pTeX:
@!xord:array[char] of ASCII_code; {conversion table}
@y
@!xord:array[char] of ASCII_code; {conversion table}
@!xchr:array[char] of byte; {specifiles conversion of output character}
@z

@x [19] l.506 - pTeX:
for k:=first_ord to last_ord do xord[chr(k)]:=invalid_code;
@y
for k:=0 to @'37 do xchr[k]:='?';
for k:=@'40 to 255 do xchr[k]:=k;
for k:=first_ord to last_ord do xord[chr(k)]:=invalid_code;
@z

@x [27] l.587 - pTeX: to convert putc of web2c
for k:=1 to loc do print(buffer[k]); {print the characters already scanned}
@y
for k:=1 to loc do print(xchr[buffer[k]]);
  {print the characters already scanned}
@z

@x [27] l.591 - pTeX: to convert putc of web2c
for k:=loc+1 to limit do print(buffer[k]); {print the characters yet unseen}
@y
for k:=loc+1 to limit do print(xchr[buffer[k]]);
  {print the characters yet unseen}
@z

@x [28] l.619 - pTeX:
else  begin while (limit<buf_size-2)and(not eoln(pl_file)) do
    begin incr(limit); read(pl_file,buffer[limit]);
    end;
@y
else  begin limit:=input_line2(pl_file,ustringcast(buffer),limit+1,buf_size-1)-1;
@z

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% [28] This fixes a bug in the original. If get_byte is reading a
%      number at the end of a line and the next line has a number
%      at the beginning (possibly preceded by some spaces!!) these
%      two numbers are run together.
%      This bug may be found in other routines so...
%      Fix: add some (more?) space at the end of each line, in fill_buffer.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% @x [28] l.622 - pTeX:
%   buffer[limit+1]:=' '; right_ln:=eoln(pl_file);
%   if left_ln then @<Set |loc| to the number of leading blanks in
%     the buffer, and check the indentation@>;
%   end;
% end;
% @y
%   buffer[limit+1]:=' '; right_ln:=eoln(pl_file);
%   if right_ln then begin incr(limit); buffer[limit+1]:=' ';
%     end;
%   if left_ln then @<Set |loc| to the number of leading blanks in
%     the buffer, and check the indentation@>;
%   end;
% end;
% @z

@x [36] l.754 - pTeX: May have to increase some numbers to fit new commands
@d max_name_index=88 {upper bound on the number of keywords}
@d max_letters=600 {upper bound on the total length of all keywords}
@y
@d max_name_index=97 {upper bound on the number of keywords}
@d max_letters=700 {upper bound on the total length of all keywords}
@z

@x [44] l.839 - pTeX: Add kanji related codes
@d character_code=12
@y
@d character_code=12
@d type_code=13            {|TYPE| property}
@d glue_kern_code=14       {|GLUEKRN| property}
@d chars_in_type_code=15   {|CHARSINTYPE| property}
@d dir_code=16             {|DIRECTION| property}
@z

@x [44] l.856 - pTeX:
@d lig_code=74
@y
@d lig_code=74
@d glue_code=75            {|GLUE| property}
@#
@d undefined=0  {not decided file format yet}
@d tfm_format=1 {\.{TFM} file format}
@d jfm_or_vfm=2 {Yoko or Tate \.{JFM} file format}
@d jfm_format=3 {Yoko-kumi \.{JFM} file format}
@d vfm_format=4 {Tate-kumi \.{JFM} file format}
@z

@x [84] l.1542 - pTeX: Change valid property code.
if cur_code=comment_code then skip_to_end_of_item
else if cur_code>character_code then
  flush_error('This property name doesn''t belong on the outer level')
@.This property name doesn't belong...@>
@y
if cur_code=comment_code then skip_to_end_of_item
else if (cur_code>dir_code)or
        ((file_format=tfm_format)and(cur_code>character_code)) then
  flush_error('This property name doesn''t belong on the outer level')
@.This property name doesn't belong...@>
@z

@x [85] l.1565 - pTeX: Added some property codes.
character_code: read_char_info;
@y
character_code: read_char_info;
type_code: read_kanji_info;
glue_kern_code: read_glue_kern;
chars_in_type_code: read_chars_in_type;
dir_code: read_direction;
@z

@x [110] l.1915 - pTeX: there are no charlists in kanji format files.
for c:=0 to 255 do
  @<Make sure that |c| is not the largest element of a charlist cycle@>;
@y
if file_format=tfm_format then
  for c:=0 to 255 do
    @<Make sure that |c| is not the largest element of a charlist cycle@>;
@z

@x [120] l.2037 - pTeX: when checking glue_kern prog check glues as well
    begin if lig_exam<>bchar then
      check_existence(lig_exam)('LIG character examined by');
@.LIG character examined...@>
    check_existence(lig_gen)('LIG character generated by');
@.LIG character generated...@>
    if lig_gen>=128 then if(c<128)or(c=256) then
      if(lig_exam<128)or(lig_exam=bchar) then seven_unsafe:=true;
    end
@y
  begin if file_format=tfm_format then
    begin if lig_exam<>bchar then
      check_existence(lig_exam)('LIG character examined by');
@.LIG character examined...@>
    check_existence(lig_gen)('LIG character generated by');
@.LIG character generated...@>
    if lig_gen>=128 then if(c<128)or(c=256) then
      if(lig_exam<128)or(lig_exam=bchar) then seven_unsafe:=true;
    end
  else check_existence(lig_exam)('GLUE character generated by');
  end
@z

@x [126] l.2178 - pTeX: Fix up output of bytes.
@<Doublecheck...@>=
if nl>0 then for lig_ptr:=0 to nl-1 do
  if lig_kern[lig_ptr].b2<kern_flag then
    begin if lig_kern[lig_ptr].b0<255 then
      begin double_check_lig(b1)('LIG step'); double_check_lig(b3)('LIG step');
      end;
    end
  else double_check_lig(b1)('KRN step');
@y
@<Doublecheck...@>=
@z

@x [128] l.2207 - pTeX: Decide the |file_format|.
@<Do the output@>=
@y
@<Do the output@>=
case file_format of
tfm_format: do_nothing;
undefined,jfm_or_vfm: begin file_format:=jfm_format;
  print_ln('Input file is in kanji YOKO-kumi format.');
  end;
jfm_format: print_ln('Input file is in kanji YOKO-kumi format.');
vfm_format: print_ln('Input file is in kanji TATE-kumi format.');
end;
@z

@x [128] l.2211 - pTeX: Output kanji character
@<Output the character info@>;
@y
if file_format<>tfm_format then @<Output the kanji character type info@>;
@<Output the character info@>;
@z

@x [128] l.2213 - pTeX: Output glue/kern programs
@<Output the ligature/kern program@>;
@y
@<Output the ligature/kern program@>;
if (file_format<>tfm_format)and(ng>0) then
  for krn_ptr:=0 to ng-1 do
    begin out_scaled(glue[3*krn_ptr+0]);
    out_scaled(glue[3*krn_ptr+1]);
    out_scaled(glue[3*krn_ptr+2]);
    end;
@z

@x [130] l.2238 - pTeX:
not_found:=true; bc:=0;
while not_found do
  if (char_wd[bc]>0)or(bc=255) then not_found:=false
  else incr(bc);
not_found:=true; ec:=255;
while not_found do
  if (char_wd[ec]>0)or(ec=0) then not_found:=false
  else decr(ec);
if bc>ec then bc:=1;
@y
if file_format<>tfm_format then
  begin bc:=0; ec:=0; nt:=1;
  for kanji_type_index:=0 to max_kanji do
    begin if kanji_type[kanji_type_index]>0 then incr(nt);
    if kanji_type[kanji_type_index]>ec then ec:=kanji_type[kanji_type_index];
    end;
  end
else  begin not_found:=true; bc:=0;
  while not_found do
    if (char_wd[bc]>0)or(bc=255) then not_found:=false
    else incr(bc);
  not_found:=true; ec:=255;
  while not_found do
    if (char_wd[ec]>0)or(ec=0) then not_found:=false
    else decr(ec);
  if bc>ec then bc:=1;
  end;
@z

@x [130] l.2250 - pTeX:
lf:=6+lh+(ec-bc+1)+memory[width]+memory[height]+memory[depth]+
memory[italic]+nl+lk_offset+nk+ne+np;
@y
if file_format<>tfm_format then
  lf:=7+nt+lh+(ec-bc+1)+memory[width]+memory[height]+memory[depth]+
    memory[italic]+nl+lk_offset+nk+3*ng+np
else
  lf:=6+lh+(ec-bc+1)+memory[width]+memory[height]+memory[depth]+
    memory[italic]+nl+lk_offset+nk+ne+np;
@z

@x [131] l.2256 - pTeX:
out_size(lf); out_size(lh); out_size(bc); out_size(ec);
out_size(memory[width]); out_size(memory[height]);
out_size(memory[depth]); out_size(memory[italic]);
out_size(nl+lk_offset); out_size(nk); out_size(ne); out_size(np);
@y
if file_format=jfm_format then
  begin out_size(yoko_id_number); out_size(nt);
  end
else if file_format=vfm_format then
  begin out_size(tate_id_number); out_size(nt);
  end;
out_size(lf); out_size(lh); out_size(bc); out_size(ec);
out_size(memory[width]); out_size(memory[height]);
out_size(memory[depth]); out_size(memory[italic]);
out_size(nl+lk_offset); out_size(nk);
if file_format<>tfm_format then begin out_size(ng*3)
  end
else begin out_size(ne);
  end;
out_size(np);
@z

@x [146] l.2476 - pTeX:
@p procedure param_enter;
@y
@p
@<Declare kanji scanning routines@>@/
procedure param_enter;
@z

@x [146] l.2488 - pTeX: LIGTABLE command can not be used in JPL.
begin @<Read ligature/kern list@>;
end;
@y
begin @<If is jfm or vfm then print error@>;
@<Read ligature/kern list@>;
end;
@z

@x [146] l.2493 - pTeX: CHARACTER command can not be used in JPL.
begin @<Read character info list@>;
end;
@y
begin @<If is jfm or vfm then print error@>;
@<Read character info list@>;
end;
@z

@x [146] l.2506 - pTeX:
begin @<Correct and check the information@>
end;
@y
begin @<Correct and check the information@>
end;
@#
procedure read_kanji_info; {TYPE command}
var @!c:byte; {the char}
begin @<If is tfm then print error@>;
@<Read Kanji character type list@>;
end;
@#
procedure read_glue_kern; {GLUEKERN command}
var krn_ptr:0..max_kerns; {an index into |kern|}
@!c:byte; {runs through all character codes}
begin @<If is tfm then print error@>;
@<Read glue/kern list@>;
end;
@#
procedure read_chars_in_type; {CHARSINTYPE command}
var @!type_num:byte; {kanji character type number}
@!jis_code:integer; {sixteen bits Kanji character code}
begin @<If is tfm then print error@>;
@<Read Kanji characters list in this type@>;
end;
@#
procedure read_direction; {DIRECTION command}
begin @<If is tfm then print error@>;
@<Read direction@>;
end;
@z

@x
const n_options = 3; {Pascal won't count array lengths for us.}
@y
const n_options = 5; {Pascal won't count array lengths for us.}
@z

@x
      usage_help (PLTOTF_HELP, nil);
@y
      usage_help (PPLTOTF_HELP, nil);
@z

@x
    end; {Else it was a flag; |getopt| has already done the assignment.}
@y
    end else if argument_is ('kanji') then begin
      if (not set_enc_string(optarg,optarg)) then
        print_ln('Bad kanji encoding "', stringcast(optarg), '".');

    end; {Else it was a flag; |getopt| has already done the assignment.}
@z

@x
@ An element with all zeros always ends the list.
@y
@ Shift-JIS terminal (the flag is ignored except for WIN32).
@.-sjis-terminal@>

@<Define the option...@> =
long_options[current_option].name := 'sjis-terminal';
long_options[current_option].has_arg := 0;
long_options[current_option].flag := address_of (sjis_terminal);
long_options[current_option].val := 1;
incr (current_option);

@ Kanji option.
@.-kanji@>

@<Define the option...@> =
long_options[current_option].name := 'kanji';
long_options[current_option].has_arg := 1;
long_options[current_option].flag := 0;
long_options[current_option].val := 0;
incr(current_option);

@ An element with all zeros always ends the list.
@z

@x [148] l.2620 - pTeX:
@* Index.
@y
@* For Japanese Font Metric routines.
We need to include some routines for handling kanji characters.

@<Constants...@>=
max_kanji=7237; { maximam number of 2byte characters }
yoko_id_number=11; { is identifier for YOKO-kumi font}
tate_id_number=9; { is identifier for TATE-kumi font}

@ @<Glob...@>=
file_format:undefined..vfm_format; {the format of the input file}
kanji_type:array[0..max_kanji] of -1..256; {the type of every kanji char }
kanji_type_index:0..max_kanji; { index into above }
nt:integer; {number of entries in character type table}
glue:array[0..768] of fix_word; {the distinct glue amounts}
ng:integer; {number of 3-word entries in glue table}

@ @<Set init...@>=
file_format:=undefined;
for kanji_type_index:=0 to max_kanji do kanji_type[kanji_type_index]:=-1;
ng:=0;

@ @<If is jfm or vfm then print error@>=
if (file_format>tfm_format) then
  err_print('This is an illegal command for kanji format files.')
else if (file_format=undefined) then file_format:=tfm_format

@ @<If is tfm then print error@>=
if (file_format=tfm_format) then
  err_print('You can use this command only for kanji format files.')
else if (file_format=undefined) then file_format:=jfm_or_vfm

@ These are extended propaties for \.{JFM}.

@<Enter all of the names and ...@>=
load4("T")("Y")("P")("E")(type_code);@/
load8("G")("L")("U")("E")("K")("E")("R")("N")(glue_kern_code);@/
load11("C")("H")("A")("R")("S")("I")("N")("T")("Y")("P")("E")
  (chars_in_type_code);@/
load9("D")("I")("R")("E")("C")("T")("I")("O")("N")(dir_code);@/
load4("G")("L")("U")("E")(glue_code);@/

@ @<Enter the parameter names@>=
load10("E")("X")("T")("R")("A")("S")("P")("A")("C")("E")(parameter_code+7);@/
load12("E")("X")("T")("R")("A")("S")("T")("R")("E")("T")("C")("H")
  (parameter_code+8);@/
load11("E")("X")("T")("R")("A")("S")("H")("R")("I")("N")("K")
  (parameter_code+9);@/


@ Here, we declare kanji related routines and package gluekern stuff.
There routines a bit similar reading ligature/kern programs.

@<Read glue/kern list@>=
begin while level=1 do
  begin while cur_char=" " do get_next;
  if cur_char="(" then @<Read a glue/kern command@>
  else if cur_char=")" then skip_to_end_of_item
  else junk_error;
  end;
finish_inner_property_list;
end;

@ @<Read a glue/kern command@>=
begin get_name;
if cur_code=comment_code then skip_to_end_of_item
else  begin case cur_code of
  label_code:@<Read a glue label step@>;
  stop_code:@<Read a stop step@>;
  krn_code:@<Read a (glue) kerning step@>;
  glue_code:@<Read a glue step@>;
  others:
    flush_error('This property name doesn''t belong in a GLUEKERN list');
@.This property name doesn't belong...@>
  end;
  finish_the_property;
  end;
end

@ When a character is about to be tagged, we use the following
so that an error message is given in case of multiple tags.

@<Read a glue label step@>=
begin c:=get_byte;
case char_tag[c] of
  no_tag: do_nothing;
  lig_tag: err_print('This character already appeared in a GLUEKERN LABEL');
  @.This character already...@>
  list_tag: err_print('Impossible: a list tag in a kanji format file?');
  ext_tag: err_print('Impossible: an extensible tag in a kanji format file?');
end;
if nl>255 then
  err_print('GLUEKERN with more than 255 commands cannot have further labels')
@.GLUEKERN with more than 255...@>
else begin char_tag[c]:=lig_tag; char_remainder[c]:=nl;
  lk_step_ended:=false;
  end;
end

@ @<Read a (glue) kerning step@>=
begin lig_kern[nl].b0:=0; lig_kern[nl].b1:=get_byte;@/
lig_kern[nl].b2:=kern_flag; kern[nk]:=get_fix; krn_ptr:=0;
while kern[krn_ptr]<>kern[nk] do incr(krn_ptr);
if krn_ptr=nk then
  begin if nk<256 then incr(nk)
  else begin err_print('At most 256 different kerns are allowed');
@.At most 256 different kerns...@>
    krn_ptr:=255;
    end;
  end;
lig_kern[nl].b3:=krn_ptr;
if nl=511 then
  err_print('GLUEKERN table should never exceed 511 LIG/KRN commands')
@.GLUEKERN table should never...@>
else incr(nl);
lk_step_ended:=true;
end

@ @<Read a glue step@>=
begin lig_kern[nl].b0:=0; lig_kern[nl].b1:=get_byte; lig_kern[nl].b2:=0;@/
glue[3*ng+0]:=get_fix; glue[3*ng+1]:=get_fix; glue[3*ng+2]:=get_fix;
krn_ptr:=0;
while (glue[3*krn_ptr+0]<>glue[3*ng+0])or
      (glue[3*krn_ptr+1]<>glue[3*ng+1])or
      (glue[3*krn_ptr+2]<>glue[3*ng+2]) do incr(krn_ptr);
if krn_ptr=ng then
  begin if ng<256 then incr(ng)
  else begin err_print('At most 256 different glues are allowed');
    krn_ptr:=255;
    end;
  end;
lig_kern[nl].b3:=krn_ptr;
if nl=511 then
  err_print('GLUEKERN table should never exceed 511 GLUE/KRN commands')
@.GLUEKERN table should never...@>
else incr(nl);
lk_step_ended:=true;
end

@ The |TYPE| command like |CHARACTER| command, but |TYPE| only use
|CHARWD|, |CHARHT|, |CHARDP| and |CHARIT|

@<Read Kanji character type list@>=
begin c:=get_byte; {read the character type that is begin specified}
if verbose then @<Print |c| in octal notation@>;
while level=1 do
  begin while cur_char=" " do get_next;
  if cur_char="(" then @<Read a kanji property@>
  else if cur_char=")" then skip_to_end_of_item
    else junk_error;
  end;
if char_wd[c]=0 then char_wd[c]:=sort_in(width,0); {legitimatize c}
finish_inner_property_list;
end;

@ @<Read a kanji property@>=
begin get_name;
if cur_code=comment_code then skip_to_end_of_item
else if (cur_code<char_wd_code)and(cur_code>char_ic_code) then
  flush_error('This property name doesn''t belong in a TYPE list')
else  begin case cur_code of
  char_wd_code: char_wd[c]:=sort_in(width,get_fix);
  char_ht_code: char_ht[c]:=sort_in(height,get_fix);
  char_dp_code: char_dp[c]:=sort_in(depth,get_fix);
  char_ic_code: char_ic[c]:=sort_in(italic,get_fix);
  end;@/
  finish_the_property;
  end;
end

@ Next codes used to get KANJI codes from \.{JPL} file.

@<Read Kanji characters list in this type@>=
begin type_num:=get_byte;
if type_num=0 then
  skip_error('You cannot list the chars in type 0. It is the default type')
else  begin repeat jis_code:=get_kanji;
  if jis_code<0 then
    err_print('Illegal characters. I was expecting a jis code or character')
  else if jis_code=0 then { 0 signals |end_of_list| }
    do_nothing
  else if kanji_type[jis_to_index(jis_code)]>=0 then
    err_print('jis code ', jis_code:1, ' is already in type ',
      kanji_type[jis_to_index(jis_code)])
  else
    kanji_type[jis_to_index(jis_code)]:=type_num;
  until jis_code=0;
  skip_to_paren;
  end
end

@ Next codes read and check direction.  We can not decide |file_format| of
metric file whether for yoko-kumi or tate-kumi, until have scan |DIRECTION|
property (|dir_code| command).

@<Read direction@>=
begin while cur_char=" " do get_next;
if cur_char="T" then
  begin if verbose then err_print('This is tatekumi format');
  file_format:=vfm_format;
  end
else if cur_char="Y" then
  begin if verbose then err_print('This is yokokumi format');
  file_format:=jfm_format;
  end
else err_print('The dir value should be "TATE" or "YOKO"');
skip_to_paren;
end

@ Next codes used to write |kanji_type| to \.{JFM}.

@<Output the kanji character type info@>=
begin out_size(0); out_size(0); { the default }
for kanji_type_index:=0 to max_kanji do
  begin if kanji_type[kanji_type_index]>0 then
    begin out_size(index_to_jis(kanji_type_index));
    out_size(kanji_type[kanji_type_index]);
    if verbose then begin
      print('char index = ', kanji_type_index);
      print(' (jis ');
      print_jis_hex(index_to_jis(kanji_type_index));
      print(') is type ');
      print_octal(kanji_type[kanji_type_index]);
      write_ln('');
      end;
    end;
  end;
end;

@ We also need to define some routines which handling 2bytes characters.
These routine is called from only |read_chars_in_type| command.

The kanji jis code is taken from the |char_ext| and |char_code| values
set by the user.  The index into the |kanji_type| array is based on the
kuten codes, with all unused codes removed and beginning at 0, not 0101.
The |jis_to_index| is called from |chars_in_type| command.

@<Declare kanji scanning routines@>=
function get_next_raw:byte; {get next rawdata in buffer}
begin while loc=limit do fill_buffer;
incr(loc); get_next_raw:=buffer[loc];
if multistrlen(ustringcast(buffer),loc+2,loc)=2 then cur_char:=" "
else cur_char:=xord[buffer[loc]];
end;
@#
function todig(@!ch:byte):byte; {convert character to number}
begin if (ch>="A")and(ch<="F") then todig:=ch-"A"+10
else if (ch>="0")and(ch<="9") then todig:=ch-"0"
else  begin skip_error('This expression is out of JIS-code encoding.');
  todig:=0;
  end;
end;
@#
procedure print_jis_hex(jis_code:integer); {prints jiscode as four digits}
var dig:array[0..4] of byte; {holds jis hex codes}
i:byte; {index of array}
begin dig[0]:=Hi(jis_code) div 16; dig[1]:=Hi(jis_code) mod 16;
dig[2]:=Lo(jis_code) div 16; dig[3]:=Lo(jis_code) mod 16;
for i:=0 to 3 do
  if dig[i]<10 then print(dig[i]) else
  case dig[i] of
     10: print('A'); 11: print('B'); 12: print('C');
     13: print('D'); 14: print('E'); 15: print('F');
  end;
end;
@#
function valid_jis_code(cx:integer):boolean;
var @!first_byte,@!second_byte:integer; { jis code bytes }
begin valid_jis_code:=true;
first_byte:=cx div @'400; second_byte:=cx mod @'400;
if (first_byte<@"21)
   or((first_byte>@"28)and(first_byte<@"30))
   or(first_byte>@"74) then valid_jis_code:=false;
if (second_byte<@"21)or(second_byte>@"7E) then valid_jis_code:=false;
end;
@#
function jis_to_index(jis:integer):integer;
var @!first_byte,@!second_byte:integer; { jis code bytes }
begin
first_byte:=jis div @'400 -@"21;
second_byte:=jis mod @'400 -@"21;
if first_byte<8 then
  jis_to_index:=first_byte*94+second_byte
else { next |first_byte| start 16 }
  jis_to_index:=(first_byte-7)*94+second_byte;
end;
@#
function index_to_jis(ix:integer):integer;
begin if ix<=8*94-1 then
  index_to_jis:=(ix div 94 +@"21)*@'400+(ix mod 94 +@"21)
else
  index_to_jis:=((ix+7*94) div 94 +@"21)*@'400+((ix+7*94) mod 94 +@"21);
end;
@#
function get_kanji:integer; {get kanji character code}
var @!ch:byte;
@!cx,@!jis_code:integer; {sixteen bits kanji character code}
begin repeat ch:=get_next_raw; {|ch| is rawdata in buffer}
until ch<>' '; {skip the blanks before the kanji code}
if ch=')' then
  begin decr(loc); jis_code:=0;
  end
else if (ch='J')or(ch='j') then
  begin repeat ch:=get_next_raw; until ch<>' ';
  cx:=todig(xord[ch])*@"1000;
  incr(loc); ch:=xord[buffer[loc]]; cx:=cx+todig(ch)*@"100;
  incr(loc); ch:=xord[buffer[loc]]; cx:=cx+todig(ch)*@"10;
  incr(loc); ch:=xord[buffer[loc]]; cx:=cx+todig(ch);
  jis_code:=toDVI(fromJIS(cx)); cur_char:=ch;
  if not valid_jis_code(jis_code) then
    err_print('jis code ', jis_code:1, ' is invalid');
  end
else if multistrlen(ustringcast(buffer), loc+2, loc)=2 then
  begin jis_code:=toDVI(fromBUFF(ustringcast(buffer), loc+2, loc));
  incr(loc); cur_char:=" ";
  if not valid_jis_code(jis_code) then
    err_print('jis code ', jis_code:1, ' is invalid');
  end
else jis_code:=-1;
get_kanji:=jis_code;
end;

@* Index.
@z

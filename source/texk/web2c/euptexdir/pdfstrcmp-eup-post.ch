@x
    cur_tok := (cur_cmd*@'400)+cur_chr;
@y
    cur_tok := (cur_cmd*max_char_val)+cur_chr;
@z

@x
    if (cc=not_cjk) then cc:=other_kchar;
@y
    if (cat>=kanji)and(cat<=modifier) then cc:=cat else if (cc=not_cjk) then cc:=other_kchar;
@z

@x
  else if (t=" ")and(cat=0) then t:=space_token
  else if (cat=0)or(cat>=kanji) then t:=other_token+t
  else if cat=active_char then t:= cs_token_flag + active_base + t
  else t:=left_brace_token*cat+t;
@y
    if (t=" ")and(cat=0) then t:=space_token
    else if (cat=0)or((cat>=kanji)and(cat<=modifier)) then t:=other_token+t
    else if cat=active_char then t:= cs_token_flag + active_base + t
    else t:=left_brace_token*cat+t;
@z

@x
@d illegal_Ucharcat_wchar_catcode(#)==(#<kanji)or(#>other_kchar)
@y
@d illegal_Ucharcat_wchar_catcode(#)==(#<kanji)or(#>modifier)
@z

@x
Uchar_convert_code:       scan_char_num;
Ucharcat_convert_code:
  begin
    scan_ascii_num;
@y
Uchar_convert_code: begin scan_char_num;
    if not is_char_ascii(cur_val) then
	  if kcat_code(kcatcodekey(cur_val))=not_cjk then cat:=other_kchar;
    end;
Ucharcat_convert_code:
  begin
    scan_char_num;
@z

@x
    if illegal_Ucharcat_ascii_catcode(cur_val) then
      begin print_err("Invalid code ("); print_int(cur_val);
@.Invalid code@>
      print("), should be in the ranges 1..4, 6..8, 10..13");
      help1("I'm going to use 12 instead of that illegal code value.");@/
      error; cat:=12;
    end else cat:=cur_val;
@y
    if i<=@"7F then { no |wchar_token| }
      begin if illegal_Ucharcat_ascii_catcode(cur_val) then
        begin print_err("Invalid code ("); print_int(cur_val);
@.Invalid code@>
        print("), should be in the ranges 1..4, 6..8, 10..13");
        help1("I'm going to use 12 instead of that illegal code value.");@/
        error; cat:=12;
      end else cat:=cur_val;
    end else if i<=@"FF then
      begin if (illegal_Ucharcat_ascii_catcode(cur_val))
        and (illegal_Ucharcat_wchar_catcode(cur_val)) then
        begin print_err("Invalid code ("); print_int(cur_val);
@.Invalid code@>
        print("), should be in the ranges 1..4, 6..8, 10..13, 16..19");
        help1("I'm going to use 12 instead of that illegal code value.");@/
        error; cat:=12;
      end else cat:=cur_val;
    end else { |wchar_token| only }
      begin if illegal_Ucharcat_wchar_catcode(cur_val) then
        begin print_err("Invalid code ("); print_int(cur_val);
@.Invalid code@>
        print("), should be in the ranges 16..19");
        help1("I'm going to use 18 instead of that illegal code value.");@/
        error; cat:=other_kchar;
      end else cat:=cur_val;
	end;
@z

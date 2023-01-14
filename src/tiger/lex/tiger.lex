%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID//
  *   Parser::STRING
  *   Parser::INT//
  *   Parser::COMMA//
  *   Parser::COLON//
  *   Parser::SEMICOLON//
  *   Parser::LPAREN//
  *   Parser::RPAREN//
  *   Parser::LBRACK//
  *   Parser::RBRACK//
  *   Parser::LBRACE//
  *   Parser::RBRACE//
  *   Parser::DOT//
  *   Parser::PLUS//
  *   Parser::MINUS//
  *   Parser::TIMES//
  *   Parser::DIVIDE//
  *   Parser::EQ//
  *   Parser::NEQ//
  *   Parser::LT//
  *   Parser::LE//
  *   Parser::GT//
  *   Parser::GE//
  *   Parser::AND//
  *   Parser::OR//
  *   Parser::ASSIGN//
  *   Parser::ARRAY//
  *   Parser::IF//
  *   Parser::THEN//
  *   Parser::ELSE//
  *   Parser::WHILE//
  *   Parser::FOR//
  *   Parser::TO//
  *   Parser::DO//
  *   Parser::LET//
  *   Parser::IN//
  *   Parser::END//
  *   Parser::OF//
  *   Parser::BREAK//
  *   Parser::NIL//
  *   Parser::FUNCTION//
  *   Parser::VAR//
  *   Parser::TYPE//
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}

"while" {adjust(); return Parser::WHILE;}
"for"	{adjust(); return Parser::FOR;}
"to"	{adjust(); return Parser::TO;}
"break"	{adjust(); return Parser::BREAK;}
"let"	{adjust(); return Parser::LET;}
"in"	{adjust(); return Parser::IN;}
"end"	{adjust(); return Parser::END;}
"function"	{adjust(); return Parser::FUNCTION;}
"var"	{adjust(); return Parser::VAR;}
"type"	{adjust(); return Parser::TYPE;}
"if"	{adjust(); return Parser::IF;}
"then"	{adjust(); return Parser::THEN;}
"else"	{adjust(); return Parser::ELSE;}
"do"	{adjust(); return Parser::DO;}
"of"	{adjust(); return Parser::OF;}
"nil"	{adjust(); return Parser::NIL;} 

 /* commas */
","	{adjust(); return Parser::COMMA;}
":"	{adjust(); return Parser::COLON;}
";"	{adjust(); return Parser::SEMICOLON;}
"("	{adjust(); return Parser::LPAREN;}
")"	{adjust(); return Parser::RPAREN;}
"["	{adjust(); return Parser::LBRACK;}
"]"	{adjust(); return Parser::RBRACK;}
"{"	{adjust(); return Parser::LBRACE;}
"}"	{adjust(); return Parser::RBRACE;}
"."	{adjust(); return Parser::DOT;}
"+"	{adjust(); return Parser::PLUS;}
"-"	{adjust(); return Parser::MINUS;}
"*"	{adjust(); return Parser::TIMES;}
"/"	{adjust(); return Parser::DIVIDE;}
"="	{adjust(); return Parser::EQ;}
"<>"	{adjust(); return Parser::NEQ;}
"<"	{adjust(); return Parser::LT;}
"<="	{adjust(); return Parser::LE;}
">"	{adjust(); return Parser::GT;}
">="	{adjust(); return Parser::GE;}
"&"	{adjust(); return Parser::AND;}
"|"	{adjust(); return Parser::OR;}
":="	{adjust(); return Parser::ASSIGN;}

 /*ID*/
 /* the matched() is equalvilent to the YYText in the textbook!it's a string that's matched by the R.E.*/
 /* the ID may contain some "_"(but can't in the start of the ID) */
[a-zA-Z][_a-zA-Z0-9]*	{adjust(); string_buf_ = matched(); return Parser::ID;}
 /*Int*/
[0-9]+	{adjust(); string_buf_ = matched(); return Parser::INT;}
 /*string*/
"/*"	{
		adjust();
		comment_level_ = 1;
		begin(StartCondition__::COMMENT);//start with the "/*",change to the comment lexical mode.
	}
\"	{
		adjust();
		string_buf_ = "";//when a new string start, clear the last inputted string.
		begin(StartCondition__::STR);//start of the string, change to the string mode.
	}
 /*error handling?*/
<STR>	{
		\" { 
			adjustStr();
			begin(StartCondition__::INITIAL);//the end of the string, change to the normal mode.
			setMatched(string_buf_);//use the setMatch func to change the string to the detailed type.
			//string_buf_ = "";//when a string's end is detected, clean it and make it ready to the next string detection.
			return Parser::STRING;//meet the real end of the string, return the token og string to the parser.
		}
		/*part of the zhuanyi xulie*/
		\\[0-9][0-9][0-9] {
			adjustStr();
			//string_buf_ += (char)atoi(matched().substr(1, 3).c_str());//don't forget the c_str function,the atoi func can only used to change the char* to a int!
			string_buf_ += (char)changeStrToInt(matched().substr(1, 3));
		}
		\\[ \t\n]+\\ {
			adjustStr();
			//string_buf_ += matched();
			//errormsg_->Newline();
			/*when detect the "f___f" string, ignore it directly(programming facing to the result... )*/
			string_buf_ += "";	
		}
		"\\\\"	{
			adjustStr();
			string_buf_ += matched().substr(1);
		}
		"\\n"	{//notice that here is the "\n" that will be printed in the string directly, so we need to show the "\\" directly.
			adjustStr();
			string_buf_ += "\n";
		}
		"\\t"	{
			adjustStr();
			string_buf_ += "\t";
		}
		\\\^[@A-Z]	{
			adjustStr();
			std::string s = matched();
			string_buf_ += (s[2] - 'A' + 1);
		}
		"\\\^["	{
			adjustStr();
			std::string s = matched();
			string_buf_ += (s[2] - 'A' + 1);
		}
		\\\"	{
			adjustStr();
			string_buf_ += "\"";//handle the nested "" tokens.
		}
		/*part of normal input except for the new line*/
		.	{
			adjustStr();
			string_buf_ += matched();
		}
		
	}
 /*comment, the comment part use the adjustStr instead of adjust() beacause we don't care about the info in the "/*" pair, but we will try to fin the error about the unpaired "/*"s */
<COMMENT>{
		"*/" {
			adjustStr();
			//comment_level_--;//detect the end of a level of the (nested) comment
			if(comment_level_ == 1) begin(StartCondition__::INITIAL);
			else comment_level_--;
		}
		.    {//any input except the new line.
			adjustStr();
		}
		\n  {//newline of the comment
			adjustStr();
		}
		"/*" {
			adjustStr();
			comment_level_++;
			begin(StartCondition__::COMMENT);
		}
		<<EOF>>	{
			return 0;
		}
	}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}
  
 /*when detect the EOF, according to the enum token__ in the file parserbase.h, the enum start from the 257, so I will return a num < 257, which is chosen to be 0.*/
<<EOF>>	{return 0;}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}

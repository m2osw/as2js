" Vim syntax file
" Language:	AS2JS
" Maintainer:	Alexis Wilke <alexis@m2osw.com>
" Last change:	2014 Nov 12
"
" Installation:
"
" To use this file, add something as follow in your .vimrc file:
"
"   if !exists("my_autocommands_loaded")
"     let my_autocommands_loaded=1
"     au BufNewFile,BufReadPost *.js    so $HOME/vim/as2js.vim
"   endif
"
" Obviously, you will need to put the correct path to the as2js.vim
" file before it works, and you may want to use an extension other
" than .js.
"
"
" Copyright (c) 2005-2025  Made to Order Software Corp.  All Rights Reserved
"
" Permission is hereby granted, free of charge, to any
" person obtaining a copy of this software and
" associated documentation files (the "Software"), to
" deal in the Software without restriction, including
" without limitation the rights to use, copy, modify,
" merge, publish, distribute, sublicense, and/or sell
" copies of the Software, and to permit persons to whom
" the Software is furnished to do so, subject to the
" following conditions:
"
" The above copyright notice and this permission notice
" shall be included in all copies or substantial
" portions of the Software.
"
" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
" ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
" LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
" FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
" EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
" LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
" WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
" ARISING FROM, OUT OF OR IN CONNECTION WITH THE
" SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
" SOFTWARE.
"


" Remove any other syntax
syn clear


set formatoptions-=tc
set formatoptions+=or

" minimum number of lines for synchronization
" /* ... */ comments can be long
syn sync minlines=1500


" path is annoying on this one...
" TODO: make proper packages for these .vim and we can resolve the path
"       (see whether the jsdoc.vim has a package already)
source /home/alexis/vim/jsdoc.vim


" Operators
"  - Additive
syn match	as2jsOperator		"+"
syn match	as2jsOperator		"-"
syn match	as2jsOperator		"+="
syn match	as2jsOperator		"-="
syn match	as2jsOperator		"++"
syn match	as2jsOperator		"--"

"  - Multiplicative
syn match	as2jsOperator		"\*"
syn match	as2jsOperator		"/"
syn match	as2jsOperator		"%"
syn match	as2jsOperator		"\*\*"
syn match	as2jsOperator		"\*="
syn match	as2jsOperator		"/="
syn match	as2jsOperator		"%="
syn match	as2jsOperator		"\*\*="

"  - Bitwise
syn match	as2jsOperator		"&"
syn match	as2jsOperator		"\~"
syn match	as2jsOperator		"|"
syn match	as2jsOperator		"\^"
syn match	as2jsOperator		"&="
syn match	as2jsOperator		"|="
syn match	as2jsOperator		"\^="

"  - Shifts
syn match	as2jsOperator		"<<"
syn match	as2jsOperator		">>"
syn match	as2jsOperator		">>>"
syn match	as2jsOperator		"<<="
syn match	as2jsOperator		">>="
syn match	as2jsOperator		">>>="
syn match	as2jsOperator		"<%"
syn match	as2jsOperator		"<%="

"  - Logical
syn match	as2jsOperator		"!"
syn match	as2jsOperator		"&&"
syn match	as2jsOperator		"||"
syn match	as2jsOperator		"\^\^"
syn match	as2jsOperator		"&&="
syn match	as2jsOperator		"||="
syn match	as2jsOperator		"\^\^="

"  - Selection
syn match	as2jsOperator		">?"
syn match	as2jsOperator		"<?"
syn match	as2jsOperator		">?="
syn match	as2jsOperator		"<?="
syn match	as2jsOperator		"??"
syn match	as2jsOperator		"??="

"  - Comparative
syn match	as2jsOperator		"=="
syn match	as2jsOperator		"==="
syn match	as2jsOperator		"≈"
syn match	as2jsOperator		"!="
syn match	as2jsOperator		"!==="
syn match	as2jsOperator		"<>"
syn match	as2jsOperator		">"
syn match	as2jsOperator		">="
syn match	as2jsOperator		"<"
syn match	as2jsOperator		"<="
syn match	as2jsOperator		"<=>"
syn match	as2jsOperator		"\~\~"
syn match	as2jsOperator		"\~!"
syn match	as2jsOperator		"\~="

"  - Assignment Only
syn match	as2jsOperator		"="
syn match	as2jsOperator		":="

"  - Other
syn match	as2jsOperator		"=>"
syn match	as2jsOperator		"\."
syn match	as2jsOperator		"?\."
syn match	as2jsOperator		"\.\."
syn match	as2jsOperator		"\.\.\."
syn match	as2jsOperator		"::"
"syn match	as2jsOperator		"\(\)" -- this one doesn't work right, plus many functions may have no parameters and match this too
"syn match	as2jsOperator		";"
"syn match	as2jsOperator		","


syn case match

" Complex keywords
syn match	as2jsKeyword		"\<function\>\([ \t\n\r]\+\<[sg]et\>\)\="
syn match	as2jsKeyword		"\<for\>\([ \t\n\r]\+\<each\>\)\="

" Keywords
syn keyword	as2jsKeyword		as async await break case catch class
syn keyword	as2jsKeyword		const continue default delete do else
syn keyword	as2jsKeyword		enum extends extern finally friend
syn keyword	as2jsKeyword		goto if implements import in
syn keyword	as2jsKeyword		inline instanceof interface
syn keyword	as2jsKeyword		intrinsic is let namespace native new
syn keyword	as2jsKeyword		package private public return
syn keyword	as2jsKeyword		static super switch
syn keyword	as2jsKeyword		this throw try typeof unimplemented
syn keyword	as2jsKeyword		use var virtual with while

" Known Types (internal)
syn keyword	as2jsType		Array BigInt Boolean Buffer Date Double
syn keyword	as2jsType		Function Global Integer Math Native Number
syn keyword	as2jsType		Object RegularExpression RegExp String
syn keyword	as2jsType		System Void Range

" Constants
syn keyword	as2jsConstant		true false null undefined Infinity NaN __dirname __filename
syn match	as2jsConstant		"\<0[xX][0-9A-F]\+n\?\>"
syn match	as2jsConstant		"\<0[oO][0-7]*n\?\>"
syn match	as2jsConstant		"\<0[bB][0-1]*n\?\>"
syn match	as2jsConstant		"\<00*\>"
syn match	as2jsConstant		"\<[1-9][0-9]*\.\=[0-9]*\([eE][+-]\=[0-9]\+\)\=\>"
syn match	as2jsConstant		"\<0\=\.[0-9]\+\([eE][+-]\=[0-9]\+\)\=\>"
syn region	as2jsConstant		start=+"+ skip=+\\.+ end=+"+
syn region	as2jsConstant		start=+'+ skip=+\\.+ end=+'+
syn region	as2jsTemplate		start=+`+ skip=+\\.+ end=+`+


" Labels
syn match	as2jsLabel		"\<[a-zA-Z_$][a-zA-Z_$0-9]*\s*:[=:]\@!"
syn match	as2jsGlobal		"\<[a-zA-Z_$][a-zA-Z_$0-9]*_\>"

" prevent labels in `?:' expressions
syn region	as2jsNothing		start="?" end=":" contains=as2jsConstant,as2jsLComment,as2jsMComment
syn match	as2jsOperator		"?"
syn match	as2jsOperator		":"


" Comments
syn keyword	as2jsTodo		contained TODO FIXME XXX
syn match	as2jsTodo		contained "WATCH\(\s\=OUT\)\="
syn region	as2jsMComment		start="/\*" end="\*/" contains=as2jsTodo
syn region	as2jsLComment		start="//" end="$" contains=as2jsTodo


let b:current_syntax = "as2js"

if !exists("did_as2js_syntax_inits")
  let did_as2js_syntax_inits = 1
  hi link as2jsKeyword			Keyword
  hi link as2jsMComment			Comment
  hi link as2jsLComment			Comment
  hi link as2jsTodo			Todo
  hi link as2jsType			Type
  hi link as2jsOperator			Operator
  hi link as2jsConstant			Constant
  hi link as2jsTemplate			Constant
  hi link as2jsRegularExpression	Constant
  hi      as2jsLabel			guifg=#cc0000
  hi      as2jsGlobal			guifg=#00aa88
endif

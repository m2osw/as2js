" Vim syntax file
" Language:	ASON
" Maintainer:	Alexis Wilke <alexis@m2osw.com>
" Last change:	2023 Jun 24
"
" Installation:
"
" To use this file, add something as follow in your .vimrc file:
"
"   if !exists("my_autocommands_loaded")
"     let my_autocommands_loaded=1
"     au BufNewFile,BufReadPost *.ason  so $HOME/vim/ason.vim
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

" Remove any other syntax
syn clear

" Load the normal json with comment syntax file
source $VIMRUNTIME/syntax/jsonc.vim

let b:current_syntax = "ason"

syn clear jsonStringMatch
syn clear jsonString

syn match jsonStringMatch /"\([^"]\|\\\"\|\r\|\n\)\+"\ze[[:blank:]\r\n]*[,}\]]/ contains=jsonString
if has('conceal') && (!exists("g:vim_json_conceal") || g:vim_json_conceal==1)
  syn region jsonString matchgroup=jsonQuote start=/"/ skip=/\\\\\|\\"/ end=/"\ze[[:blank:]\r\n]*[,}\]]/ concealends contains=jsonEscape contained
else
  syn region jsonString matchgroup=jsonQuote start=/"/ skip=/\\\\\|\\"/ end=/"\ze[[:blank:]\r\n]*[,}\]]/ contains=jsonEscape contained
endif

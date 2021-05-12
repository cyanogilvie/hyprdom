" Vim syntax file
" Language: Parser Expression Grammars (PEG)
" Maintainer: Cyan Ogilvie
" Latest Revision: 2021-05-11

if exists("b:current_syntax")
  finish
endif

syn region	pegCRegion			start=/\v^\%(source|header|common|earlysource|earlyheader|earlycommon)\s+\{/ end=/\v^\}/
syn region	pegCRegion			start=/\v^\%(prefix|auxil|value)\s+"/ end=/"/
syn region	pegCRegion			start=/\v^\%\%\n/ end=/\v^\%\%EOF\%\%/
syn match   pegRuleIdentifier   /^\s*\<[a-zA-Z_][a-zA-Z_0-9]*\>/ skipwhite nextgroup=pegSeperator
syn match   contnIdentifier      /^\s\+\// skipwhite nextgroup=pegExpression

syn match   pegSeperator        "<-" skipwhite contained nextgroup=pegExpression
syn match   pegSeperator        "←" skipwhite contained nextgroup=pegExpression

syn match   pegExpression       /.*/ contained skipwhite contains=pegDelimiter,pegGrouping,pegSpecial,pegRange,pegTerminal,pegOrderedChoice,pegNonTerminal,pegQuantifier
syn match   pegDelimiter        /[§]/ contained display
syn region  pegGrouping         matchgroup=pegDelimiter start=/(/ end=/)/ contained skipwhite keepend contains=pegExpression display
syn region  pegGrouping         matchgroup=pegDelimiter start=/</ end=/>/ contained skipwhite keepend contains=pegExpression display
syn match   pegSpecial          /[!&ϵ^]/ contained display
syn match   pegOrderedChoice    /\// contained display
syn region  pegRange            matchgroup=pegDelimiter start=/\[^/ start=/\[/ end=/\]/ contained skipwhite contains=pegRangeValue,pegUnicode display
syn match   pegRangeValue       /\d\+-\d\+/ contained display
syn match   pegRangeValue       /\a\+-\a\+/ contained display
syn region  pegTerminal         matchgroup=pegDelimiter start=/"/ end=/"/ contained display
syn region  pegTerminal         matchgroup=pegDelimiter start=/'/ end=/'/ contained display
syn match   pegUnicode          /U+[A-F0-9]\{4,6}/ contained display
syn match   pegNonTerminal      /\a+/ contained display
syn match   pegQuantifier       /[+\*?]/ contained display
syn match   pegQuantifier       /{\d\+,\d\+}/ contained display
syn match   pegQuantifier       /{\d\+}/ contained display

syn match   pegComment          /[;#].*$/ contains=pegTodo
syn match	pegVariable			/\$[0-9]+/ contained display
syn keyword pegTodo             TODO FIXME XXX NOTE contained

hi link pegRuleIdentifier Identifier
hi link pegSeperator      Conditional
hi link pegDelimiter      Delimiter
hi link pegSpecial        Special
hi link pegOrderedChoice  Conditional
hi link pegComment        Comment
hi link pegRangeValue     Constant
hi link pegTerminal       String
hi link pegUnicode        Constant
hi link pegQuantifier     Function
hi link pegTodo           Todo
hi link pegVariable       Identifier
hi link pegCRange         Delimiter

let b:current_syntax = "peg"

call SyntaxRange#Include('\v\s\~\s*\{', '\}', 'c', 'CRegion')
call SyntaxRange#Include('\v\s*\{', '\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%header\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%source\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%common\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%earlyheader\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%earlysource\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%earlycommon\s+\{', '\v^\}', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%prefix\s+"', '"', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%auxil\s+"', '"', 'c', 'CRegion')
call SyntaxRange#Include('\v^\s*\%value\s+"', '"', 'c', 'CRegion')
call SyntaxRange#Include('^%%', '%%EOF%%', 'c', 'CRegion')

hi link CRegion Special

set autoindent

call OnSyntaxChange#Install('CRegion', 'synIncludeC', 1, 'a')
"autocmd User SyntaxCRegionEnterA unsilent echo "EnterC"
"autocmd User SyntaxCRegionLeaveA unsilent echo "LeaveC"
autocmd User SyntaxCRegionEnterA unsilent setlocal cindent
autocmd User SyntaxCRegionLeaveA unsilent setlocal nocindent

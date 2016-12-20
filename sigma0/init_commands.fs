: print-string ( addr -- )
  while dup c@ 0 != begin
      dup c@ emit
      1 +
  repeat drop
;

: hex ( n -- )
  "0x" print-string .x cr drop
;

: print-memfield ( n name -- )
  print-string ": " print-string . " cells (" print-string
  cells .  " bytes)" print-string
  cr drop
;

: meminfo
  push-meminfo
  "available memory:" print-string cr
  "     calls" print-memfield
  "parameters" print-memfield
  "      data" print-memfield
;

: make-msgbuf create 8 cells allot ;
: get-type    @ ;
: set-type    ! ;
: is-keycode? get-type 0xbabe = ;
: get-keycode 2 cells + @ ;

1  value debug-msg
2  value map-msg
3  value mapto-msg
4  value unmap-msg
5  value grant-msg
6  value grantto-msg
7  value requestphys-msg
8  value pagefault-msg
9  value dumpmaps-msg
10 value stop-msg
11 value continue-msg
12 value end-msg
13 value kill-msg

make-msgbuf buffer

: msgtest
  buffer recvmsg
  if buffer is-keycode? then
      "got a keypress: " print-string
      buffer get-keycode . cr drop
  else
      "got message with type " print-string
      buffer get-type . cr drop
  end
;

: do-send  buffer set-type buffer swap sendmsg ;
: stop     stop-msg     do-send ;
: continue continue-msg do-send ;
: debug    debug-msg    do-send ;
: kill     kill-msg     do-send ;
: dumpmaps dumpmaps-msg do-send ;
: ping     1234         do-send ;

: help
  "some things you can do:" print-string cr
  "  print-archives  : list available base words" print-string cr
  "  help            : print this help" print-string cr
  "  meminfo         : print the amount of memory available" print-string cr
  "  stop | continue : stop or continue the given thread id" print-string cr
  "  dumpmaps        : dump memory maps for the given thread" print-string cr
;

: initfs-print ( name -- )
  tarfind dup tarsize
  "size: " print-string .   cr drop
  "addr: " print-string hex cr
;

0xffffbeef value list-start
list-start value [[
: ]] ;

: dumpmaps-list
  while dup list-start != begin
    dumpmaps
  repeat
;

: ls ( -- )
  "/" tarfind
  while dup c@ 0 != begin
    dup print-string cr
    tarnext
  repeat
;

: exec ( program-path -- )
  "loading elf executable: " print-string dup print-string cr
  tarfind elfload drop
;

: exec-list ( path-list ... -- )
  while dup list-start != begin
    exec
  repeat
  drop
;

( initial program list to bootstrap the system )
: bootstrap ( -- )
  [[
    "sigma0/initfs/bin/keyboard"
    "sigma0/initfs/bin/test"
  ]] exec-list
;

bootstrap
"All systems are go, good luck" print-string cr

( XXX : newline needed at the end of the file because the init routine )
(       sets the last byte of the file to 0 )


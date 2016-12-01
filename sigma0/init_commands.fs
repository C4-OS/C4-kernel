: print-string ( addr -- )
  begin dup c@ 0 != while
      dup c@ emit
      1 +
  repeat drop
;

: hex ( n -- )
  "0x" print-string .x cr drop
;

: make-msgbuf create 8 cells allot ;
: get-type    @ ;
: set-type    ! ;
: is-keycode? get-type 47806 ( 0xbabe ) = ;
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
  buffer is-keycode? if
      "got a keypress: " print-string
      buffer get-keycode . cr drop
  else
      "got message with type " print-string
      buffer get-type . cr drop
  then
;

: do-send  buffer set-type buffer swap sendmsg ;
: stop     stop-msg     do-send ;
: continue continue-msg do-send ;
: debug    debug-msg    do-send ;
: kill     kill-msg     do-send ;
: dumpmaps dumpmaps-msg do-send ;

"All systems are go, good luck" print-string cr

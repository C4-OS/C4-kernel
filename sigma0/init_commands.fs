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
9  value stop-msg
10 value continue-msg
11 value end-msg
12 value kill-msg

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

: stop ( thread-id -- )
  stop-msg buffer set-type
  buffer 1 sendmsg
;

: continue
  continue-msg buffer set-type
  buffer 1 sendmsg
;

"All systems are go, good luck" print-string cr

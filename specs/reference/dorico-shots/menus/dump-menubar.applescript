on run argv
 set pn to item 1 of argv
 set maxd to (item 2 of argv) as integer
 set out to ""
 tell application "System Events" to tell process pn
  set bn to name of every menu bar item of menu bar 1
  repeat with b from 1 to count of bn
   if item b of bn is not "Apple" then
    set out to out & "## " & (item b of bn) & linefeed
    try
     set out to out & my walk(menu 1 of menu bar item b of menu bar 1, "", maxd)
    on error e
     set out to out & "ERR " & e & linefeed
    end try
   end if
  end repeat
 end tell
 return out
end run
on walk(m, ind, d)
 set r to ""
 tell application "System Events"
  set mi to every menu item of m
  set ns to name of every menu item of m
  set es to enabled of every menu item of m
  set cs to value of attribute "AXMenuItemCmdChar" of every menu item of m
  set ms to value of attribute "AXMenuItemCmdModifiers" of every menu item of m
  set mk to value of attribute "AXMenuItemMarkChar" of every menu item of m
  set gl to value of attribute "AXMenuItemCmdGlyph" of every menu item of m
  repeat with i from 1 to count of mi
   set n to item i of ns
   if n is missing value or n is "" then
    set r to r & ind & "---" & linefeed
   else
    set s to ind & n
    set c to item i of cs
    if c is not missing value and c is not "" then
     set md to item i of ms
     set s to s & "  [" & my modstr(md) & c & "]"
    else
     set g to item i of gl
     if g is not missing value then set s to s & "  [glyph " & g & " mod " & (item i of ms) & "]"
    end if
    if item i of mk is not missing value then set s to s & "  ✓"
    if not (item i of es) then set s to s & "  (disabled)"
    set subs to {}
    try
     set subs to every menu of (item i of mi)
    end try
    if (count of subs) > 0 then
     set r to r & s & " >" & linefeed
     if d > 0 then set r to r & my walk(item 1 of subs, ind & "    ", d - 1)
    else
     set r to r & s & linefeed
    end if
   end if
  end repeat
 end tell
 return r
end walk
on modstr(md)
 -- bit0 shift, bit1 option, bit2 control, bit3 NO cmd
 set s to ""
 if (md div 4) mod 2 = 1 then set s to s & "Ctrl+"
 if (md div 2) mod 2 = 1 then set s to s & "Opt+"
 if md mod 2 = 1 then set s to s & "Shift+"
 if (md div 8) mod 2 = 0 then set s to s & "Cmd+"
 return s
end modstr

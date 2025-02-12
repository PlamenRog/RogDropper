# RogDropper
Independant eye dropper tool


Dependencies:
 - GCC(tested on 14.2, but should work with previous ones)
 - (lib32-)libx11
 - (lib32-)libxfixes

Compile it with:
```make
make install
```
<pre>


</pre>
Recommended way to run is through some script like:
```bash
rogdrop | xclip
```
so that you don't need to open a seperate terminal everytime you use the program and instead just want to save the output to the clipboard directly. The choice is yours of course.

TODO:
 - Fix small graphical bugs

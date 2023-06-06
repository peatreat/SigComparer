# SigComparer

This program will find all the possible unique signatures in the biggest file out of the 2 that you input and it will see how many of those exist in the other file. 

----

## Usage

-s | The scan size of each signature in bytes. The bounds are from 1 to 64 | Default: 16

-m | The mode of the scan. 0 : dynamic, 1 : light scan | Default: 0

-o | Enable/Disable outputing to a file. 0 : disabled, 1 : enabled | Default: 1

-sort | The sorting for the output file. 0 : size desc, 1 : size asc, 2 : address desc, 3: address asc | Default: 0

sigcomparer.exe [-m 0:1] [-s 1-64] [-out 0:1] [-sort 0-3] file1.ext file2.ext

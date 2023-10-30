# pkmn-bw-script-tools
Original C code. Don't use this. \
``printscript``: reads a script file(a/0/5/7) and outputs the known commands.\
usage: ``printscript filename``

``offsetinc``: for increasing all jump/if/movement offsets from the start of the file to ``stopoffset`` by ``amount``, if jumping past ``stopoffset``.\
e.g. if you inserted commands into the file, causing existing commands to point to the wrong location in the file\
usage: ``offsetinc filename stopoffset amount``\
**specify stopoffset and amount in hex**

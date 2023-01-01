# The Telnet Productivity App
A telnet server. For a "productivity" app: primarily, this is a way for Linux enthusiasts to waste time

Basic idea-
Go in with telnet. It asks for a space ID, which will probably be a lowercased and dashed version of the space name. It asks for a username or password. Login that way.  
The owner of the space will login as `root` and will be able to create new users - if the user you login as has been newly created, it will ask for a password to use.

## Building the server:
If on Linux, run `./build.sh`. Dunno what C++ version we're gonna use yet but probably 17 or 20 - if you don't have the right standard library installed, fix that

If on Windows, get Linux. If you're reading this nobody has put any effort into supporting Windows. It doesn't surprise me.

If on Mac, get Linux or alter `./build.sh` to work for Mac. If you're reading this, nobody has put much effort into supporting Mac.

## Using the client:
It's a telnet server. All operating systems can connect to it - on Linux, just the `telnet` command, on Windows PuTTY, and on Mac probably `telnet` too but I don't know.

## Wait, why does this exist?
Because we're Linux users who like C/C++ and Telnet and have nothing better to do, apparently.

## Credits + Legal Garbage
Initial concept by Tyler Clarke (@LinuxRocks2000)  
Developed by Tyler, Jakson Clyde (@1000Dimensions), and Luke (@lukewhite32). And probably others.

Totally open and unlicensed. It would be kind of you to give credit if you use our code, but we won't sue you if you don't.
If we do end up caring, we'll change this and add an actual License file.

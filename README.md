# SpadesX

## What is SpadesX and why ?
TL;DR Ace of Spades server written in C (Currently in full on development and thus early Alpha)
Now if you would like to know the full picture read bellow:

SpadesX is project started by 3 people.
We love Ace of Spades (Mainly the open source alternative OpenSpades) but we hated the fact that the only type of
server that was there for it was Python based. 
While thats not the worst thing in the world it did give us a lot of headaches when managing our servers.
Some scripts were just too much for it to handle and it would start lagging. And in FPS game thats not ideal.
So we decided why not make it in C ?
And thats what we did. That is what SpadesX is. Ace of Spades server fully written in C.

## Why the SpadesX name ?
Honestly we dont know. We just one day decided to brainstorm the name and decide on something that sounded cool
yet still had the original naming.
There we truly a lot of great names thrown around. But SpadesX was one day thrown into the mix and we just knew this is IT

## OK thats all great but i want to test it. How do i test it ?????
Rather simple. Are you on Unix based system ? You are in luck.

> git clone https://github.com/SpadesX/SpadesX

> cd SpadesX

> git submodule init && git submodule update

> mkdir build && cd build
 
> cmake ..
 
> make -jX (Replace X with number of threads you have on your system)

Windows users will have to use WSL.

## HEY DEVELOPERS I HAVE A BUG. WHERE DO I REPORT IT ?
Ok Ok calm down first of all.
All we need from you is to open an Issue in this repo. Fully explaining your bug.
Beware tho that it may take days or even weeks before we fix the issue.
While we would love to fix issues all day (OK maybe not that actually sounds horrible) we are people too with our lives too.
Just like you we have jobs or other things we have to take care of. Please remember this before you start yelling at us why we cant fix your bug fast enough.
THIS IS WHY.

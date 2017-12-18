## Introduction
`session` is a small utility tool that enables user to save context-specific
variables around the system. It enhances build toolchains by allowing to save
data bound to a specific _directory_ rather than terminal shell instance.

This is a WIP version, still lacking of features.

## Installation
The program is written in C++14 and uses `g++` for compilation. `Makefile`
scripts are used for building.
Make sure you also have `sqlite3` and `boost-program-options` development
packages installed.

After all requirements are met, simply execute:

    make session

No system installation currently supported.

## Usage
No stable interface defined, all the flags and usages are subject to change.
Please consult `./session --help` for details on current version usage.

## Motivation
Let's say you are developing an embedded system for a set of different devices.
You have different versions of the software branched out in different
directories. Each such version is targeted at a specific hardware platform and
devices might share this platform internally, although requiring additional
tuning in variables, so same-platform builds might not be compatible between
same-platform devices.

Directory structure:

    <root> ./
      project-platform1/
      project-platform2/

`platform1` supports devices `A`, `B` and `C`. `platform2` supports devices `D`
and `E`.

You decide you will now do something on device `B`. So you `cd project-platform1`
and start setting up the environment, then you connect device to your computer,
you hack around the code, build the project and deploy it to your device.

But how do you tell the environment you will now develop and test on device `B`
and what's its identifier/serial port address? You have two choices:

1. Save device name/address as environment/global variable
2. Specify device name/address every time you build/deploy the project


**Ad. 1.** It might work just fine _sometimes_. But such variables won't be
persisted between shell sessions, so the moment you want to do something
in another terminal window, you'd need to setup the whole environment once more.
The same if you're interrupted and you PC crashes. Another setup needed.

When you end you tweaking and `cd` anywhere else, project variables might still
be here, depending on your terminal usage. So when later you perform some
project-related actions, your variables might interfere in any way possible.

The problem here is the uncontrollable persistence of data. It could easily be
avoided if project variables are bound to project directory. You could integrate
that into toolchain scripts, writing all the features by yourself... Or just use
`session`.

**Ad. 2.** This seems easy. Instead of saving device info anywhere you just
specify it when needed.

But... Are you **really** gonna do that? Wouldn't you just rather save the data
into some variables and use those variables in toolchain commands? Or maybe
you'd like to write yourself a script that does that for you? What about
shell history? It surely helps solve the problem, isn't it?

Yes, but maybe only partially. The truth is that as manageable it might be,
it's not the most comfortable scenario. Your commands might get long, you might
need to specify quite some info with each command... Why not just bind project variables to project directory?

So I've started writing `session` to tacle those minor usability issues. Maybe
someone one day will find that useful.

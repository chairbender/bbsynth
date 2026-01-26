# BBSynth

A virtual analog synth attempting to implement and compare a variety of
virtual analog synthesis techniques. It's also my first ever
attempt at a virtual instrument project in general, so it lets me
hack away and implement whatever I'm curious about at the moment.

For example, how bad does it sound to have no anti-aliasing vs
minBlep vs oversampling? How does it sound when our filter
has 1-sample delay vs zero delay? Those are all questions I'm curious
about.

I suppose it's also a decent (amateurish) collection of virtual analog
synthesis techniques all in one place, for those looking to
implement their own.

Techniques covered:
- OTA Filter emulation
- Anti-aliasing
- Analog Envelope Generators
- Anti-aliased Hard sync
- Basic FM (jupiter 8 calls it "cross mod")
- Other filter model emulations (Moog ladder filter, etc...)
- Analog imperfection emulation (voice dispersion, etc...) (coming soon)
- Skeumorphic UI using Blender + image strips (coming soon)

I have always been curious about how different virtual analog techniques
compare to each other, and this project is an attempt to do just that.
There are also various fine-tuning parameters that are often
hidden in commercial synths, which I would like to expose.

Are these even correct implementations of the techniques?
I don't know. Take them with a grain of salt. Do they
sound interesting and fairly correct to me? Yes. (Note I also have
pretty severe tinnitus...)

The general starting reference was the Jupiter 8 synth because 
it's fairly simple in terms of features (ignoring the sequencing and
keyboard configuration and the fact that, well, it's an analog synth and
we're in the digital realm) 
and allows me to focus on the synthesis techniques. Also, there
are a few patches on the original that I quite like.
(No, I don't have one of my own.)

Made with JUCE (hence the AGPLv3 license).

# Setup Notes

on PopOS, with clang++-20, had to sudo apt install g++ libstdc++-12-dev

style guide
https://google.github.io/styleguide/cppguide.html

# 🐺 WolfSound's Audio Plugin Template

![Cmake workflow success badge](https://github.com/JanWilczek/audio-plugin-template/actions/workflows/cmake.yml/badge.svg)

Want to create an audio plugin (e.g., a VST3 plugin) with C++ but don't know how to go about?

Heard about the [JUCE C++ framework](https://github.com/juce-framework/JUCE) but not sure how to start a JUCE project?

Want to use CMake with JUCE but don't know how?

Want to be able to easily integrate third-party C++ libraries to your project?

Want to unit test your audio plugin?

Want to ensure maximum safety of your software?

And all this with a click of a button?

Well, this template allows you to immediately start your JUCE C++ framework audio plugin project with a CMake-based project structure. It involves

* clear repo structure
* C++ 23 standard
* effortless handling of third-party dependencies with the CPM package manager; use the C++ libraries you want together with JUCE
* highest warning level and "treat warnings as errors"
* ready-to-go unit test project with GoogleTest

Additionally

* continuous integration made easy with Github actions: build and run tests on the main branch and on every pull request
* automatic clang-format on C++ files run on every commit; don't worry about code formatting anymore!

I am personally using this template all the time.

Feel free to propose suggestions 😉

## Usage

This is a template repository which means you can right click "Use this template" on GitHub and create your own repo out of it.

After cloning it locally, you can proceed with the usual CMake workflow.

In the main repo directory execute

```bash
$ cmake -S . -B build
$ cmake --build build
```

The first run will take the most time because the dependencies (CPM, JUCE, and googletest) need to be downloaded.

Alternatively, you can use bundled CMake presets:

```bash
$ cmake --preset default # uses the Ninja build system
$ cmake --build build
$ ctest --preset default
```

Existing presets are `default`, `release`, and `Xcode`.

To run clang-format on every commit, in the main directory execute

```bash
pre-commit install
```

(for this you may need to install `pre-commit` with `pip`: `pip install pre-commit`).

Don't forget to change "YourPluginName" to, well, your plugin name everywhere 😉

## How was this template built?

See how I create this template step by step in this video:

[![Audio plugin template tutorial video](http://img.youtube.com/vi/Uq7Hwt18s3s/0.jpg)](https://www.youtube.com/watch?v=Uq7Hwt18s3s "Audio plugin template tutorial video")

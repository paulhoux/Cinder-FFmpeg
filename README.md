Cinder-FFmpeg
================

**This block is EXPERIMENTAL!**


Cinder block for playing video using FFmpeg's Libavcodec and the OpenAL libraries


Supports most video formats, including multi-track audio. Does NOT support video without audio tracks yet.


Included is the v4.0 Windows distribution of Libavcodec and OpenAL 1.1. Please refer to the [FFmpeg website](http://www.ffmpeg.org/) for more information on licensing.


##### Known issues
* To successfully compile the application, define `__STDC_LIMIT_MACROS` and `__STDC_CONSTANT_MACROS`.
* To successfully link the application, set "Image Has Safe Exception Handlers" to "No" in Configuration Properties > Linker > All Options.
* Can't play files without at least one audio track. Synchronization depends on audio.
* Seeking is supported, but scrubbing is not. After a seek, the video may be distorted until the next keyframe.


##### Adding this block to Cinder
This block is meant to be used with version 0.9.1 of Cinder. See for more information on how to obtain this version:
http://libcinder.org/download/


##### Creating a sample application using Tinderbox
For more information on how to use the block with TinderBox, please follow this link:
http://libcinder.org/docs/welcome/TinderBox.html


##### Copyright notice and acknowledgements
Copyright (c) 2010-2018, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Portions of this code: (c) [Dirk van den Boer](https://code.google.com/p/glover/). Portions of this code based on [Zeranoe's FFmpeg build for Windows](http://ffmpeg.zeranoe.com/). 


Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and	the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

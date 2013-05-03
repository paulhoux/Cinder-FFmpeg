Cinder-FFmpeg
================

Cinder block for playing video using FFmpeg's Libavcodec and the OpenAL libraries


Included is the Windows distribution of Libavcodec (the version of November 25th, 2012) and OpenAL 1.1. Please refer to the [FFmpeg website](http://www.ffmpeg.org/) for more information on licensing. As a special warning, it should be noted that FFmpeg is not available under any other licensing terms, especially not proprietary/commercial ones, not even in exchange for payment.


#####Known issues
* Can't play audio in floating point format, most notably the Microsoft WMV-format. Resampling is required, which is not yet implemented.
* Can't play files without at least one audio track. Synchronization depends on audio. Will be fixed later.
* Uploading the video texture to the GPU via DMA is not yet implemented and therefor not yet optimized.


#####Adding this block to Cinder
This block is meant to be used with version 0.8.5 of Cinder. See for more information on how to obtain this version:
http://libcinder.org/download/

Cinder's tool for setting up empty projects, TinderBox, has been revamped and now supports a neat system for the management of plug-ins called Cinder Blocks. This Video block supports this new feature. To add this block to your copy of Cinder, so it will be detected automatically by TinderBox:
* Open a command window (or Terminal)
* Switch to the disk containing the Cinder root folder, e.g.: ```d:```
* Browse to the Cinder root folder: ```cd path\to\cinder_master```
* Add the Video block to the blocks folder: ```git clone https://github.com/paulhoux/Cinder-LibAV.git blocks/LibAV```

Alternatively, you can download the repository as a [ZIP-file](https://github.com/paulhoux/Cinder-LibAV/zipball/master) and manually add the files to a "cinder_master\blocks\Video" folder.


#####Creating a sample application using Tinderbox
For more information on how to use the block with TinderBox, please follow this link:
http://libcinder.org/docs/welcome/TinderBox.html


#####Copyright notice and acknowledgements
Copyright (c) 2010-2013, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Portions of this code: (c) [Dirk van den Boer](https://code.google.com/p/glover/). Portions of this code based on [Zeranoe's FFmpeg build for Windows](http://ffmpeg.zeranoe.com/). 


Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and	the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

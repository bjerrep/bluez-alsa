# Bluetooth Audio ALSA Backend 
## bluez-alsa fork for direct audio [![Build Status](https://travis-ci.org/bjerrep/bluez-alsa.svg?branch=master)](https://travis-ci.org/bjerrep/bluez-alsa)


This is a modification of bluez-alsa which, when running as an a2dp sink, is able to send audio directly to a fifo file as PCM, RTP or as the raw codec stream.

It is made for a setup where bluez-alsa is running on a raspberry pi server that forwards audio to a number of playing raspberry pi clients. The direct audio operation decreases the number of required transcodings by one and lets the clients do the decoding where it is needed.

A new command line switch is introduced, **--a2dp-fifo=pcm|rtp|stream**. Currently it is only implemented for SBC, the android used for testing doesn't trigger AAC. (so configure without enabling AAC for now)

**pcm** Perhaps not terribly interesting. Delivering PCM to ALSA is what bluez-alsa does out of the box.

**rtp** The RTP stream received over a2dp. This will carry information about the used codec.

**stream** The RTP payload. The raw codec data where the RTP header is stripped. If more codecs than SBC is supported then this mode will be a little problematic. 

## A quick spin

First make the fifo (name is hardcoded):

    mkfifo /tmp/audio

_running in PCM mode_:

    bluealsa -p a2dp-sink --a2dp-fifo=pcm
    cat /tmp/audio | gst-launch-1.0 fdsrc fd=0 ! audio/x-raw,format=S16LE,channels=2,rate=44100,layout=interleaved ! audioconvert ! autoaudiosink

_running in RTP mode_:

    bluealsa -p a2dp-sink --a2dp-fifo=rtp
unsuccessful with gstreamer so far, even with the codec fixed to SBC. Tried a.o. "rtpsbcdepay | sbcparse | sbcdec". Not sure if gstreamer, the pipelines tried or the new code is to blame.
    
_running in stream mode_:

    bluealsa -p a2dp-sink --a2dp-fifo=stream
    cat /tmp/audio | gst-launch-1.0 fdsrc fd=0 ! queue2 use-buffering=true use-tags-bitrate=true ! sbcparse ! sbcdec ! autoaudiosink
    
    
## V2: Added AAC
    
AAC now runs in stream mode (not PCM or RTP)

    bluealsa -p a2dp-sink --a2dp-fifo=stream
    cat /tmp/audio | gst-launch-1.0 fdsrc fd=0 ! queue2 use-buffering=true use-tags-bitrate=true ! aacparse ! avdec_aac ! autoaudiosink
    
A2DP AAC uses a latm header that seems impossible for anyone to grasp except for the fdk-acc decoder natively used by bluez-alsa. For AAC the latm header is replaced by a adts header. This is a bad implementation of a bad solution but for now it seems to do the trick, everyone from gstreamer to mediainfo is now happy. This fork is beyond repair anyway.

## V2: Added inband signalling
Another easteregg is that apple uses inband signaling of volume. Volume on the (sbc) android was done on the mobile before encoding so that worked out of the box. By adding the argument "--inband" the following ascii packet headers / signals are used, at least in stream mode:

    ::volume=i::		0 <= i <= 127
    ::mute=i::			i = 0 | 1
    ::state=stop::
    ::codec=s::			sbc | aac
    ::audio=len::[len audio bytes]
    
Using inband means that the only recipient that will be able to understand this will have to be homemade.

## V2: Added buffering  
Audio data are now buffered into blocks of about 4kbytes before they shipped. A raspberry pi seemed to choke on the flood of individual rtp payloads.

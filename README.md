Balls
=====

This reads values from the trackballs and scroll rings of three Kensington USB trackballs, keeps track of their state, and pushes that state to a Levels adjustment layer in Photoshop.

It uses hidapi from http://www.signal11.us/oss/hidapi/, and the pscryptor class from Adobe's Connection SDK.

It's lacking any kind of error checking or configurability, but it works pretty reliably.

Performance is abysmal but that seems to be a hallmark of Photoshop's scripting API.

xo


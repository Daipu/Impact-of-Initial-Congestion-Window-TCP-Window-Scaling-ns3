# Impact-of-Initial-Congestion-Window-TCP-Window-Scaling-ns3

## Course Code: CS822

## Assignment: #GP7

## Overview

There are numerous strategies that are deployed to control the congestion in the network. One of the most early effort was to put a sender-side limit on how much data that would be sent at one time. This limit is congestion window (cwnd). On start of any new connection the sender initializes a new congestion window (cwnd) variable per TCP connection and sets its initial value to a conservative, system-specified value (initcwnd on Linux). The cwnd value increase with each ACK received at sender side. Originally, the cwnd start value was set to 1 network segment; [1] updated this value to 4 segments; again [2] suggested the value to be increased to 10 segments.

The original TCP specification allocated 16 bits for advertising the receive window size, which places a hard upper bound on the maximum value (2^16, or 65,535 bytes) that can be advertised by the sender and receiver. In networks with high bandwidth delay product, this upper bound is often not enough to get optimal performance. [3] was drafted to provide a "TCP window scaling" option, which allows us to raise the maximum receive window size from 65,535 bytes to 1 gigabyte.

This repository provides an implementation of a point-to-point and dumbbell topology and showcases effect of initial congestion window and window scaling on them.

###Initial Congestion Window TCP Window Scaling example

An example program is stored in:

`src/point-to-point/examples/initcwnd-wndscale-example.cc`

and should be executed as

`./waf --run="initcwnd-wndscale-example --topology='S or D' --leafNodes=l --maxBytes=mb --Host_bandwidth=hb --Host_delay=hd --Link_bandwidth=lb --Link_delay=ld --wnd_scaling=ws --sack=s --duration=d --InitCwnd=cw"`

e.g.:

`./waf --run="initcwnd-wndscale-example --topology='S' --leafNodes=5 --maxBytes=500000 --Host_bandwidth=1Gbps --Host_delay=5ms --Link_bandwidth=50Mbps --Link_delay=20ms --wnd_scaling=true --sack=true --duration=5s --InitCwnd=10"`

All these parameters are optional.

References

[1] RFC 2581 - TCP Congestion Control. https://tools.ietf.org/html/rfc2581

[2] RFC 6928 - Increasing TCP's Initial Window. https://tools.ietf.org/html/rfc6928

[3] RFC 1323 - TCP Extensions for High Performance. https://tools.ietf.org/html/rfc1323

[4] http://www.nsnam.org/

# mahimahi: a web performance measurement toolkit

For general information, see http://mahimahi.mit.edu/.

This is a fork of the mahimahi source code from
https://github.com/ravinet/mahimahi. 

Two features have been added over the base mahimahi tool:

(1) a new shell, `mm-drop`, which drops every N'th packet
deterministically;

(2) a new shell, `mm-corrupt`, which corrupts every N'th
packet deterministically. Corruption can be made to occur
with or without preserving the (TCP/UDP) checksum of the
overall packet.

## Installation from source

Currently, it is only possible to install this fork from
source. (Deb packages will reflect the base mahimahi tool;
they won't have the shell functions mentioned above.)

### Install dependencies

On a fresh Ubuntu 20.04 LTS install (kernel
5.4.0-100-generic), you can run

```
sudo apt install libprotobuf-dev autotools-dev dh-autoreconf \
  iptables pkg-config dnsmasq-base apache2-bin debhelper \
  libssl-dev ssl-cert libcairo2-dev libpango1.0-dev \
  libxcb1-dev protobuf-compiler
```

The full list of dependencies is available at
http://mahimahi.mit.edu/#getting

### Compiling

```
git clone https://github.com/novel-scheduler/mahimahi.git
cd mahimahi
./autogen.sh
./configure
make
sudo make install
```

## How to test the new shells

### Testing mm-drop

`mm-drop` is invoked by running `mm-drop uplink|downlink N`

Here `uplink|downlink` is the regular link direction argument of mahimahi.

The shell will drop every N'th packet in the link direction specified.

For example, once inside the shell, running the `ping` command should
show the corresponding result.

Here's an example result within the shell `mm-drop downlink 5`

```
node:~/mahimahi> ping google.com
PING google.com (142.250.190.110) 56(84) bytes of data.
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=2 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=3 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=4 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=6 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=7 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=8 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=9 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=11 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=12 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=13 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=14 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=16 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=17 ttl=115 time=14.4 ms
64 bytes from ord37s35-in-f14.1e100.net (142.250.190.110): icmp_seq=18 ttl=115 time=14.4 ms
^C
--- google.com ping statistics ---
18 packets transmitted, 14 received, 22.2222% packet loss, time 17104ms
rtt min/avg/max/mdev = 14.367/14.401/14.433/0.021 ms
```

### Testing mm-corrupt

`mm-corrupt` takes three arguments:

(1) `downlink|uplink` is the same as the regular link
direction argument in mahimahi.

(2) `chknotok` specifies that the packet will be corrupted
in a way that will change the checksum. `chksumok` specifies
that the packet will be corrupted in a way that preserves
its TCP/UDP checksum (assuming the packet is TCP/UDP).

(3) `N`: every N'th packet in the specified link direction
will be corrupted in the manner specified.

Here is an example result after invoking the corruption
shell `mm-corrupt uplink chksumok 5`. Inside the shell, we
could send UDP packets using scapy:

```
sudo scapy
## Inside the scapy terminal
send ( IP(dst="100.64.0.1")/UDP()/"123456789123456789" , iface="ingress", count = 10 ) 
```

Capturing packets in another shell (in the host's namespace
not within mahimahi) shows the data `123...` in the
packet is corrupted. tcpdump still thinks the checksum is OK,
but the data is different (look at the last few digits in the hex output). 

```
sudo tcpdump -i `ifconfig -a | grep corrupt | awk -F: '{print $1}'` -XAvvv udp
### [snip lots of output] 
### [original packet below]
21:26:41.670498 IP (tos 0x0, ttl 64, id 1, offset 0, flags [none], proto UDP (17), length 46)
    100.64.0.2.domain > 100.64.0.1.domain: [udp sum ok] 12594 op6+% [b2&3=0x3334] [14136a] [13622q] [14641n] [12851au][|domain]
        0x0000:  4500 002e 0001 0000 4011 b23b 6440 0002  E.......@..;d@..
        0x0010:  6440 0001 0035 0035 001a 57ee 3132 3334  d@...5.5..W.1234
        0x0020:  3536 3738 3931 3233 3435 3637 3839       56789123456789
### [corrupted packet below]
21:26:41.669570 IP (tos 0x0, ttl 64, id 1, offset 0, flags [none], proto UDP (17), length 46)
    100.64.0.2.domain > 100.64.0.1.domain: [udp sum ok] 12594 op6+% [b2&3=0x3334] [14136a] [13622q] [14641n] [12851au][|domain]
        0x0000:  4500 002e 0001 0000 4011 b23b 6440 0002  E.......@..;d@..
        0x0010:  6440 0001 0035 0035 001a 57ee 3132 3334  d@...5.5..W.1234
        0x0020:  3536 3738 3931 3233 3435 3638 3838       56789123456888
```

## Questions about this fork?

Please start a GitHub issue in this repo.


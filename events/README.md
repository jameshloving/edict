
Make sure the conntrack kernel module is installed (if it isn't built into the kernel):

   `modprobe ip_conntrack`

If you have the conntrack tools installed you should be able to watch conntrack events with

   `conntrack -E`

Additionally, make sure you have suitable IPtables rules in place:

   `iptables -A INPUT -m conntrack --ctstate NEW -j NFLOG --nflog-group 2`
   `ip6tables -A INPUT -m conntrack --ctstate NEW -j NFLOG --nflog-group 2`

The code in this directory is based entirely off an example program from:

https://github.com/threatstack/libnetfilter_conntrack

Note this does require linking with a GPL licensed library so this code has to be GPL licensed as well.





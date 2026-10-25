#!/usr/bin/env python3
"""Phase 5 tests: NET STATUS / INTERFACES / ROUTES / STATS / SOCKETS.

    test_phase5.py --host H --port P [--wait SECS]
Assumes the hosted harness layout: eth0 = 10.0.0.2/24 on tap.device, lo0 loopback.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_phase1 import Conn, check, results, wait_for_agent  # noqa: E402
from test_phase3 import fields, records  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=7777)
    ap.add_argument("--wait", type=int, default=0)
    a = ap.parse_args()
    if a.wait and not wait_for_agent(a.host, a.port, a.wait):
        return 1

    c = Conn(a.host, a.port)

    r = c.command("NET STATUS")
    f = fields(r)
    check("NET STATUS block", r[0] == "OK" and r[-1] == "END", repr(r[:4]))
    check("NET STATUS version/release", f.get("BSDSOCKET_VERSION", "0").split(".")[0].isdigit() and f.get("STACK_RELEASE"), repr(f))
    check("NET STATUS hostname", bool(f.get("HOSTNAME")), repr(f.get("HOSTNAME")))
    check("NET STATUS counts", int(f.get("INTERFACES", 0)) >= 2 and int(f.get("ROUTES_INET", 0)) >= 1, repr(f))
    check("NET STATUS stats available via miami.library", f.get("STATS") == "available", repr(f.get("STATS")))
    print("INFO  DNS_COUNT=%s DOMAIN=%s" % (f.get("DNS_COUNT"), f.get("DOMAIN")))

    r = c.command("NET INTERFACES")
    f = fields(r)
    ifs = {x["NAME"]: x for x in records(r, "IFACE")}
    check("NET INTERFACES block", r[0] == "OK" and r[-1] == "END" and len(ifs) == int(f.get("COUNT", -1)), repr(list(ifs)))
    eth = ifs.get("eth0", {})
    lo = ifs.get("lo0", {})
    check("eth0 present with harness address", eth.get("ADDR") == a.host and eth.get("NETMASK") == "255.255.255.0" and eth.get("BROADCAST") == "10.0.0.255", repr(eth))
    check("eth0 flags decoded", "UP" in eth.get("FLAGNAMES", "") and "RUNNING" in eth.get("FLAGNAMES", "") and "BROADCAST" in eth.get("FLAGNAMES", ""), eth.get("FLAGNAMES"))
    check("eth0 mtu and hwaddr", eth.get("MTU") == "1500" and len(eth.get("HWADDR", "").split(":")) == 6, repr((eth.get("MTU"), eth.get("HWADDR"))))
    check("eth0 state up", eth.get("STATE") == "up")
    check("lo0 is loopback", "LOOPBACK" in lo.get("FLAGNAMES", "") and lo.get("ADDR") == "127.0.0.1", repr(lo))
    rx0, tx0 = int(eth.get("RXBYTES", 0)), int(eth.get("TXPKT", 0))
    for _ in range(30):
        c.command("PING")
    eth2 = {x["NAME"]: x for x in records(c.command("NET INTERFACES"), "IFACE")}.get("eth0", {})
    check("eth0 counters advance", int(eth2.get("RXBYTES", 0)) > rx0 and int(eth2.get("TXPKT", 0)) >= tx0 + 30, "rxbytes %s -> %s, txpkt %s -> %s" % (rx0, eth2.get("RXBYTES"), tx0, eth2.get("TXPKT")))
    print("INFO  eth0 RXPKT=%s (if_sana.c counts received bytes but never if_ipackets; stack accounting gap)" % eth2.get("RXPKT"))

    r = c.command("NET ROUTES")
    f = fields(r)
    routes = records(r, "ROUTE")
    check("NET ROUTES block", r[0] == "OK" and r[-1] == "END" and len(routes) == int(f.get("COUNT", -1)) and len(routes) >= 1, repr(f))
    net = [x for x in routes if x["DST"] == "10.0.0.0" and x["MASK"] == "255.255.255.0"]
    check("10.0.0.0/24 interface route present", net and "U" in net[0]["FLAGNAMES"] and "G" not in net[0]["FLAGNAMES"], repr(net))
    lo_r = [x for x in routes if x["DST"] == "127.0.0.1"]
    check("loopback host route", lo_r and lo_r[0]["MASK"] == "host" and "H" in lo_r[0]["FLAGNAMES"], repr(lo_r))
    r = c.command("NET ARP")
    f = fields(r)
    arp = {x["ADDR"]: x for x in records(r, "ARP")}
    check("NET ARP block", r[0] == "OK" and r[-1] == "END" and len(arp) == int(f.get("COUNT", -1)), repr(f))
    peer = arp.get("10.0.0.1", {})
    check("peer 10.0.0.1 in ARP table with a MAC", len(peer.get("HWADDR", "").split(":")) == 6 and "COMPLETE" in peer.get("FLAGNAMES", ""), repr(peer))
    check("route fields numeric", all(x["FLAGS"].startswith("0x") and x["USE"].isdigit() and x["IFINDEX"].isdigit() for x in routes))
    print("INFO  routes: %s" % ", ".join("%s/%s->%s[%s]" % (x["DST"], x["MASK"], x["GW"], x["FLAGNAMES"]) for x in routes))

    r = c.command("NET STATS")
    f = fields(r)
    check("NET STATS block with all three protocols", r[0] == "OK" and all(k in f for k in ("IP_TOTAL", "TCP_ACCEPTS", "UDP_IPACKETS")), repr(list(f)[:8]))
    check("NET STATS plausible", int(f.get("IP_TOTAL", 0)) > 0 and int(f.get("TCP_ACCEPTS", 0)) >= 1 and int(f.get("TCP_RCVPACK", 0)) > 0, repr({k: f[k] for k in ("IP_TOTAL", "TCP_ACCEPTS", "TCP_RCVPACK") if k in f}))
    acc0 = int(f.get("TCP_ACCEPTS", 0))
    c.command("QUIT"); c.close()
    for _ in range(3):
        c = Conn(a.host, a.port); c.command("PING"); c.command("QUIT"); c.close()
    c = Conn(a.host, a.port)
    f2 = fields(c.command("NET STATS TCP"))
    check("NET STATS TCP subset", "TCP_ACCEPTS" in f2 and "IP_TOTAL" not in f2 and "UDP_IPACKETS" not in f2)
    check("TCP_ACCEPTS advanced by the reconnects", int(f2.get("TCP_ACCEPTS", 0)) >= acc0 + 4, "%d -> %s" % (acc0, f2.get("TCP_ACCEPTS")))
    f3 = fields(c.command("NET STATS ip"))
    check("NET STATS IP subset (case-insensitive)", "IP_TOTAL" in f3 and "TCP_ACCEPTS" not in f3)
    f4 = fields(c.command("NET STATS UDP"))
    check("NET STATS UDP subset", "UDP_IPACKETS" in f4 and "UDP_NOPORT" in f4)

    for bad in ("NET", "NET BOGUS", "NET STATS ICMP"):
        rr = c.command(bad)
        check("%r -> ERR 400" % bad, len(rr) == 1 and rr[0].startswith("ERR 400 "), repr(rr))
    rr = c.command("NET SOCKETS")
    check("NET SOCKETS -> ERR 501 (documented as unsupported)", len(rr) == 1 and rr[0].startswith("ERR 501 "), repr(rr))

    ok = True
    for _ in range(10):
        for cmd in ("NET STATUS", "NET INTERFACES", "NET ROUTES", "NET ARP", "NET STATS"):
            rr = c.command(cmd)
            if rr[0] != "OK" or rr[-1] != "END":
                ok = False
    check("10x NET commands stable", ok)
    check("agent still answers PING", c.command("PING") == ["OK PONG"])
    c.command("QUIT")
    c.close()

    failed = [n for n, ok, _ in results if not ok]
    print("\n%d tests, %d failed" % (len(results), len(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

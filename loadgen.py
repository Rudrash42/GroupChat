#!/usr/bin/env python3
import argparse, asyncio, time
from statistics import mean

def pct(vals, p):
    if not vals: return float("nan")
    vals = sorted(vals); k = (len(vals)-1)*p/100
    f = int(k); c = min(f+1, len(vals)-1)
    return vals[f] if f==c else vals[f]*(c-k)+vals[c]*(k-f)

class Client:
    def __init__(self, i, host, port, join, room, upref, ping, pong, rate, dur, extra):
        self.i=i; self.host=host; self.port=port; self.join=join
        self.room=room; self.user=f"{upref}{i}"; self.ping=ping; self.pong=pong
        self.rate=rate; self.dur=dur; self.extra="x"*max(0,extra)
        self.r=None; self.w=None; self.lat=[]; self.sent=0; self.recv=0

    async def connect(self):
        self.r, self.w = await asyncio.open_connection(self.host, self.port)
        if self.join:
            msg = self.join.format(room=self.room, user=self.user).rstrip("\n")+"\n"
            self.w.write(msg.encode()); await self.w.drain()

    async def sender(self):
        per = 1.0/self.rate if self.rate>0 else 0
        end = time.monotonic()+self.dur; next_t = time.monotonic()
        while time.monotonic()<end:
            tns = time.time_ns()
            line = f"{self.ping} {self.i} {tns} {self.extra}\n"
            self.w.write(line.encode()); await self.w.drain()
            self.sent += 1; next_t += per
            delay = next_t - time.monotonic()
            if delay>0: await asyncio.sleep(delay)

    async def receiver(self):
        while True:
            line = await self.r.readline()
            if not line: break
            parts = line.decode(errors="ignore").strip().split()
            if len(parts)>=3 and parts[0]==self.pong and int(parts[1])==self.i:
                try: ts=int(parts[2]); self.lat.append((time.time_ns()-ts)/1e6); self.recv+=1
                except: pass

    async def run(self):
        await self.connect()
        recv = asyncio.create_task(self.receiver())
        send = asyncio.create_task(self.sender())
        await send; await asyncio.sleep(1); recv.cancel()
        try: self.w.close(); await self.w.wait_closed()
        except: pass

async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=5000)
    ap.add_argument("--clients", type=int, default=20)
    ap.add_argument("--rate", type=float, default=20.0, help="msgs/sec per client")
    ap.add_argument("--duration", type=float, default=10.0)
    ap.add_argument("--msg-size", type=int, default=0)
    ap.add_argument("--join", default="", help='e.g. "JOIN {room} {user}"')
    ap.add_argument("--room", default="room1")
    ap.add_argument("--user-prefix", default="u")
    ap.add_argument("--ping", default="PING")
    ap.add_argument("--pong", default="PONG")
    a = ap.parse_args()

    cs = [Client(i,a.host,a.port,a.join,a.room,a.user_prefix,a.ping,a.pong,
                 a.rate,a.duration,a.msg_size) for i in range(a.clients)]
    t0=time.monotonic(); await asyncio.gather(*(c.run() for c in cs)); wall=time.monotonic()-t0
    sent=sum(c.sent for c in cs); recv=sum(c.recv for c in cs)
    print("=== Loadgen summary ===")
    print(f"clients={a.clients} duration={a.duration}s rate={a.rate} msg_size={a.msg_size}")
    print(f"sent={sent} recv={recv} wall={wall:.2f}s thr_sent={sent/wall:.1f} thr_recv={recv/wall:.1f} msg/s")
    all_lat=[x for c in cs for x in c.lat]
    if all_lat:
        print(f"latency_ms: p50={pct(all_lat,50):.2f}  p95={pct(all_lat,95):.2f}  p99={pct(all_lat,99):.2f}  avg={mean(all_lat):.2f}")
    else:
        print("no latency samples; ensure server echoes 'PONG <id> <ts>' per 'PING <id> <ts>'")

if __name__ == "__main__":
    asyncio.run(main())



# python3 loadgen.py --host 127.0.0.1 --port 5000 \
#   --clients 20 --rate 20 --duration 10 \
#   --ping PING --pong PONG

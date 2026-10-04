# Netscan

A Linux command-line tool for TCP port scanning and ICMP network discovery.

---

## Features

- Scan an inclusive TCP port range on an IPv4 address.
- Report open ports and scan summary.
- Probe IPv4 subnets using CIDR notation and ICMP echo requests.

---

## Build

Requires GCC and Make on Linux.

```sh
make
```

---

## Usage

Scan TCP ports:

```sh
./netscan -H 192.168.1.10 -p 1-1024
```

Scan a single port:

```sh
./netscan -H 192.168.1.10 -p 80-80
```

Probe a subnet (requires root or `CAP_NET_RAW`):

```sh
sudo ./netscan -n 192.168.1.0/24
```

---

## Technical Details

### TCP scanning (`scan.c`)

- Uses nonblocking TCP connections and `select()` to detect completion.
- Limits concurrent sockets to the system and `select()` limits; starts remaining ports as sockets become available.
- Waits up to five seconds per readiness check.

### ICMP probing (`ping.c`)

- Builds echo requests with a manually computed Internet checksum.
Locates the ICMP header using the IPv4 header length, then verifies it's an echo reply (type) whose ID matches the process ID, confirming it answers this program's probe.
- IPv4 stores the header length in 32-bit words; multiplying by four gives the byte offset of the ICMP header:


```c
struct iphdr *ipheader = (struct iphdr *)buffer;
size_t ipheader_size = ipheader->ihl * 4; 
struct icmphdr *reply = (struct icmphdr *)(buffer + ipheader_size);

if (reply->type == ICMP_ECHOREPLY &&
    reply->un.echo.id == (uint16_t)getpid()) {
  printf("Received %ld bytes from %s\n", received - ipheader_size, ip);
  return 1;
}
```

### Subnet handling (`main.c`)

- **`/32` — single host:** probes the supplied IPv4 address.
- **`/31` — point-to-point link:** probes both addresses, with no reserved network or broadcast address. Requires the lower, even address as input.
- **`/0`–`/30` — subnet:** calculates boundaries and probes addresses between the network and broadcast addresses:

```c
uint64_t nb_addrs = 1ULL << (32 - cidr);
uint32_t mask = 0xFFFFFFFF - (nb_addrs - 1);
uint32_t network = ip & mask;
uint32_t broadcast = network + nb_addrs - 1;

for (uint32_t i = network + 1; i < broadcast; i++) {
  ip_bintostr(buffer, sizeof(buffer), i);
  ping(buffer);
}
```

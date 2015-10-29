###Requirements for using c66x opencv
1. c66x CPU accelerator card made by one of several vendors<br />
2. Half-length or full-length PCIe card slot, depending on card type and vendor. Cards are full height, single slot thickness, x8 lanes, consume from 55 to 120W (power also depending on card type and vendor), and contain onboard NICs from 1 to 10 GbE<br />
3. Software from Signalogic: -c66x CPU code acceleration executable and x86 DirectCore library and drivers (for supported Linux, see below). For VM functionality, DirectCore virtIO drivers are additionally required

###Tested Linux and kernel versions
Ubuntu: 14.04, 14.10, 12.04<br />
CentOS: 7.1<br />
Kernel: 3.2.0, 3.13.0, 3.15.6

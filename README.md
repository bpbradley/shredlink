# Shred Link

Development repository for work related to an open source guitar controller adapter.

Work is primarily focused on wii guitars in the short term as a low cost, high performance
alternative to the raphnet adapter. 

Eventually, it will support more controller backends such as PS2 and direct GPIO, and more
generic gamepads rather than guitars specifically.

Regardless of the backend, shredlink enumerates to the host PC as a standard HID gamepad
so that it can be easily mapped in-game, just like the raphnet adapter. Eventually,
this will support bluetooth connection as well.

This firmware should remain hardware agnostic in the application layer, such that it can 
easily be adapted to any host processor, possibly even eventually supporting reflashing of 
off the shelf adapters.

## Getting Started

Shred Link is based on the Zephyr RTOS, so before getting started,
make sure to have a proper Zephyr development environment. You can follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder where this repository
and all Zephyr modules will be cloned. You can do
that by running:

```shell
# initialize workspace (main branch)
west init -m https://github.com/bpbradley/shredlink --mr main shredlink-workspace
# update Zephyr modules
cd shredlink-workspace
west update
```

### Build & Run

The application can be built by running:

```shell
west build -b $BOARD -s app
```

where `$BOARD` is the target board, `nrf52840dk_nrf52840` for example

A sample debug configuration is also provided. You can apply it by running:

```shell
west build -b $BOARD -s app -- -DOVERLAY_CONFIG=`configs/debug.conf`
```
Once you have built the application you can flash it by running:

```shell
west flash
```

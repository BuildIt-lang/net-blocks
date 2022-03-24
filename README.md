# Net-Blocks

Net-Blocks is a modular staged network protocol toolkit that uses [BuildIt](https://buildit.so) to generate efficient protocols and their implementations. Net-Blocks creates simple modules with individual network related features such as flow-identifiers, checksum, reliability, in-order delivery among others which the users can combine and parameterize to generate their very own protocol stack. 

Net-Blocks is currently WIP and new modules/features are being added

## Build and Test

To build Net-Blocks clone this repository and all its submodules - 

```
git clone --recursive https://github.com/BuildIt-lang/net-blocks.git
cd net-blocks
```

The framework can be built with the following command in the top-level directory

```
make -j$(nproc)
```


To build and run a simple test case that runs an echo client and server on the same server in separate processes, run - 

```
make simple_test
./build/test/simple_server &
./build/test/simple_client
```

If successful, the client and server will both print a hello message. If failed, you might have to kill the server background process. Usually you can do this by bring the server to the foreground with the `fg` command and then killing it with `Ctrl+c`. 

Please report bugs if any by creating an issue here. 


## Quick Start

### Building and Testing
Build docker with the command:
```sh
docker build -t mpc-lib .
```

Run docker:
```sh
docker run -it mpc-lib
```

Testing:
```sh
make run-tests
```

### Generate address and signature
Install package:
```sh
cd scripts && npm install
```

Replace `DERIVED_PUBLIC_KEY`, `r`, `s` variable in `index.js` file and run:
```sh
node index.js
```

Go to [Etherscan Verified Signatures](https://etherscan.io/verifiedSignatures) to verify the signature


## Usage

A few examples for running a full signing process can be found in the [tests section](https://github.com/fireblocks/mpc-lib/tree/main/test/cosigner)

## Security

Please see our dedicated [security policy](SECURITY.md) page.
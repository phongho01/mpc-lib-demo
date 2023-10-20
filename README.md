## Quick Start

### Building and Testing
Build docker with the command:
```sh
docker build -t mpc-lib-demo .
```

Run docker:
```sh
docker run -it mpc-lib-demo
```

Testing:
```sh
make run-tests
```

### Generate address and signature
Get `public key`, `message` and `signature` by running:
```sh
node scripts/verify.js
```

Or go to [Etherscan Verified Signatures](https://etherscan.io/verifiedSignatures) to verify the signature


## Usage

A few examples for running a full signing process can be found in the [tests section](https://github.com/fireblocks/mpc-lib/tree/main/test/cosigner)

## Security

Please see our dedicated [security policy](SECURITY.md) page.
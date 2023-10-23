## How to run
1. Build docker:
```sh
docker build -t mpc-cmp-demo .
```

2. Run docker:
```sh
docker run -it mpc-cmp-demo
```

3. Create transaction:
```sh
node scripts/create-transaction.js
```

4. Generate key share and sign transaction:
```sh
make run-tests
```

5. Submit transaction:
```sh
node scripts/submit-transaction.js
```

6. Copy `hash` property in `Transaction Receipt` object and check transaction on Polygon Testnet: https://mumbai.polygonscan.com
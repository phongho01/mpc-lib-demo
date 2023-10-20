const { ethers } = require('ethers');
const publicKeyToAddress = require('ethereum-public-key-to-address');

// VARIABLE
const MESSAGE = 'hello world';
const DERIVED_PUBLIC_KEY = '029baa2b1d7e0419650dc0b06297ea8ecb900d0d7ddaf802633941380ecef690dc';
const r = 'e475fc69c579b842b719be0c78771ba6dbf3ce03f1ec698c9cceebe102db7650';
const s = '4358a2fd2e41c01d33965e708143a210c2abeb2c27facee8197efec6a627d2b7';
const v = '1b'; // 1b

// Sign the string message
const getSignature = async () => {
  const hash = ethers.utils.hashMessage(MESSAGE);
  const ADDRESS = publicKeyToAddress(DERIVED_PUBLIC_KEY);
  const SIGNATURE = '0x' + r + s + v;

  console.log({
    MESSAGE,
    ADDRESS,
    SIGNATURE,
  });
};

getSignature();

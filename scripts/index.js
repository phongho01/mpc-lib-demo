const { ethers } = require('ethers');
const publicKeyToAddress = require('ethereum-public-key-to-address');

// VARIABLE
const MESSAGE = 'hello world';
const DERIVED_PUBLIC_KEY = '03b31cd1e0e7497cb0f4478277e3d00616155170e9f6b156faebb8c91e93ebed91';
const r = 'cd1c110d1b2a3be3e07d4c7b45c304bf34b22d027bb6eb30f5cffba7c479d5ee';
const s = '08f59b5d67e2dfdc5f94fb57ba1530a9392d964ddbbf90d0b60c7060ec521093';
const v = '1c'; // 1b

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

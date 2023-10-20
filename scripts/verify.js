const { ethers } = require('ethers');
const publicKeyToAddress = require('ethereum-public-key-to-address');
const SIGNATURE_RESULT = require('./signature_result.json');

// VARIABLE
const MESSAGE = 'hello world';
const PUBLIC_KEY = SIGNATURE_RESULT.public_key;
const r = SIGNATURE_RESULT.R
const s = SIGNATURE_RESULT.S;
const v = ['1c', '1b'];

// Sign the string message
const getSignature = async () => {
  const hash = ethers.utils.hashMessage(MESSAGE);
  const ADDRESS = publicKeyToAddress(PUBLIC_KEY);
  let result = {};
  for(let i = 0; i < v.length; i++) {
    const SIGNATURE = '0x' + r + s + v[i];
    const address = ethers.utils.recoverAddress(hash, SIGNATURE);
    if(address.toLowerCase() === ADDRESS.toLowerCase()) {
      result = {
        MESSAGE,
        ADDRESS,
        SIGNATURE,
      }
    }

  }

  console.log(result)
};

getSignature();

const { ethers } = require('ethers');
const publicKeyToAddress = require('ethereum-public-key-to-address');
const UNSIGNED_TX = require('./unsignedTx.json');
const SIGNATURE_RESULT = require('./signature_result.json');

const v = [160037, 160038];

// network variables
const RPC_PROVIDER = 'https://rpc-mumbai.maticvigil.com';
const provider = new ethers.providers.JsonRpcProvider(RPC_PROVIDER);

const signTransaction = async () => {
  const unsignedTx = UNSIGNED_TX;
  const signature = {
    r: '0x' + SIGNATURE_RESULT.R,
    s: '0x' + SIGNATURE_RESULT.S,
}

  for(let i = 0; i < v.length; i++) {
    signature.v = v[i];
     const joinedSignature = ethers.utils.joinSignature(signature)

    const rawTransaction = ethers.utils.serializeTransaction(unsignedTx);
    const msgHash = ethers.utils.keccak256(rawTransaction) // as specified by ECDSA
    const msgBytes = ethers.utils.arrayify(msgHash) // create binary hash
    const recoveredAddress = ethers.utils.recoverAddress(msgBytes, joinedSignature)
    
    if(recoveredAddress.toLowerCase() === publicKeyToAddress(SIGNATURE_RESULT.public_key).toLowerCase()) {
        const submitData = ethers.utils.serializeTransaction(unsignedTx, signature);
        const txRes = await provider.sendTransaction(submitData);
        console.log('Transaction receipt:',txRes);
        await txRes.wait();
        break;
    }
  }
}

signTransaction();
